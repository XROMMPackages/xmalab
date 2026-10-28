//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
//
//  All Rights Reserved
//
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided
//  that this copyright notice appear in all copies and that the name of Brown University not be used in
//  advertising or publicity pertaining to the use or distribution of the software without specific written
//  prior permission from Brown University.
//
//  See license.txt for further information.
//  ----------------------------------
//
///\file MergeMonitor.cpp

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MergeMonitor.h"
#include "processing/MarkerDetection.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/Camera.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "core/Image.h"

#include <QStringList>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <functional>

// windows.h may have leaked min/max macros through the Qt/OpenCV headers above
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace xma;

namespace
{
	const double INF = 1e6;
}

MergeMonitor* MergeMonitor::instance = nullptr;

MergeMonitor* MergeMonitor::getInstance()
{
	if (!instance) instance = new MergeMonitor();
	return instance;
}

MergeMonitor::MergeMonitor()
	: m_lastTrial(-1)
	, m_lastFrame(-1)
	, m_runStart(-1)
	, m_lastForward(true)
	, m_debug(qEnvironmentVariableIsSet("XMALAB_MERGE_DEBUG"))
{
}

void MergeMonitor::reset()
{
	m_groups.clear();
	m_holds.clear();
	m_medianHistory.clear();
	m_medianValid.clear();
	m_inconsistentLast.clear();
	m_inconsistentNow.clear();
	m_allMessages.clear();
	m_noVelocity.clear();
	m_lastTrial = -1;
	m_lastFrame = -1;
}

// ---------------------------------------------------------------------------
// thresholds and small helpers
// ---------------------------------------------------------------------------

double MergeMonitor::epiThreshold()
{
	return Settings::getInstance()->getFloatSetting("MergeEpipolarThreshold");
}

double MergeMonitor::auditThreshold()
{
	return Settings::getInstance()->getFloatSetting("MergeAuditThreshold");
}

double MergeMonitor::maxCost()
{
	// a member may only be placed where it is consistent with its evidence
	return 3.0 * epiThreshold();
}

int MergeMonitor::maxPendingFrames()
{
	return Settings::getInstance()->getIntSetting("MergeMaxPendingFrames");
}

int MergeMonitor::maxHoldFrames()
{
	return Settings::getInstance()->getIntSetting("MergeMaxHoldFrames");
}

int MergeMonitor::maxOverlapFrames()
{
	return Settings::getInstance()->getIntSetting("MergeMaxOverlapFrames");
}

double MergeMonitor::radius(Marker* m)
{
	double r = m->getSize();
	if (r < 1.0 || r > 50.0) r = 3.0;
	return r;
}

bool MergeMonitor::singleBlob(Marker* m, double size)
{
	double r = radius(m);
	return size >= 0.7 * r && size <= 1.4 * r;
}

double MergeMonitor::mergeDistance(Trial* trial, const std::vector<int>& members)
{
	double r = 0.0;
	for (int m : members) r = std::max(r, radius(trial->getMarkers()[m]));
	return 2.0 * r + 1.0;
}

bool MergeMonitor::isMeasured(Marker* m, int camera, int frame)
{
	if (camera < 0 || camera >= (int)m->getStatus2D().size()) return false;
	if (frame < 0 || frame >= (int)m->getStatus2D()[camera].size()) return false;
	return m->getStatus2D()[camera][frame] > UNDEFINED;
}

bool MergeMonitor::isEditable(Marker* m, int camera, int frame)
{
	if (!isMeasured(m, camera, frame)) return false;
	markerStatus s = m->getStatus2D()[camera][frame];
	return s == TRACKED || s == PREDICTED;
}

bool MergeMonitor::inGroup(int camera, int marker) const
{
	for (const Group& g : m_groups)
	{
		if (g.camera == camera && std::find(g.members.begin(), g.members.end(), marker) != g.members.end())
			return true;
	}
	return false;
}

std::vector<int> MergeMonitor::cleanCameras(Trial* trial, int marker, int camera, int frame)
{
	std::vector<int> out;
	Marker* mk = trial->getMarkers()[marker];
	for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
	{
		if ((int)c == camera) continue;
		if (!Project::getInstance()->getCameras()[c]->isVisible()) continue;
		if (!isMeasured(mk, c, frame)) continue;
		if (mk->getStatus2D()[c][frame] == PREDICTED) continue;
		if (inGroup(c, marker)) continue;
		out.push_back((int)c);
	}
	return out;
}

bool MergeMonitor::rigidBodyPrediction(Trial* trial, int marker, const std::vector<int>& exclude, int frame, int camera, cv::Point2d& projected)
{
	for (RigidBody* rb : trial->getRigidBodies())
	{
		const std::vector<int>& idx = rb->getPointsIdx();
		if (std::find(idx.begin(), idx.end(), marker) == idx.end()) continue;
		cv::Point3d p3d;
		int used = 0;
		if (rb->predictMarkerPosition(marker, frame, exclude, p3d, &used) && used >= 3)
		{
			projected = Project::getInstance()->getCameras()[camera]->projectPoint(p3d, trial->getReferenceCalibrationImage());
			return true;
		}
	}
	return false;
}

cv::Point2d MergeMonitor::holdPosition(Trial* trial, Group& g, int marker, int frame, const std::vector<int>& clean, bool& usedBody)
{
	Marker* mk = trial->getMarkers()[marker];
	cv::Point2d base = mk->getPoints2D()[g.camera][frame];
	int dir = m_lastForward ? 1 : -1;
	std::map<int, std::vector<bool> >::const_iterator mv = m_medianValid.find(frame);
	if (isMeasured(mk, g.camera, frame - dir) && mv != m_medianValid.end() && g.camera < (int)mv->second.size() && mv->second[g.camera])
	{
		// where the marker should be if it moved with the others; trust the tracker only if it agrees
		cv::Point2d expected = mk->getPoints2D()[g.camera][frame - dir] + m_medianHistory[frame][g.camera];
		if (cv::norm(base - expected) > 2.0 * radius(mk) + 1.0) base = expected;
	}
	cv::Point2d rbProj;
	usedBody = rigidBodyPrediction(trial, marker, g.members, frame, g.camera, rbProj);
	if (usedBody) base = rbProj;

	cv::Point2d held(0.0, 0.0);
	for (int c : clean)
		held += mk->projectOntoEpipolarLine(c, g.camera, mk->getPoints2D()[c][frame], base);
	held *= 1.0 / (double)clean.size();
	return held;
}

QString MergeMonitor::memberList(Trial* trial, const std::vector<int>& members)
{
	QStringList parts;
	for (int m : members)
	{
		QString d = trial->getMarkers()[m]->getDescription();
		parts.push_back(d.isEmpty() ? QString::number(m + 1) : QString("%1 (%2)").arg(m + 1).arg(d));
	}
	return parts.join(", ");
}

int MergeMonitor::templateFrame(int camera, int marker, int frame)
{
	std::map<std::pair<int, int>, HoldInfo>::const_iterator h = m_holds.find(std::make_pair(camera, marker));
	if (h != m_holds.end()) return h->second.templateFrame;
	for (const Group& g : m_groups)
	{
		if (g.camera != camera) continue;
		std::map<int, int>::const_iterator it = g.templateFrame.find(marker);
		if (it != g.templateFrame.end()) return it->second;
	}
	return frame;
}

