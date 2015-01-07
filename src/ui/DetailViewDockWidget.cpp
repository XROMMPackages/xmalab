#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Project.h"

#include "ui/MainWindow.h"
#include "ui/DetailViewDockWidget.h"
#include "ui_DetailViewDockWidget.h"
#include "ui/CameraViewDetailWidget.h"

#include "ui/GLSharedWidget.h"

#include <QLabel>

using namespace xma;

DetailViewDockWidget* DetailViewDockWidget::instance = NULL;

DetailViewDockWidget::DetailViewDockWidget(QWidget *parent) :
												QDockWidget(parent),
												dock(new Ui::DetailViewDockWidget){
	dock->setupUi(this);
}

DetailViewDockWidget::~DetailViewDockWidget(){
	clear();
	delete dock;
	instance = NULL;
}

DetailViewDockWidget* DetailViewDockWidget::getInstance()
{
	if(!instance) 
	{
		instance = new DetailViewDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::TopDockWidgetArea, instance);
	}
	return instance;
}


void DetailViewDockWidget::draw()
{
	if (this->isVisible()){
		for (unsigned int i = 0; i < cameraViews.size(); i++) {
			cameraViews[i]->draw();
		}
	}
}

void DetailViewDockWidget::setup()
{
	for (std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
		CameraViewDetailWidget* cam_widget = new CameraViewDetailWidget((*it), this);
		cam_widget->setSharedGLContext(GLSharedWidget::getInstance()->getQGLContext());
		cameraViews.push_back(cam_widget);
	}

	for (int i = 0; i < cameraViews.size(); i++){
		dock->horizontalLayout->addWidget(cameraViews[i]);
	}
}

void DetailViewDockWidget::clear()
{
	for (int i = 0; i < cameraViews.size(); i++){
		dock->horizontalLayout->removeWidget(cameraViews[i]);
	}

	for (int i = 0; i < cameraViews.size(); i++){
		delete cameraViews[i];
	}
	cameraViews.clear();
}