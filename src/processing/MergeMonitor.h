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
///\file MergeMonitor.h
///\brief Resolves markers that overlap in one camera using the other cameras.
///
/// Runs once per frame after the 2D trackers, the detector and the validity check have
/// finished for that frame. See docs/merge-aware-tracking-design.md for the reasoning.
///
/// - Markers whose blobs touch in a camera form a merge group for that camera. While merged,
///   each member's point in that camera is held on the epipolar line of its point in a clean
///   camera (at the rigid-body prediction when the marker belongs to a body) and written as
///   PREDICTED, never TRACKED.
/// - When separate blobs appear, members are assigned to blobs by distance to their epipolar
///   lines plus the rigid-body prediction. A clear margin commits; a small margin keeps the
///   assignment provisional until later frames decide, when provisional frames are swapped if
///   needed. A member that fits nothing is reset, not guessed.
/// - Markers outside any group are audited: if their cameras disagree by more than a threshold
///   the tracked points of that frame are reset, which stops the marker.

#ifndef MERGEMONITOR_H
#define MERGEMONITOR_H

#include <QString>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <opencv2/opencv.hpp>

namespace xma
{
	class Trial;
	class Marker;

	class MergeMonitor
	{
	public:
		static MergeMonitor* getInstance();

		/// Process a frame that has just been tracked, detected and validated for `markers`.
		void processFrame(int trial, int frame, bool forward, const std::vector<int>& markers);

		/// Frame whose image supplies the correlation template for `marker` in `camera` when
		/// tracking away from `frame`. Equals `frame` unless the marker is held in a merge there.
		int templateFrame(int camera, int marker, int frame);

		/// True (with the position at `frameFrom`) when the tracker leaving `frameFrom` should not
		/// extrapolate velocity, because the earlier frames were held positions, not measurements.
		bool predictionOverride(int trial, int camera, int marker, int frameFrom, double& x, double& y);

		/// Console messages produced by the last processFrame call.
		const std::vector<QString>& messages() const
		{
			return m_messages;
		}

		/// Every message since the last reset, for log files.
		const std::vector<QString>& allMessages() const
		{
			return m_allMessages;
		}

		void reset();

	private:
		MergeMonitor();
		static MergeMonitor* instance;

		enum GroupState
		{
			MERGED,
			PENDING
		};

		struct Group
		{
			int camera;
			GroupState state;
			std::vector<int> members;
			std::map<int, int> templateFrame;
			std::vector<int> pendingFrames;
			std::vector<int> heldFrames;
			int startFrame;
		};

		struct Candidate
		{
			cv::Point2d pt;
			double size;
			int fromMember;
		};

		// a single marker whose measurement in one camera is not trusted: held on its epipolar line while the tracker retries
		struct HoldInfo
		{
			int templateFrame;
			int startFrame;
			int count;
			QString reason;
		};

		struct Assignment
		{
			std::vector<int> choice;      // candidate index per member
			std::vector<double> cost;     // assigned cost per member
			std::vector<double> margin;   // per member: best alternative cost minus assigned cost
			std::vector<bool> evidence;   // per member: had any epipolar or rigid-body evidence
			double minMargin;
		};

		std::vector<Group> m_groups;
		std::vector<QString> m_messages;
		std::vector<QString> m_allMessages;
		std::set<std::pair<int, int> > m_noVelocity;   // (camera, marker) released from a hold at m_lastFrame
		std::map<std::pair<int, int>, HoldInfo> m_holds;   // (camera, marker) currently held outside any group
		std::map<int, std::vector<cv::Point2d> > m_medianHistory;   // frame -> per-camera median step of the tracked markers
		std::map<int, std::vector<bool> > m_medianValid;
		std::set<int> m_inconsistentLast;   // markers whose cameras disagreed on the previous frame
		std::set<int> m_inconsistentNow;
		std::vector<int> m_currentMarkers;
		int m_lastTrial;
		int m_lastFrame;
		int m_runStart;   // first frame of the current uninterrupted run
		bool m_lastForward;
		bool m_debug;

		double epiThreshold();
		double auditThreshold();
		double maxCost();
		int maxPendingFrames();
		int maxHoldFrames();
		int maxOverlapFrames();

		double radius(Marker* m);
		bool singleBlob(Marker* m, double size);
		double mergeDistance(Trial* trial, const std::vector<int>& members);
		bool isMeasured(Marker* m, int camera, int frame);
		bool isEditable(Marker* m, int camera, int frame);
		bool inGroup(int camera, int marker) const;
		std::vector<int> cleanCameras(Trial* trial, int marker, int camera, int frame);
		bool rigidBodyPrediction(Trial* trial, int marker, const std::vector<int>& exclude, int frame, int camera, cv::Point2d& projected);
		cv::Point2d holdPosition(Trial* trial, Group& g, int marker, int frame, const std::vector<int>& clean, bool& usedBody);

		void detectNewGroups(Trial* trial, int camera, int frame, bool forward, const std::vector<int>& markers);
		void updateGroup(Trial* trial, Group& g, int frame);
		void holdMember(Trial* trial, Group& g, int marker, int frame);
		std::vector<Candidate> candidates(Trial* trial, Group& g, int frame, bool useDetector, const std::vector<int>& markers);
		bool assign(Trial* trial, Group& g, int frame, const std::vector<Candidate>& cands, Assignment& result);
		void placeMember(Trial* trial, Group& g, int member, int frame, const cv::Point2d& pt, double size, bool tracked);
		void commitPending(Trial* trial, Group& g, int frame, const std::vector<int>& order);
		void dropMember(Trial* trial, Group& g, int member, int frame, const QString& why);
		void stopGroup(Trial* trial, Group& g, int frame, const QString& why);
		void checkMarkers(Trial* trial, int frame, bool forward, const std::vector<int>& markers);
		int lastSingleBlobFrame(Marker* mk, int camera, int frame, int dir);
		double displacement(Marker* mk, int camera, int frame, int dir);
		bool holdSingle(Trial* trial, int marker, int camera, int frame, int dir, const QString& reason);
		// Reset a marker's unconfirmed frames in `camera` from `from` to `to`; with two visible cameras the other camera's
		// tracked frames in that window were never confirmed either and go too. Returns how many points were cleared.
		int clearWindow(Trial* trial, int marker, int camera, int from, int to);
		bool markerSizedBlob(Marker* mk, int camera, int frame);
		void intruders(Trial* trial, int camera, int frame, int dir, std::vector<int>& members);
		bool medianMotion(Trial* trial, int camera, int frame, int dir, const std::vector<int>& markers, cv::Point2d& median);

		QString memberList(Trial* trial, const std::vector<int>& members);
		void log(const QString& s)
		{
			m_messages.push_back(s);
		}
	};
}

#endif // MERGEMONITOR_H