bool MergeMonitor::predictionOverride(int trialIdx, int camera, int marker, int frameFrom, double& x, double& y)
{
	if (frameFrom != m_lastFrame || trialIdx != m_lastTrial) return false;
	if (m_noVelocity.find(std::make_pair(camera, marker)) == m_noVelocity.end()) return false;
	if (trialIdx < 0 || trialIdx >= (int)Project::getInstance()->getTrials().size()) return false;
	Trial* trial = Project::getInstance()->getTrials()[trialIdx];
	if (marker < 0 || marker >= (int)trial->getMarkers().size()) return false;
	Marker* mk = trial->getMarkers()[marker];
	if (!isMeasured(mk, camera, frameFrom)) return false;
	x = mk->getPoints2D()[camera][frameFrom].x;
	y = mk->getPoints2D()[camera][frameFrom].y;
	return true;
}

// ---------------------------------------------------------------------------
// per-frame entry point
// ---------------------------------------------------------------------------

void MergeMonitor::processFrame(int trialIdx, int frame, bool forward, const std::vector<int>& markers)
{
	m_messages.clear();
	if (trialIdx < 0 || trialIdx >= (int)Project::getInstance()->getTrials().size()) return;
	if (Project::getInstance()->getCalibration() == NO_CALIBRATION) return;

	Trial* trial = Project::getInstance()->getTrials()[trialIdx];
	int dir = forward ? 1 : -1;
	if (trialIdx != m_lastTrial || frame != m_lastFrame + dir || forward != m_lastForward)
	{
		m_groups.clear();
		m_holds.clear();
		m_medianHistory.clear();
		m_medianValid.clear();
		m_inconsistentLast.clear();
		m_runStart = frame;
	}
	m_lastTrial = trialIdx;
	m_lastFrame = frame;
	m_lastForward = forward;
	m_currentMarkers = markers;
	m_noVelocity.clear();

	// a project or trial change can leave groups that refer to markers that no longer exist
	for (size_t i = 0; i < m_groups.size();)
	{
		bool valid = true;
		for (int m : m_groups[i].members)
			if (m < 0 || m >= (int)trial->getMarkers().size()) valid = false;
		if (!valid) m_groups.erase(m_groups.begin() + i);
		else ++i;
	}

	{
		unsigned int nbCams = Project::getInstance()->getCameras().size();
		std::vector<bool> haveMedian(nbCams, false);
		std::vector<cv::Point2d> median(nbCams);
		for (unsigned int c = 0; c < nbCams; c++)
			if (Project::getInstance()->getCameras()[c]->isVisible())
				haveMedian[c] = medianMotion(trial, (int)c, frame, dir, markers, median[c]);
		m_medianHistory[frame] = median;
		m_medianValid[frame] = haveMedian;
	}
	for (size_t i = 0; i < m_groups.size();)
	{
		updateGroup(trial, m_groups[i], frame);
		if (m_groups[i].members.size() < 2)
			m_groups.erase(m_groups.begin() + i);
		else
			++i;
	}

	for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
	{
		if (!Project::getInstance()->getCameras()[c]->isVisible()) continue;
		detectNewGroups(trial, (int)c, frame, forward, markers);
	}

	m_inconsistentNow.clear();
	checkMarkers(trial, frame, forward, markers);
	m_inconsistentLast = m_inconsistentNow;
	m_allMessages.insert(m_allMessages.end(), m_messages.begin(), m_messages.end());
}

// ---------------------------------------------------------------------------
// merge detection and holding
// ---------------------------------------------------------------------------

void MergeMonitor::detectNewGroups(Trial* trial, int camera, int frame, bool forward, const std::vector<int>& markers)
{
	std::vector<int> cand;
	for (int m : markers)
	{
		if (m < 0 || m >= (int)trial->getMarkers().size()) continue;
		Marker* mk = trial->getMarkers()[m];
		if (!isEditable(mk, camera, frame) || inGroup(camera, m)) continue;
		if (cleanCameras(trial, m, camera, frame).empty()) continue;   // nothing to hold it against
		cand.push_back(m);
	}
	if (cand.size() < 2) return;

	std::map<int, int> parent;
	for (int m : cand) parent[m] = m;
	std::function<int(int)> find = [&](int x)
	{
		while (parent[x] != x)
		{
			parent[x] = parent[parent[x]];
			x = parent[x];
		}
		return x;
	};

	for (size_t i = 0; i < cand.size(); i++)
	{
		Marker* a = trial->getMarkers()[cand[i]];
		for (size_t j = i + 1; j < cand.size(); j++)
		{
			Marker* b = trial->getMarkers()[cand[j]];
			double d = cv::norm(a->getPoints2D()[camera][frame] - b->getPoints2D()[camera][frame]);
			double r = std::max(radius(a), radius(b));
			bool touching = d < 2.0 * r + 1.0;
			bool swollen = d < 3.0 * r &&
				(a->getSize(camera, frame) > 1.5 * radius(a) || b->getSize(camera, frame) > 1.5 * radius(b));
			if (touching || swollen)
				parent[find(cand[i])] = find(cand[j]);
		}
	}

	std::map<int, std::vector<int> > groups;
	for (int m : cand) groups[find(m)].push_back(m);

	int dir = forward ? 1 : -1;
	for (std::map<int, std::vector<int> >::iterator it = groups.begin(); it != groups.end(); ++it)
	{
		if (it->second.size() < 2) continue;
		intruders(trial, camera, frame, dir, it->second);
		if (it->second.size() < 2) continue;

		Group g;
		g.camera = camera;
		g.state = MERGED;
		g.members = it->second;
		g.startFrame = frame;

		bool anyClean = false;
		for (int m : g.members)
		{
			Marker* mk = trial->getMarkers()[m];
			int nb = (int)mk->getStatus2D()[camera].size();
			int tf = frame - dir;
			int tries = 0;
			// the last frame where this marker sat on a single blob of its own
			while (tf >= 0 && tf < nb && tries < 10 &&
				!(mk->getStatus2D()[camera][tf] >= TRACKED && singleBlob(mk, mk->getSize(camera, tf))))
			{
				tf -= dir;
				tries++;
			}
			if (tf < 0 || tf >= nb || mk->getStatus2D()[camera][tf] < TRACKED)
			{
				tf = frame - dir;
				if (tf < 0 || tf >= nb || mk->getStatus2D()[camera][tf] < TRACKED) tf = frame;
			}
			g.templateFrame[m] = tf;
			if (!cleanCameras(trial, m, camera, frame).empty()) anyClean = true;
		}

		if (!anyClean)
		{
			log(QString("Merge-aware: frame %1, camera %2: markers %3 overlap but no other camera sees them apart; left to 2D tracking")
				.arg(frame + 1).arg(camera + 1).arg(memberList(trial, g.members)));
			continue;
		}

		log(QString("Merge-aware: frame %1, camera %2: markers %3 overlap; holding them on their epipolar lines")
			.arg(frame + 1).arg(camera + 1).arg(memberList(trial, g.members)));
		for (int m : g.members)
		{
			std::map<std::pair<int, int>, HoldInfo>::iterator h = m_holds.find(std::make_pair(camera, m));
			if (h != m_holds.end())
			{
				g.templateFrame[m] = h->second.templateFrame;
				m_holds.erase(h);
			}
		}
		for (int m : g.members) holdMember(trial, g, m, frame);
		m_groups.push_back(g);
	}
}

