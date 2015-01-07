#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ConsoleDockWidget.h"
#include "ui_ConsoleDockWidget.h"
#include "ui/MainWindow.h"

#include <QLabel>
#include <QColor>
#include <QFile>
#include <QTextStream>
#include <QScrollbar>

using namespace xma;

ConsoleDockWidget* ConsoleDockWidget::instance = NULL;

ConsoleDockWidget::ConsoleDockWidget(QWidget *parent) :
												QDockWidget(parent),
												dock(new Ui::ConsoleDockWidget){

	dock->setupUi(this);
	//memset(errorBuffer, 0, sizeof(errorBuffer));  
    //setvbuf(stderr, errorBuffer, _IOLBF, sizeof(errorBuffer));
	//memset(outputBuffer, 0, sizeof(outputBuffer));  
    //setvbuf(stdout, outputBuffer, _IOLBF, sizeof(outputBuffer));
 
    // set up QTimer to call logErrors periodically
    timer = new QTimer(this);
    timer->start(500);
    connect(timer, SIGNAL(timeout()), this, SLOT(logTimer()));

	LoadText = "";
}


ConsoleDockWidget::~ConsoleDockWidget(){
	delete dock;
	instance = NULL;
}

void ConsoleDockWidget::clear(){
	mutex.lock();
	dock->console->clear();
	dock->console->verticalScrollBar()->setValue(0);
	mutex.unlock();
}

void ConsoleDockWidget::prepareSave(){
	mutex.lock();
	LoadText = dock->console->toHtml();
	mutex.unlock();
}

void ConsoleDockWidget::save(QString filename){
	QFile file(filename);
	if (file.open(QIODevice::ReadWrite)) {
		QTextStream stream(&file); 
		stream << LoadText;
		file.flush();
		file.close();
	}
}

void ConsoleDockWidget::afterLoad(){
	mutex.lock();
	dock->console->clear();
	if(!LoadText.isEmpty())dock->console->setHtml(LoadText);
	dock->console->verticalScrollBar()->setValue(dock->console->verticalScrollBar()->maximum());
	mutex.unlock();
}

void ConsoleDockWidget::load(QString filename){
	QFile file(filename);
	if(file.exists()){
		file.open(QFile::ReadOnly | QFile::Text);

		QTextStream ReadFile(&file);
		LoadText = ReadFile.readAll();
		file.close();
	}
}

ConsoleDockWidget* ConsoleDockWidget::getInstance()
{
	if(!instance) 
	{
		instance = new ConsoleDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::BottomDockWidgetArea, instance);
	}
	return instance;
}

void ConsoleDockWidget::writeLog(QString message,unsigned int level)
{
	mutex.lock();
	switch(level){
		default:
		case 0:
			dock->console->setTextColor(QColor::fromRgb(0,0,0));
			break;
		case 1:
			dock->console->setTextColor(QColor::fromRgb(0,100,0));
			break;
		case 2:
			dock->console->setTextColor(QColor::fromRgb(0,0,100));
			break;
		case 3:
			dock->console->setTextColor(QColor::fromRgb(100,0,0));
			break;
	}
	
	dock->console->append(message);
	dock->console->verticalScrollBar()->setValue(dock->console->verticalScrollBar()->maximum());
	mutex.unlock();
}

void ConsoleDockWidget::logTimer()
{
    fflush(stderr);
 
    // if there is stuff in the buffer, send it as errorMessage signal and clear the buffer
    if(strlen(errorBuffer) > 0){
		QString tmp = errorBuffer;
		memset(errorBuffer, 0, sizeof(errorBuffer));
		writeLog(tmp,3);  
    }

	fflush(stdout);
 
    // if there is stuff in the buffer, send it as errorMessage signal and clear the buffer
    if(strlen(outputBuffer) > 0){
		QString tmp = outputBuffer;
		memset(outputBuffer, 0, sizeof(outputBuffer));
		writeLog(tmp,2);
    }
}
