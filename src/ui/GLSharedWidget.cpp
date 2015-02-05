#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include "GL/glew.h"

#include "ui/GLSharedWidget.h"

#include <iostream>
#include <stdio.h>

using namespace xma;

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

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	hasBlendSubtract = glewGetExtension("GL_EXT_blend_subtract");
	hasBlendExt = glewGetExtension("GL_EXT_blend_minmax");
}

bool GLSharedWidget::getHasBlendSubtract()
{
	return hasBlendSubtract;
}

bool GLSharedWidget::getHasBlendExt()
{
	return hasBlendExt;
}
