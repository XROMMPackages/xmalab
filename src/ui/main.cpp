#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MainWindow.h"
#include "ui/ErrorDialog.h"

#include "core/Settings.h"

#include <QApplication>
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
    MApplication (int &argc, char ** argv ) :QApplication ( argc, argv ) {};
    ~MApplication() {};
    bool notify(QObject * object, QEvent * event)
    {
        try
        {
            return QApplication::notify(object, event);
        }
        catch( cv::Exception& e ){
			ErrorDialog::getInstance()->showErrorDialog(QString("OpenCV Error occured\n") + QString::fromStdString(e.err) + QString(" in ") + QString::fromStdString(e.file) + QString(" in function ") + QString::fromStdString(e.func) + QString(" at line ") + QString::number(e.line));
		}
		catch (std::exception& ex)
        {
			ErrorDialog::getInstance()->showErrorDialog(QString("Exception occured\n") + QString::fromStdString(ex.what()));
		}
        return false;
    }
};



int main ( int argc, char **argv )
{
	MApplication  app (argc, argv);

	Settings::getInstance();
	MainWindow *widget = MainWindow::getInstance();

	widget->show();

	return app.exec();
}