void MergeMonitor::holdMember(Trial* trial, Group& g, int marker, int frame)
{
	Marker* mk = trial->getMarkers()[marker];
	if (!isEditable(mk, g.camera, frame)) return;
	std::vector<int> clean = cleanCameras(trial, marker, g.camera, frame);
	if (clean.empty()) return;

	cv::Point2d p = mk->getPoints2D()[g.camera][frame];
	bool usedBody = false;
	cv::Point2d held = holdPosition(trial, g, marker, frame, clean, usedBody);

	if (m_debug)
		log(QString("  [debug] frame %1 cam %2: marker %3 held at (%4,%5)%6, tracker had (%7,%8)")
			.arg(frame + 1).arg(g.camera + 1).arg(marker + 1).arg(held.x, 0, 'f', 1).arg(held.y, 0, 'f', 1)
			.arg(usedBody ? " from rigid body" : "").arg(p.x, 0, 'f', 1).arg(p.y, 0, 'f', 1));

	mk->setPoint(g.camera, frame, held.x, held.y, PREDICTED);
	mk->setSize(g.camera, frame, -1);
	if (g.heldFrames.empty() || g.heldFrames.back() != frame) g.heldFrames.push_back(frame);
}

void MergeMonitor::placeMember(Trial* trial, Group& g, int member, int frame, const cv::Point2d& pt, double size, bool tracked)
{
	Marker* mk = trial->getMarkers()[member];
	if (!isEditable(mk, g.camera, frame)) return;
	mk->setPoint(g.camera, frame, pt.x, pt.y, tracked ? TRACKED : PREDICTED);
	mk->setSize(g.camera, frame, size);
	if (singleBlob(mk, size)) g.templateFrame[member] = frame;
	m_noVelocity.insert(std::make_pair(g.camera, member));
}

// ---------------------------------------------------------------------------
// split handling
// ---------------------------------------------------------------------------

std::vector<MergeMonitor::Candidate> MergeMonitor::candidates(Trial* trial, Group& g, int frame, bool useDetector, const std::vector<int>& markers)
{
	std::vector<Candidate> out;
	double dmin = mergeDistance(trial, g.members);

	auto add = [&](cv::Point2d p, double s, int from)
	{
		for (const Candidate& c : out)
			if (cv::norm(c.pt - p) < dmin) return;
		Candidate cnd;
		cnd.pt = p;
		cnd.size = s;
		cnd.fromMember = from;
		out.push_back(cnd);
	};

	std::vector<cv::Point2d> seeds;
	for (size_t i = 0; i < g.members.size(); i++)
	{
		Marker* mk = trial->getMarkers()[g.members[i]];
		if (!isMeasured(mk, g.camera, frame)) continue;
		add(mk->getPoints2D()[g.camera][frame], mk->getSize(g.camera, frame), (int)i);
		seeds.push_back(mk->getPoints2D()[g.camera][frame]);
	}

	if (useDetector && !out.empty() && (int)trial->getVideoStreams().size() > g.camera)
	{
		Marker* ref = trial->getMarkers()[g.members[0]];
		int method = ref->getMethod();
		if (method != 3 && method != 4)
		{
			Image* image = trial->getVideoStreams()[g.camera]->getImage();
			double r = radius(ref);
			int masksize = ref->getSizeOverride() > 0 ? ref->getSizeOverride() : (ref->getSize() > 0 ? (int)(ref->getSize() + 0.5) : 5);
			int searchArea = std::max(12, (int)(6.0 * r + 6.0));

			// look where the markers are expected as well as where the trackers sit
			for (size_t i = 0; i < g.members.size(); i++)
			{
				cv::Point2d rbProj;
				if (rigidBodyPrediction(trial, g.members[i], g.members, frame, g.camera, rbProj)) seeds.push_back(rbProj);
			}
			std::vector<cv::Point2d> uniqueSeeds;
			for (const cv::Point2d& s : seeds)
			{
				bool dup = false;
				for (const cv::Point2d& u : uniqueSeeds)
					if (cv::norm(u - s) < 0.5 * searchArea) dup = true;
				if (!dup) uniqueSeeds.push_back(s);
			}

			QString dbg;
			for (const cv::Point2d& seed : uniqueSeeds)
			{
				std::vector<cv::Point2d> allCentres;
				std::vector<double> allSizes;
				double dummy = -1.0;
				MarkerDetection::detectionPoint(image, method, seed, searchArea, masksize, ref->getThresholdOffset(), &dummy, NULL, false, &allCentres, &allSizes);

				if (m_debug)
				{
					dbg += QString(" seed(%1,%2):").arg(seed.x, 0, 'f', 0).arg(seed.y, 0, 'f', 0);
					for (size_t k = 0; k < allCentres.size() && k < allSizes.size() && k < 8; k++)
						dbg += QString(" (%1,%2 r%3)").arg(allCentres[k].x, 0, 'f', 0).arg(allCentres[k].y, 0, 'f', 0).arg(allSizes[k], 0, 'f', 1);
				}

				for (size_t k = 0; k < allCentres.size() && k < allSizes.size(); k++)
				{
					double sz = allSizes[k];
					if (!singleBlob(ref, sz)) continue;
					if (cv::norm(allCentres[k] - seed) > searchArea) continue;

					bool claimed = false;
					for (int m : markers)
					{
						if (std::find(g.members.begin(), g.members.end(), m) != g.members.end()) continue;
						if (m < 0 || m >= (int)trial->getMarkers().size()) continue;
						Marker* o = trial->getMarkers()[m];
						if (isMeasured(o, g.camera, frame) && cv::norm(o->getPoints2D()[g.camera][frame] - allCentres[k]) < dmin)
						{
							claimed = true;
							break;
						}
					}
					if (!claimed) add(allCentres[k], sz, -1);
				}
			}
			if (m_debug && !dbg.isEmpty())
				log(QString("  [debug] frame %1 cam %2 contours:%3").arg(frame + 1).arg(g.camera + 1).arg(dbg));
		}
	}
	return out;
}

