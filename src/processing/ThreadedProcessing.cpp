//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file ThreadedProcessing.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/ThreadedProcessing.h" 

#include "ui/ProgressDialog.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>

using namespace xma;

int ThreadedProcessing::nbInstances = 0;

ThreadedProcessing::ThreadedProcessing(QString _message) : QObject()
{
	nbInstances++;

	message = _message;
}

ThreadedProcessing::~ThreadedProcessing()
{
}

void ThreadedProcessing::start()
{
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(thread_complete()));

	QFuture<void> future = QtConcurrent::run([this]() { thread(); });
	m_FutureWatcher->setFuture(future);

	if (!message.isEmpty()) ProgressDialog::getInstance()->showProgressbar(0, 0, message.toUtf8());
}

void ThreadedProcessing::thread()
{
	process();
}

void ThreadedProcessing::thread_complete()
{
	process_finished();

	delete m_FutureWatcher;
	nbInstances--;

	if (nbInstances == 0)
	{
		if (!message.isEmpty())ProgressDialog::getInstance()->closeProgressbar();
		emit signal_finished();
	}
	delete this;
}

