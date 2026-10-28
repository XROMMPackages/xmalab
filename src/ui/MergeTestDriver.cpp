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
///\file MergeTestDriver.cpp

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MergeTestDriver.h"
#include "ui/State.h"
#include "ui/WizardDockWidget.h"
#include "ui/WizardDigitizationFrame.h"
#include "ui/ProjectFileIO.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "processing/MarkerTracking.h"
#include "processing/MarkerDetection.h"
#include "processing/MergeMonitor.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include <cstdlib>

using namespace xma;

MergeTestDriver::MergeTestDriver(const QString& project, int startFrame, int endFrame, const QString& output, bool plain2D, QObject* parent)
	: QObject(parent)
	, m_phase(LOADING)
	, m_project(project)
	, m_output(output)
	, m_start(startFrame)
	, m_end(endFrame)
	, m_lastFrameSeen(-1)
	, m_lastProgressMs(0)
	, m_settleMs(-1)
	, m_retries(0)
	, m_plain2D(plain2D)
{
	m_clock.start();
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(poll()));
	m_timer.start(200);
}

void MergeTestDriver::startTracking()
{
	Trial* trial = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()];
	int nbFrames = trial->getEndFrame();
	if (m_end >= nbFrames) m_end = nbFrames - 1;
	if (m_start < 1) m_start = 1;
	if (m_start >= m_end)
	{
		finish("start frame must be before end frame");
		return;
	}

	trial->setActiveFrame(m_start);
	State::getInstance()->changeActiveFrameTrial(m_start, true);
	QApplication::processEvents();
	if (State::getInstance()->getActiveFrameTrial() != m_start || trial->getActiveFrame() != m_start)
	{
		std::cerr << "merge-test: frame did not take (state " << State::getInstance()->getActiveFrameTrial() << ", trial " << trial->getActiveFrame() << "), retrying" << std::endl;
		m_retries++;
		if (m_retries > 20) finish("could not set the start frame");
		return;
	}

	// clear what a tracker wrote, keep the user's set and manual points as anchors, exactly as the GUI's track-all does
	for (unsigned int j = 0; j < trial->getMarkers().size(); j++)
	{
		Marker* mk = trial->getMarkers()[j];
		for (unsigned int c = 0; c < mk->getStatus2D().size(); c++)
			for (int f = m_start + 1; f <= m_end; f++)
				if (mk->getStatus2D()[c][f] > UNDEFINED && mk->getStatus2D()[c][f] <= TRACKED_AND_OPTIMIZED) mk->reset((int)c, f);
	}
	State::getInstance()->changeActiveFrameTrial(m_start, true);
	QApplication::processEvents();
	std::cerr << "merge-test: tracking frames " << m_start << " -> " << m_end << std::endl;
	m_lastProgressMs = m_clock.elapsed();
	if (m_plain2D)
		WizardDockWidget::getInstance()->getDigitizationFrame()->startPlainTracking(true);
	else
		WizardDockWidget::getInstance()->getDigitizationFrame()->startMergeTracking(true, true);
	m_phase = TRACKING;
}

void MergeTestDriver::finish(const QString& why)
{
	m_timer.stop();
	m_phase = DONE;
	std::cerr << "merge-test: " << why.toStdString() << std::endl;

	if (Project::getInstance()->getTrials().size() > 0)
	{
		int rc = ProjectFileIO::getInstance()->saveProject(m_output, Project::getInstance()->getTrials(), false);
		std::cerr << "merge-test: saved " << m_output.toStdString() << " (rc " << rc << ")" << std::endl;
	}

	QFile log(m_output + ".log");
	if (log.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&log);
		out << "project " << m_project << "\n";
		out << "frames " << m_start << " -> " << m_end << " (0-based)\n";
		out << "result " << why << "\n";
		out << "elapsed_ms " << m_clock.elapsed() << "\n";
		const std::vector<QString>& msgs = MergeMonitor::getInstance()->allMessages();
		for (unsigned int i = 0; i < msgs.size(); i++) out << msgs[i] << "\n";
		log.close();
	}

	// a plain quit would raise the main window's save confirmation
	std::exit(0);
}

void MergeTestDriver::poll()
{
	if (m_phase == LOADING)
	{
		if (m_clock.elapsed() > 180000)
		{
			finish("timed out waiting for the project to load");
			return;
		}
		if (State::getInstance()->isLoading()) return;
		if (Project::getInstance()->getTrials().size() == 0) return;
		if (State::getInstance()->getWorkspace() != DIGITIZATION) return;
		if (State::getInstance()->getActiveTrial() < 0) return;
		if (m_settleMs < 0) m_settleMs = m_clock.elapsed();
		if (m_clock.elapsed() - m_settleMs < 2000) return;   // let queued UI updates settle
		startTracking();
		return;
	}

	if (m_phase == TRACKING)
	{
		WizardDigitizationFrame* wizard = WizardDockWidget::getInstance()->getDigitizationFrame();
		int frame = State::getInstance()->getActiveFrameTrial();
		if (frame != m_lastFrameSeen)
		{
			m_lastFrameSeen = frame;
			m_lastProgressMs = m_clock.elapsed();
		}
		if (frame < m_start - 1)
		{
			if (wizard->isTracking()) wizard->stopTracking();
			finish(QString("ERROR: tracking ran at frame %1, below the start frame").arg(frame));
			return;
		}
		bool reachedEnd = frame >= m_end;
		bool stalled = (m_clock.elapsed() - m_lastProgressMs) > 60000;
		if (reachedEnd || !wizard->isTracking() || stalled)
		{
			if (wizard->isTracking()) wizard->stopTracking();
			m_phase = STOPPING;
			m_lastProgressMs = m_clock.elapsed();
		}
		return;
	}

	if (m_phase == STOPPING)
	{
		WizardDigitizationFrame* wizard = WizardDockWidget::getInstance()->getDigitizationFrame();
		bool busy = wizard->isTracking() || MarkerTracking::isRunning() || MarkerDetection::isRunning();
		if (!busy || (m_clock.elapsed() - m_lastProgressMs) > 30000)
		{
			finish(QString("finished at frame %1").arg(State::getInstance()->getActiveFrameTrial()));
		}
	}
}