bool MergeMonitor::assign(Trial* trial, Group& g, int frame, const std::vector<Candidate>& cands, Assignment& result)
{
	int n = (int)g.members.size();
	int k = (int)cands.size();
	if (n < 1 || k < n) return false;

	double T = epiThreshold();
	double dmin = mergeDistance(trial, g.members);

	std::vector<std::vector<double> > cost(n, std::vector<double>(k, 0.0));
	std::vector<double> ownCost(n, INF);
	std::vector<bool> evidence(n, false);
	bool anyEvidence = false;

	for (int i = 0; i < n; i++)
	{
		Marker* mk = trial->getMarkers()[g.members[i]];
		std::vector<int> clean = cleanCameras(trial, g.members[i], g.camera, frame);
		cv::Point2d rbProj;
		bool haveRb = rigidBodyPrediction(trial, g.members[i], g.members, frame, g.camera, rbProj);
		if (clean.empty() && !haveRb) continue;
		evidence[i] = true;
		anyEvidence = true;

		auto costAt = [&](const cv::Point2d& p)
		{
			double c = 0.0;
			for (int cam : clean)
			{
				double d = mk->distanceToEpipolarLine(cam, g.camera, mk->getPoints2D()[cam][frame], p);
				if (d >= 0.0) c += d;
			}
			if (haveRb) c += cv::norm(p - rbProj);
			return c;
		};

		bool measured = isMeasured(mk, g.camera, frame);
		cv::Point2d own = measured ? mk->getPoints2D()[g.camera][frame] : cv::Point2d(-1, -1);
		// a blob far from where the marker can plausibly be is not a candidate for it
		int dir = m_lastForward ? 1 : -1;
		bool haveExpected = false;
		cv::Point2d expected;
		double reach = 4.0 * radius(mk) + 4.0;
		if (isMeasured(mk, g.camera, frame - dir))
		{
			expected = mk->getPoints2D()[g.camera][frame - dir];
			std::map<int, std::vector<bool> >::const_iterator mv = m_medianValid.find(frame);
			if (mv != m_medianValid.end() && g.camera < (int)mv->second.size() && mv->second[g.camera])
			{
				expected += m_medianHistory[frame][g.camera];
				reach = std::max(reach, 3.0 * cv::norm(m_medianHistory[frame][g.camera]) + 6.0);
			}
			haveExpected = true;
		}
		if (haveRb)
		{
			expected = rbProj;
			reach = std::max(reach, 4.0 * radius(mk) + 4.0);
			haveExpected = true;
		}
		ownCost[i] = measured ? costAt(own) : INF;
		double gate = std::max(T, 1.5 * radius(mk));

		for (int j = 0; j < k; j++)
		{
			double c = costAt(cands[j].pt);
			if (haveExpected && cv::norm(cands[j].pt - expected) > reach) c = INF;
			bool isOwn = measured && cv::norm(cands[j].pt - own) < dmin;
			if (cands[j].fromMember < 0 && !isOwn)
			{
				// a blob the detector found: only usable if this marker's current blob is clearly
				// inconsistent with its evidence and the new blob clearly improves on it
				if (!(ownCost[i] > gate && c < ownCost[i] - T)) c = INF;
			}
			cost[i][j] = c;
		}
	}
	if (!anyEvidence) return false;

	double best = INF;
	std::vector<int> cur(n, -1);
	std::vector<int> bestChoice;
	std::vector<bool> used(k, false);
	std::function<void(int, double)> rec = [&](int i, double acc)
	{
		if (acc >= INF) return;
		if (i == n)
		{
			if (acc < best)
			{
				best = acc;
				bestChoice = cur;
			}
			return;
		}
		for (int j = 0; j < k; j++)
		{
			if (used[j]) continue;
			if (cost[i][j] >= INF) continue;
			used[j] = true;
			cur[i] = j;
			rec(i + 1, acc + cost[i][j]);
			used[j] = false;
		}
	};
	rec(0, 0.0);
	if (bestChoice.empty()) return false;

	result.choice = bestChoice;
	result.cost.assign(n, 0.0);
	result.margin.assign(n, INF);
	result.evidence = evidence;
	result.minMargin = INF;
	for (int i = 0; i < n; i++)
	{
		result.cost[i] = cost[i][bestChoice[i]];
		if (!evidence[i]) continue;
		double alt = INF;
		for (int j = 0; j < k; j++)
		{
			if (j == bestChoice[i] || cost[i][j] >= INF) continue;
			alt = std::min(alt, cost[i][j]);
		}
		result.margin[i] = (alt < INF) ? alt - result.cost[i] : INF;
		result.minMargin = std::min(result.minMargin, result.margin[i]);
	}

	if (m_debug)
	{
		QString line = QString("  [debug] frame %1 cam %2 group %3:").arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members));
		for (int j = 0; j < k; j++)
		{
			line += QString(" cand%1(%2,%3 r%4%5)[").arg(j).arg(cands[j].pt.x, 0, 'f', 1).arg(cands[j].pt.y, 0, 'f', 1).arg(cands[j].size, 0, 'f', 1).arg(cands[j].fromMember >= 0 ? " own" : " det");
			for (int i = 0; i < n; i++)
				line += (cost[i][j] >= INF ? QString("x") : QString::number(cost[i][j], 'f', 1)) + (i + 1 < n ? "," : "");
			line += "]";
		}
		line += " choice[";
		for (int i = 0; i < n; i++) line += QString::number(bestChoice[i]) + (i + 1 < n ? "," : "");
		line += "] margins[";
		for (int i = 0; i < n; i++) line += (result.margin[i] >= INF ? QString("inf") : QString::number(result.margin[i], 'f', 1)) + (i + 1 < n ? "," : "");
		line += "]";
		log(line);
	}
	return true;
}

void MergeMonitor::commitPending(Trial* trial, Group& g, int frame, const std::vector<int>& order)
{
	bool identity = true;
	for (size_t i = 0; i < order.size(); i++)
		if (order[i] != (int)i) identity = false;

	std::vector<int> frames = g.pendingFrames;
	frames.push_back(frame);
	size_t n = g.members.size();

	for (int f : frames)
	{
		std::vector<cv::Point2d> pts(n);
		std::vector<double> sizes(n);
		std::vector<bool> ok(n, true);
		for (size_t i = 0; i < n; i++)
		{
			Marker* mk = trial->getMarkers()[g.members[i]];
			ok[i] = isEditable(mk, g.camera, f);
			if (ok[i])
			{
				pts[i] = mk->getPoints2D()[g.camera][f];
				sizes[i] = mk->getSize(g.camera, f);
			}
		}
		bool allOk = true;
		for (size_t i = 0; i < n; i++) allOk = allOk && ok[i];

		for (size_t i = 0; i < n; i++)
		{
			if (!ok[i]) continue;
			size_t src = (identity || !allOk) ? i : (size_t)order[i];
			Marker* mk = trial->getMarkers()[g.members[i]];
			mk->setPoint(g.camera, f, pts[src].x, pts[src].y, TRACKED);
			mk->setSize(g.camera, f, sizes[src]);
		}
	}
}

void MergeMonitor::dropMember(Trial* trial, Group& g, int member, int frame, const QString& why)
{
	int start = frame;
	if (!g.heldFrames.empty()) start = g.heldFrames.front();
	else if (!g.pendingFrames.empty()) start = g.pendingFrames.front();
	int cleared = clearWindow(trial, member, g.camera, start, frame);
	log(QString("Merge-aware: frame %1, camera %2: marker %3 stopped in this camera because %4; %5 frame(s) cleared")
		.arg(frame + 1).arg(g.camera + 1).arg(member + 1).arg(why).arg(cleared));
	g.members.erase(std::remove(g.members.begin(), g.members.end(), member), g.members.end());
	g.templateFrame.erase(member);
}

void MergeMonitor::stopGroup(Trial* trial, Group& g, int frame, const QString& why)
{
	int start = frame;
	if (!g.heldFrames.empty()) start = g.heldFrames.front();
	else if (!g.pendingFrames.empty()) start = g.pendingFrames.front();
	int cleared = 0;
	for (int m : g.members) cleared += clearWindow(trial, m, g.camera, start, frame);
	std::vector<int> frames(1, cleared);
	log(QString("Merge-aware: frame %1, camera %2: markers %3 stopped in this camera because %4; %5 frame(s) cleared")
		.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)).arg(why).arg((int)frames.size()));
	g.members.clear();
}

