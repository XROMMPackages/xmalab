#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "GLSharedWidget.h"

#include <iostream>

GLSharedWidget* GLSharedWidget::instance = NULL;

GLSharedWidget::GLSharedWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    setAutoFillBackground(false);
	makeCurrent();
	initializeGL();
}

GLSharedWidget::~GLSharedWidget(){
	instance = NULL;
}

GLSharedWidget* GLSharedWidget::getInstance()
{
	if(!instance) 
	{
		instance = new GLSharedWidget();
	}
	return instance;
}

const QGLContext* GLSharedWidget::getQGLContext(){
	return this->context();
}

void GLSharedWidget::makeGLCurrent(){
	((QGLContext*) this->context())->makeCurrent();
}

void GLSharedWidget::initializeGL(){
	std::cout << "Graphics Card Vendor"<< glGetString(GL_VENDOR)   << std::endl;
	std::cout << "Renderer"<<  glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version"<<  glGetString(GL_VERSION)  << std::endl;
}

