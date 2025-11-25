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
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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
#include <QStyleFactory>
#include <QSurfaceFormat>

#ifdef Q_OS_MACOS
#include <QOperatingSystemVersion>
#endif

#include <iostream>
#include <opencv2/opencv.hpp>

#if (_MSC_VER >= 1915)
#define no_init_all deprecated
#endif

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
			ErrorDialog::getInstance()->showErrorDialog(QString("OpenCV Error occured. XMALab might be in an unstable state.\nPlease save your work in a NEW xma-file. Restart XMALab and check carefully if the new file and data are ok.\n : Error-Message: \n") + QString::fromStdString(e.err) + QString(" in ") + QString::fromStdString(e.file) + QString(" in function ") + QString::fromStdString(e.func) + QString(" at line ") + QString::number(e.line));
		}
		catch (std::exception& ex)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("Exception occured. XMALab might be in an unstable state.\nPlease save your work in a NEW xma-file. Restart XMALab and check carefully if the new file and data are ok.\n : Error-Message: \n") + QString::fromStdString(ex.what()));
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
	// macOS Tahoe (26) has severe OpenGL issues with Qt's backing store
	// Try to work around by setting the surface format before anything else
	// and avoiding any RHI/compositor interaction with OpenGL
	
	// IMPORTANT: These must be set before QApplication is created
	qputenv("QT_MAC_WANTS_LAYER", "1");  // Use layer-backed views (required on Tahoe)
	qputenv("QSG_RENDER_LOOP", "basic"); // Use basic render loop
	
	// Configure OpenGL 4.1 Core Profile
	QSurfaceFormat format;
	format.setVersion(4, 1);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setSamples(0);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	// Set swap interval to 1 for vsync - helps stability
	format.setSwapInterval(1);
	QSurfaceFormat::setDefaultFormat(format);
	
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	
#ifdef Q_OS_MACOS
	// Disable native dialogs on macOS Sequoia (15.0) and later due to compatibility issues
	auto osVersion = QOperatingSystemVersion::current();
	if (osVersion >= QOperatingSystemVersion::MacOSSequoia) {
		QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
	}
#endif
	MApplication app(argc, argv);

	// Set Fusion style by default for cross-platform theming
	app.setStyle(QStyleFactory::create("Fusion"));

#ifdef _DEBUG
	cv::setBreakOnError(true);
#endif

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