void MergeMonitor::updateGroup(Trial* trial, Group& g, int frame)
{
	std::vector<int> alive;
	std::vector<int> lost;
	for (int m : g.members)
	{
		if (isMeasured(trial->getMarkers()[m], g.camera, frame)) alive.push_back(m);
		else lost.push_back(m);
	}
	if (!lost.empty())
	{
		if (g.state == PENDING)
		{
			stopGroup(trial, g, frame, "a marker was lost before the split was resolved");
			return;
		}
		log(QString("Merge-aware: frame %1, camera %2: markers %3 no longer tracked; released from the overlap group")
			.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, lost)));
		g.members = alive;
		if (g.members.size() < 2)
		{
			g.members.clear();
			return;
		}
	}

	// a member that no other camera can vouch for can be neither held nor assigned: stop it in this camera
	{
		std::vector<int> blind;
		for (int m : g.members)
		{
			cv::Point2d rbProj;
			if (cleanCameras(trial, m, g.camera, frame).empty() && !rigidBodyPrediction(trial, m, g.members, frame, g.camera, rbProj))
				blind.push_back(m);
		}
		for (int m : blind) dropMember(trial, g, m, frame, "no other camera sees it");
		if (g.members.size() < 2)
		{
			if (g.state == PENDING && !g.members.empty())
			{
				std::vector<int> identity(g.members.size());
				for (size_t i = 0; i < identity.size(); i++) identity[i] = (int)i;
				commitPending(trial, g, frame, identity);
			}
			g.members.clear();
			return;
		}
	}

	double dmin = mergeDistance(trial, g.members);
	double T = epiThreshold();
	int n = (int)g.members.size();

	if (g.state == MERGED)
	{
		if ((int)g.heldFrames.size() >= maxOverlapFrames())
		{
			// an overlap this long without a resolvable split: the held points are only on the right line, not
			// at the right place along it, so clear them rather than keep guessing
			int start = g.heldFrames.empty() ? frame : g.heldFrames.front();
			int cleared = 0;
			for (int m : g.members) cleared += clearWindow(trial, m, g.camera, start, frame);
			std::vector<int> frames(1, cleared);
			log(QString("Merge-aware: frame %1, camera %2: markers %3 stopped in this camera, their overlap was not resolved within %4 frames; %5 frame(s) cleared")
				.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)).arg(maxOverlapFrames()).arg((int)frames.size()));
			g.members.clear();
			return;
		}
		std::vector<Candidate> cands = candidates(trial, g, frame, true, m_currentMarkers);
		Assignment A;
		if ((int)cands.size() < n || !assign(trial, g, frame, cands, A))
		{
			for (int m : g.members) holdMember(trial, g, m, frame);
			return;
		}

		std::vector<bool> clean(n, false);
		bool allClean = true;
		for (int i = 0; i < n; i++)
		{
			Marker* mk = trial->getMarkers()[g.members[i]];
			const Candidate& c = cands[A.choice[i]];
			clean[i] = singleBlob(mk, c.size) && A.cost[i] <= maxCost();
			if (!clean[i]) allClean = false;
		}

		if (allClean && A.minMargin > T)
		{
			for (int i = 0; i < n; i++) placeMember(trial, g, g.members[i], frame, cands[A.choice[i]].pt, cands[A.choice[i]].size, true);
			log(QString("Merge-aware: frame %1, camera %2: markers %3 separated; assigned with a %4 px margin")
				.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)).arg(A.minMargin >= INF ? QString("clear") : QString::number(A.minMargin, 'f', 1)));
			g.members.clear();
			return;
		}

		// a provisional split is only meaningful when the sole ambiguity is which member is which; with more
		// blobs than members an ambiguous member is held until a later frame separates the options
		if (allClean && (int)cands.size() == n)
		{
			for (int i = 0; i < n; i++) placeMember(trial, g, g.members[i], frame, cands[A.choice[i]].pt, cands[A.choice[i]].size, false);
			g.state = PENDING;
			g.pendingFrames.clear();
			g.pendingFrames.push_back(frame);
			log(QString("Merge-aware: frame %1, camera %2: markers %3 separated but the margin is only %4 px; assignment provisional")
				.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)).arg(A.minMargin, 0, 'f', 1));
			return;
		}

		// some members have a clean blob of their own, the rest are still merged: place what is known, hold the rest
		for (int i = 0; i < n; i++)
		{
			if (clean[i] && A.margin[i] > T)
				placeMember(trial, g, g.members[i], frame, cands[A.choice[i]].pt, cands[A.choice[i]].size, false);
			else
				holdMember(trial, g, g.members[i], frame);
		}
		return;
	}

	// PENDING: members follow their provisional blobs; decide when the evidence allows.
	std::vector<Candidate> cands;
	for (int i = 0; i < n; i++)
	{
		Marker* mk = trial->getMarkers()[g.members[i]];
		Candidate cnd;
		cnd.pt = mk->getPoints2D()[g.camera][frame];
		cnd.size = mk->getSize(g.camera, frame);
		cnd.fromMember = i;
		cands.push_back(cnd);
	}
	for (size_t i = 0; i < cands.size(); i++)
	{
		for (size_t j = i + 1; j < cands.size(); j++)
		{
			if (cv::norm(cands[i].pt - cands[j].pt) < dmin)
			{
				stopGroup(trial, g, frame, "the markers overlapped again before the split was resolved");
				return;
			}
		}
	}

	Assignment A;
	bool ok = assign(trial, g, frame, cands, A);
	if (ok)
	{
		// a member whose best option is still inconsistent with its evidence has lost its blob
		std::vector<int> toDrop;
		for (int i = 0; i < n; i++)
			if (A.evidence[i] && A.cost[i] > maxCost()) toDrop.push_back(g.members[i]);
		if (!toDrop.empty())
		{
			for (int m : toDrop)
				dropMember(trial, g, m, frame, QString("it no longer matches its evidence (%1 px)").arg(maxCost(), 0, 'f', 0));
			if (g.members.size() < 2)
			{
				// the survivor is consistent on its own; keep its provisional frames as tracked
				std::vector<int> identity(g.members.size());
				for (size_t i = 0; i < identity.size(); i++) identity[i] = (int)i;
				commitPending(trial, g, frame, identity);
				if (!g.members.empty())
					log(QString("Merge-aware: frame %1, camera %2: marker %3 kept as tracked after its partner was dropped")
						.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)));
				g.members.clear();
			}
			return;
		}

		if (A.minMargin > T)
		{
			bool swapped = false;
			for (size_t i = 0; i < A.choice.size(); i++)
				if (A.choice[i] != (int)i) swapped = true;
			int nbPending = (int)g.pendingFrames.size();
			commitPending(trial, g, frame, A.choice);
			log(QString("Merge-aware: frame %1, camera %2: markers %3 confirmed after %4 provisional frame(s)%5 (margin %6 px)")
				.arg(frame + 1).arg(g.camera + 1).arg(memberList(trial, g.members)).arg(nbPending)
				.arg(swapped ? ", identities swapped" : "").arg(A.minMargin >= INF ? QString("clear") : QString::number(A.minMargin, 'f', 1)));
			g.members.clear();
			return;
		}
	}

	for (int m : g.members)
	{
		Marker* mk = trial->getMarkers()[m];
		if (mk->getStatus2D()[g.camera][frame] == TRACKED)
		{
			cv::Point2d p = mk->getPoints2D()[g.camera][frame];
			mk->setPoint(g.camera, frame, p.x, p.y, PREDICTED);
		}
	}
	g.pendingFrames.push_back(frame);
	if ((int)g.pendingFrames.size() >= maxPendingFrames())
		stopGroup(trial, g, frame, QString("the split is still ambiguous after %1 frames").arg((int)g.pendingFrames.size()));
}

