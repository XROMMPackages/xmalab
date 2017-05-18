//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
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
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file main.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MainWindow.h"
#include "ui/ErrorDialog.h"

#include "core/Settings.h"

#include <QApplication>
#include <QFileOpenEvent>
#include <QPushButton>

#include <iostream>
#include <opencv/cv.h>

#ifdef _MSC_VER 
#ifndef WITH_CONSOLE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#endif

using namespace xma;

class MApplication : public QApplication
{
public:
	MApplication(int& argc, char** argv) : QApplication(argc, argv)
	{
	};

	~MApplication()
	{
	};

	bool notify(QObject* object, QEvent* event)
	{
		try
		{
			return QApplication::notify(object, event);
		}
		catch (cv::Exception& e)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("OpenCV Error occured\n") + QString::fromStdString(e.err) + QString(" in ") + QString::fromStdString(e.file) + QString(" in function ") + QString::fromStdString(e.func) + QString(" at line ") + QString::number(e.line));
		}
		catch (std::exception& ex)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("Exception occured\n") + QString::fromStdString(ex.what()));
		}
		return false;
	}
    
    // responds to FileOpenEvent specific for mac
    bool event(QEvent *event)
    {
        
        switch(event->type())
        {
            case QEvent::FileOpen:
            {
                QFileOpenEvent * fileOpenEvent = static_cast<QFileOpenEvent *>(event);
                if(fileOpenEvent)
                {
                    QString m_macFileOpenOnStart = fileOpenEvent->file();
                   
					if(!m_macFileOpenOnStart.isEmpty())
                    {
						MainWindow::getInstance()->loadProjectFromEvent(m_macFileOpenOnStart);  // open file in existing window
                        return true;
                    }
                }
            }
            default:
                return QApplication::event(event);
        }
        return QApplication::event(event);
    }
};


int main(int argc, char** argv)
{
	MApplication app(argc, argv);

	Settings::getInstance();
	MainWindow* widget = MainWindow::getInstance();
	if (argc > 1)
	{
		widget->showAndLoad(argv[1]);
	}
	else{
		widget->show();
	}
	return app.exec();
}

