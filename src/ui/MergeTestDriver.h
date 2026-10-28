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
///\file MergeTestDriver.h
///\brief Command-line driver for merge-aware tracking: `XMALab --merge-test project.xma start end output.xma`.
///
/// Loads the project as the GUI would, clears every marker from frame start+1 to end (0-based frame
/// indices, as in the saved CSV files), runs "merge-aware all" forward from start until end, saves the
/// result to output.xma and writes output.xma.log with the MergeMonitor messages. Used to measure the
/// tracker against a reference project without clicking through the GUI.

#ifndef MERGETESTDRIVER_H
#define MERGETESTDRIVER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>

namespace xma
{
	class MergeTestDriver : public QObject
	{
		Q_OBJECT

	public:
		MergeTestDriver(const QString& project, int startFrame, int endFrame, const QString& output, bool plain2D = false, QObject* parent = 0);

	private slots:
		void poll();

	private:
		enum Phase
		{
			LOADING,
			TRACKING,
			STOPPING,
			DONE
		};

		void startTracking();
		void finish(const QString& why);

		Phase m_phase;
		QTimer m_timer;
		QElapsedTimer m_clock;
		QString m_project;
		QString m_output;
		int m_start;
		int m_end;
		int m_lastFrameSeen;
		qint64 m_lastProgressMs;
		qint64 m_settleMs;
		int m_retries;
		bool m_plain2D;
	};
}

#endif // MERGETESTDRIVER_H