// ---------------------------------------------------------------------------
// markers outside any group: hold what cannot be trusted, stop what cannot recover
// ---------------------------------------------------------------------------

int MergeMonitor::lastSingleBlobFrame(Marker* mk, int camera, int frame, int dir)
{
	int nb = (int)mk->getStatus2D()[camera].size();
	int tf = frame - dir;
	int tries = 0;
	while (tf >= 0 && tf < nb && tries < 10 &&
		!(mk->getStatus2D()[camera][tf] >= TRACKED && singleBlob(mk, mk->getSize(camera, tf))))
	{
		tf -= dir;
		tries++;
	}
	if (tf >= 0 && tf < nb && mk->getStatus2D()[camera][tf] >= TRACKED) return tf;
	tf = frame - dir;
	if (tf >= 0 && tf < nb && mk->getStatus2D()[camera][tf] >= TRACKED) return tf;
	return frame;
}

double MergeMonitor::displacement(Marker* mk, int camera, int frame, int dir)
{
	if (!isMeasured(mk, camera, frame) || !isMeasured(mk, camera, frame - dir)) return -1.0;
	return cv::norm(mk->getPoints2D()[camera][frame] - mk->getPoints2D()[camera][frame - dir]);
}

int MergeMonitor::clearWindow(Trial* trial, int marker, int camera, int from, int to)
{
	Marker* mk = trial->getMarkers()[marker];
	int dir = m_lastForward ? 1 : -1;
	int nbVisible = 0;
	for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		if (Project::getInstance()->getCameras()[c]->isVisible()) nbVisible++;

	int cleared = 0;
	for (int f = from; (to - f) * dir >= 0; f += dir)
	{
		if (isEditable(mk, camera, f))
		{
			mk->reset(camera, f);
			cleared++;
		}
		if (nbVisible == 2)
		{
			for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
			{
				if ((int)c == camera || !Project::getInstance()->getCameras()[c]->isVisible()) continue;
				if (isEditable(mk, (int)c, f))
				{
					mk->reset((int)c, f);
					cleared++;
				}
			}
		}
	}
	return cleared;
}

bool MergeMonitor::markerSizedBlob(Marker* mk, int camera, int frame)
{
	if (mk->getMethod() == 4) return true;
	double r = radius(mk);
	double sz = mk->getSize(camera, frame);
	return sz >= 0.7 * r && sz <= 2.0 * r;
}

// Hold a single marker's point in `camera` on the epipolar line(s) of its other measurements, at the
// position it had on the previous frame. Returns false (and resets the point) when it cannot be held.
bool MergeMonitor::holdSingle(Trial* trial, int marker, int camera, int frame, int dir, const QString& reason)
{
	Marker* mk = trial->getMarkers()[marker];
	int K = maxHoldFrames();

	std::vector<int> others;
	for (unsigned int c2 = 0; c2 < Project::getInstance()->getCameras().size(); c2++)
	{
		if ((int)c2 == camera) continue;
		if (!Project::getInstance()->getCameras()[c2]->isVisible()) continue;
		if (!isMeasured(mk, (int)c2, frame) || inGroup((int)c2, marker)) continue;
		if (mk->getStatus2D()[c2][frame] == PREDICTED) continue;
		if (m_holds.find(std::make_pair((int)c2, marker)) != m_holds.end()) continue;
		others.push_back((int)c2);
	}

	std::pair<int, int> key(camera, marker);
	bool isNew = (m_holds.find(key) == m_holds.end());
	HoldInfo& h = m_holds[key];
	if (isNew)
	{
		h.templateFrame = lastSingleBlobFrame(mk, camera, frame, dir);
		h.startFrame = frame;
		h.count = 0;
		h.reason = reason;
	}
	h.count++;

	if (others.empty() || h.count > K)
	{
		int start = h.startFrame;
		int count = h.count;
		m_holds.erase(key);
		int cleared = clearWindow(trial, marker, camera, start, frame);
		log(QString("Merge-aware: frame %1, camera %2: marker %3 stopped, %4%5; %6 unconfirmed point(s) since frame %7 cleared")
			.arg(frame + 1).arg(camera + 1).arg(marker + 1).arg(reason)
			.arg(others.empty() ? QString(" and no other camera can hold it") : QString(" for %1 frames").arg(count))
			.arg(cleared).arg(start + 1));
		return false;
	}

	cv::Point2d base = isMeasured(mk, camera, frame - dir) ? mk->getPoints2D()[camera][frame - dir] : mk->getPoints2D()[camera][frame];
	// carry the held point along with the other markers, so a marker moving with the specimen stays near its blob
	std::map<int, std::vector<bool> >::const_iterator mv = m_medianValid.find(frame);
	if (isMeasured(mk, camera, frame - dir) && mv != m_medianValid.end() && camera < (int)mv->second.size() && mv->second[camera])
		base += m_medianHistory[frame][camera];
	cv::Point2d held(0.0, 0.0);
	for (int c2 : others)
		held += mk->projectOntoEpipolarLine(c2, camera, mk->getPoints2D()[c2][frame], base);
	held *= 1.0 / (double)others.size();

	if (h.count == 1)
		log(QString("Merge-aware: frame %1, camera %2: marker %3 held on its epipolar line, %4")
			.arg(frame + 1).arg(camera + 1).arg(marker + 1).arg(reason));
	else if (m_debug)
		log(QString("  [debug] frame %1 cam %2: marker %3 still held (%4), tracker had (%5,%6)")
			.arg(frame + 1).arg(camera + 1).arg(marker + 1).arg(h.count).arg(mk->getPoints2D()[camera][frame].x, 0, 'f', 1).arg(mk->getPoints2D()[camera][frame].y, 0, 'f', 1));

	if (isEditable(mk, camera, frame))
	{
		mk->setPoint(camera, frame, held.x, held.y, PREDICTED);
		mk->setSize(camera, frame, -1);
	}
	return true;
}

// Trackers that have landed on one blob are not necessarily markers that merged: one of them may have
// jumped onto the other's blob. A jumper is recognised by a sudden move, or by disagreeing with its own
// other camera while the resident agrees. Jumpers are held alone and removed from `members`.
void MergeMonitor::intruders(Trial* trial, int camera, int frame, int dir, std::vector<int>& members)
{
	double T = auditThreshold();
	std::vector<double> disp(members.size(), -1.0);
	std::vector<double> cost(members.size(), -1.0);
	for (size_t i = 0; i < members.size(); i++)
	{
		Marker* mk = trial->getMarkers()[members[i]];
		disp[i] = displacement(mk, camera, frame, dir);
		std::vector<int> clean = cleanCameras(trial, members[i], camera, frame);
		if (!clean.empty())
		{
			double c = 0.0;
			for (int c2 : clean)
			{
				double d = mk->distanceToEpipolarLine(c2, camera, mk->getPoints2D()[c2][frame], mk->getPoints2D()[camera][frame]);
				if (d >= 0.0) c += d;
			}
			cost[i] = c / (double)clean.size();
		}
	}

	std::vector<int> jumpers;
	for (size_t i = 0; i < members.size(); i++)
	{
		Marker* mk = trial->getMarkers()[members[i]];
		double r = radius(mk);
		double othersMax = 0.0;
		bool anyConsistentOther = false;
		for (size_t j = 0; j < members.size(); j++)
		{
			if (j == i) continue;
			if (disp[j] >= 0.0) othersMax = std::max(othersMax, disp[j]);
			if (cost[j] >= 0.0 && cost[j] <= T) anyConsistentOther = true;
		}
		bool jumped = disp[i] > std::max(3.0 * r + 2.0, 2.5 * othersMax + 2.0);
		bool inconsistent = cost[i] > T && anyConsistentOther;
		if (jumped || inconsistent) jumpers.push_back(members[i]);
	}
	if (jumpers.size() >= members.size()) return;   // everyone moved: treat as a genuine merge

	for (int m : jumpers)
	{
		QString who;
		for (int o : members)
			if (o != m) who += (who.isEmpty() ? "" : ", ") + QString::number(o + 1);
		holdSingle(trial, m, camera, frame, dir, QString("its tracker jumped onto marker %1").arg(who));
		members.erase(std::remove(members.begin(), members.end(), m), members.end());
	}
}

// Median displacement of the markers tracked in `camera` this frame (excluding held ones). The specimen
// moves coherently, so a marker whose step differs a lot from this is likely to have left its blob.
bool MergeMonitor::medianMotion(Trial* trial, int camera, int frame, int dir, const std::vector<int>& markers, cv::Point2d& median)
{
	std::vector<double> xs;
	std::vector<double> ys;
	for (int m : markers)
	{
		if (m < 0 || m >= (int)trial->getMarkers().size()) continue;
		Marker* mk = trial->getMarkers()[m];
		if (!isMeasured(mk, camera, frame) || !isMeasured(mk, camera, frame - dir)) continue;
		if (mk->getStatus2D()[camera][frame] != TRACKED) continue;
		if (mk->getStatus2D()[camera][frame - dir] == PREDICTED) continue;
		if (inGroup(camera, m) || m_holds.find(std::make_pair(camera, m)) != m_holds.end()) continue;
		cv::Point2d d = mk->getPoints2D()[camera][frame] - mk->getPoints2D()[camera][frame - dir];
		xs.push_back(d.x);
		ys.push_back(d.y);
	}
	if (xs.size() < 4) return false;
	std::sort(xs.begin(), xs.end());
	std::sort(ys.begin(), ys.end());
	median = cv::Point2d(xs[xs.size() / 2], ys[ys.size() / 2]);
	return true;
}

void MergeMonitor::checkMarkers(Trial* trial, int frame, bool forward, const std::vector<int>& markers)
{
	double T = auditThreshold();
	int dir = forward ? 1 : -1;
	unsigned int nbCams = Project::getInstance()->getCameras().size();
	int nbVisible = 0;
	for (unsigned int c = 0; c < nbCams; c++)
		if (Project::getInstance()->getCameras()[c]->isVisible()) nbVisible++;

	std::vector<bool> haveMedian = m_medianValid[frame];
	std::vector<cv::Point2d> median = m_medianHistory[frame];
	if (haveMedian.size() < nbCams) haveMedian.assign(nbCams, false);
	if (median.size() < nbCams) median.assign(nbCams, cv::Point2d(0, 0));
	const int W = 5;   // frames over which a lagging marker is compared with the others

	for (int m : markers)
	{
		if (m < 0 || m >= (int)trial->getMarkers().size()) continue;
		Marker* mk = trial->getMarkers()[m];
		double r = radius(mk);

		std::vector<int> measured;     // real measurements this frame (tracked, set or manual), outside any group
		std::vector<int> fresh;        // just written by the tracker this frame, outside any group
		std::vector<int> freshAny;     // just written by the tracker this frame, groups included
		int measuredAny = 0;           // including held points and group members
		for (unsigned int c = 0; c < nbCams; c++)
		{
			if (!Project::getInstance()->getCameras()[c]->isVisible()) continue;
			if (!isMeasured(mk, (int)c, frame)) continue;
			measuredAny++;
			markerStatus st = mk->getStatus2D()[c][frame];
			if (st == TRACKED) freshAny.push_back((int)c);
			if (inGroup((int)c, m)) continue;
			if (st == PREDICTED) continue;
			measured.push_back((int)c);
			if (st == TRACKED) fresh.push_back((int)c);
		}

		// a marker seen by one camera only cannot be checked: stop it in this mode
		if (nbVisible >= 2 && measuredAny == 1 && !freshAny.empty())
		{
			for (int c : freshAny)
			{
				mk->reset(c, frame);
				m_holds.erase(std::make_pair(c, m));
			}
			log(QString("Merge-aware: frame %1: marker %2 stopped, it is tracked in one camera only")
				.arg(frame + 1).arg(memberList(trial, std::vector<int>(1, m))));
			continue;
		}
		if (fresh.empty()) continue;

		std::vector<int> suspects;
		std::map<int, QString> why;

		// cameras that were held last frame are on a recovery attempt: the new point must be near the hold
		for (int c : fresh)
		{
			std::map<std::pair<int, int>, HoldInfo>::iterator h = m_holds.find(std::make_pair(c, m));
			if (h == m_holds.end()) continue;
			double jump = displacement(mk, c, frame, dir);
			double allowed = 3.0 * r;
			if (haveMedian[c] && isMeasured(mk, c, frame - dir))
			{
				// the hold was carried with the other markers; the marker should reappear about there
				jump = cv::norm(mk->getPoints2D()[c][frame] - (mk->getPoints2D()[c][frame - dir] + median[c]));
				allowed = std::max(3.0 * r + 2.0, 0.5 * cv::norm(median[c]) + 3.0);
			}
			else
			{
				double otherMove = 0.0;
				for (int c2 : measured)
					if (c2 != c) otherMove = std::max(otherMove, displacement(mk, c2, frame, dir));
				allowed = std::max(3.0 * r, 1.5 * otherMove + 3.0);
			}
			double sz = mk->getSize(c, frame);
			bool blob = (mk->getMethod() == 4) || (sz >= 0.5 * r && sz <= 2.0 * r);
			int onOther = -1;
			for (int o : markers)
			{
				if (o == m || o < 0 || o >= (int)trial->getMarkers().size()) continue;
				Marker* om = trial->getMarkers()[o];
				if (!isMeasured(om, c, frame)) continue;
				if (cv::norm(om->getPoints2D()[c][frame] - mk->getPoints2D()[c][frame]) < 2.0 * std::max(r, radius(om)) + 1.0) { onOther = o; break; }
			}
			if (jump > allowed || !blob || onOther >= 0)
			{
				if (m_debug)
					log(QString("  [debug] frame %1 cam %2: marker %3 recovery rejected, moved %4 px (allowed %5), blob size %6").arg(frame + 1).arg(c + 1).arg(m + 1).arg(jump, 0, 'f', 1).arg(allowed, 0, 'f', 1).arg(sz, 0, 'f', 1));
				suspects.push_back(c);
				why[c] = onOther >= 0 ? QString("it came back on marker %1's blob").arg(onOther + 1) : blob ? QString("it has not come back where it was") : QString("it has not come back on a marker-sized blob");
			}
		}

		// a marker shows a marker-sized blob on almost every frame; background does not
		if (mk->getMethod() != 4)
		{
			for (int c : fresh)
			{
				if (std::find(suspects.begin(), suspects.end(), c) != suspects.end()) continue;
				int looked = 0;
				int odd = 0;
				for (int k = 0; k < 5; k++)
				{
					int fk = frame - k * dir;
					if ((fk - m_runStart) * dir < 0) continue;   // only frames this run produced carry detector sizes
					if (!isMeasured(mk, c, fk) || mk->getStatus2D()[c][fk] == PREDICTED) continue;
					looked++;
					double sz = mk->getSize(c, fk);
					if (!(sz >= 0.5 * r && sz <= 2.0 * r)) odd++;
				}
				if (looked >= 3 && odd >= 3)
				{
					suspects.push_back(c);
					why[c] = QString("no marker-sized blob on %1 of the last %2 frames").arg(odd).arg(looked);
				}
			}
		}

		// a step that disagrees with how the other markers moved in this camera
		for (int c : fresh)
		{
			if (std::find(suspects.begin(), suspects.end(), c) != suspects.end()) continue;
			if (!haveMedian[c]) continue;
			if (!isMeasured(mk, c, frame - dir) || mk->getStatus2D()[c][frame - dir] == PREDICTED) continue;
			cv::Point2d d = mk->getPoints2D()[c][frame] - mk->getPoints2D()[c][frame - dir];
			double dev = cv::norm(d - median[c]);
			double tol = std::max(2.0 * r + 2.0, 0.5 * cv::norm(median[c]) + 2.0);
			// a marker moving independently (tongue, jaw) differs from the others smoothly; a tracker jump is abrupt
			bool abrupt = true;
			if (isMeasured(mk, c, frame - 2 * dir) && mk->getStatus2D()[c][frame - 2 * dir] != PREDICTED && mk->getStatus2D()[c][frame - dir] != PREDICTED)
			{
				cv::Point2d dPrev = mk->getPoints2D()[c][frame - dir] - mk->getPoints2D()[c][frame - 2 * dir];
				abrupt = cv::norm(d - dPrev) > std::max(3.0 * r + 2.0, 0.75 * cv::norm(dPrev) + 3.0);
			}
			if (dev > tol && abrupt)
			{
				suspects.push_back(c);
				why[c] = QString("it moved %1 px against the %2 px the other markers moved").arg(dev, 0, 'f', 1).arg(cv::norm(median[c]), 0, 'f', 1);
			}
		}

		// over several frames, a marker that covers much less ground than the others, or goes the other way,
		// has lost its blob even if each single step looked plausible
		for (int c : fresh)
		{
			if (std::find(suspects.begin(), suspects.end(), c) != suspects.end()) continue;
			cv::Point2d sumMedian(0.0, 0.0);
			bool ok = true;
			for (int k = 0; k < W; k++)
			{
				int fk = frame - k * dir;
				std::map<int, std::vector<bool> >::const_iterator v = m_medianValid.find(fk);
				if (v == m_medianValid.end() || c >= (int)v->second.size() || !v->second[c]) { ok = false; break; }
				sumMedian += m_medianHistory[fk][c];
				if (!isMeasured(mk, c, fk) || mk->getStatus2D()[c][fk] == PREDICTED) { ok = false; break; }
			}
			int f0 = frame - W * dir;
			if (!ok || !isMeasured(mk, c, f0) || mk->getStatus2D()[c][f0] == PREDICTED) continue;
			cv::Point2d own = mk->getPoints2D()[c][frame] - mk->getPoints2D()[c][f0];
			double ref = cv::norm(sumMedian);
			if (ref < 15.0) continue;
			bool tooSlow = cv::norm(own) < 0.4 * ref;
			bool opposite = cv::norm(own) > 12.0 && own.dot(sumMedian) < 0.0;
			if (tooSlow || opposite)
			{
				suspects.push_back(c);
				why[c] = QString("over %1 frames it moved %2 px while the other markers moved %3 px").arg(W).arg(cv::norm(own), 0, 'f', 0).arg(ref, 0, 'f', 0);
			}
		}

		// do the cameras agree
		double worst = 0.0;
		for (size_t a = 0; a < measured.size(); a++)
			for (size_t b = a + 1; b < measured.size(); b++)
			{
				double d = mk->distanceToEpipolarLine(measured[a], measured[b], mk->getPoints2D()[measured[a]][frame], mk->getPoints2D()[measured[b]][frame]);
				worst = std::max(worst, d);
			}
		bool disagree = measured.size() >= 2 && worst > T;
		if (disagree) m_inconsistentNow.insert(m);
		// a one-frame disagreement just over the threshold is usually a lagging tracker; act on it when it is large or repeats
		bool inconsistent = disagree && (worst > 2.0 * T || m_inconsistentLast.find(m) != m_inconsistentLast.end());

		if (suspects.empty() && inconsistent)
		{
			// which camera to doubt: first the one whose blob is not a marker, else the one whose point moved most.
			// If no camera shows a marker-sized blob there is nothing to hold against: stop here.
			int best = -1;
			int good = 0;
			for (int c : fresh)
			{
				if (markerSizedBlob(mk, c, frame)) good++;
				else if (best < 0) best = c;
			}
			if (good == 0 && fresh.size() >= 2)
			{
				int cleared = 0;
				for (int c : fresh)
				{
					std::map<std::pair<int, int>, HoldInfo>::iterator h = m_holds.find(std::make_pair(c, m));
					int start = (h != m_holds.end()) ? h->second.startFrame : frame;
					if (h != m_holds.end()) m_holds.erase(h);
					cleared += clearWindow(trial, m, c, start, frame);
				}
				log(QString("Merge-aware: frame %1: marker %2 stopped, its cameras disagree by %3 px and neither shows a marker-sized blob; %4 point(s) cleared")
					.arg(frame + 1).arg(m + 1).arg(worst, 0, 'f', 1).arg(cleared));
				continue;
			}
			if (best < 0)
			{
				double bestDisp = -1.0;
				for (int c : fresh)
				{
					double disp = displacement(mk, c, frame, dir);
					if (disp < 0.0) disp = 1e9;
					if (disp > bestDisp)
					{
						bestDisp = disp;
						best = c;
					}
				}
			}
			if (best >= 0)
			{
				suspects.push_back(best);
				why[best] = QString("its cameras disagree by %1 px").arg(worst, 0, 'f', 1);
			}
		}

		if (m_debug && (inconsistent || !suspects.empty()))
			log(QString("  [debug] frame %1: marker %2 residual %3 px, suspects %4").arg(frame + 1).arg(m + 1).arg(worst, 0, 'f', 2).arg((int)suspects.size()));

		if (suspects.empty())
		{
			for (int c : fresh)
			{
				std::map<std::pair<int, int>, HoldInfo>::iterator h = m_holds.find(std::make_pair(c, m));
				if (h == m_holds.end()) continue;
				log(QString("Merge-aware: frame %1, camera %2: marker %3 recovered after %4 held frame(s)")
					.arg(frame + 1).arg(c + 1).arg(m + 1).arg(h->second.count));
				m_holds.erase(h);
				m_noVelocity.insert(std::make_pair(c, m));
			}
			continue;
		}

		for (int c : suspects)
			holdSingle(trial, m, c, frame, dir, why[c]);
	}
}
