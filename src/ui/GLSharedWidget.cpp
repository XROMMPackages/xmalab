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
///\file GLSharedWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef XMA_USE_PAINTER
#include "GL/glew.h"
#endif

#include "ui/GLSharedWidget.h"

#include <iostream>
#include <stdio.h>

using namespace xma;

GLSharedWidget* GLSharedWidget::instance = NULL;

GLSharedWidget::GLSharedWidget(QWidget* parent)
#ifdef XMA_USE_PAINTER
    : QWidget(parent)
{
    setAutoFillBackground(false);
}
#else
	: QOpenGLWidget(parent), version(0.0f)
{
	setAutoFillBackground(false);
	show();
	initializeGL();
	hide();
}
#endif

GLSharedWidget::~GLSharedWidget()
{
	instance = NULL;
}

GLSharedWidget* GLSharedWidget::getInstance()
{
	if (!instance)
	{
		instance = new GLSharedWidget();
	}
	return instance;
}

//const QGLContext* GLSharedWidget::getQGLContext()
//{
//	return this->context();
//}

//void GLSharedWidget::makeGLCurrent()
//{
	//((QGLContext*) this->context())->makeCurrent();
//}

double GLSharedWidget::getVersion() const
{
#ifdef XMA_USE_PAINTER
	return 0.0;
#else
	return version;
#endif
}

QString GLSharedWidget::getInfo() const
{
#ifdef XMA_USE_PAINTER
	return QStringLiteral("GL info unavailable (stub)");
#else
	return gl_VENDOR + "\n" + gl_RENDERER + "\n" + gl_VERSION + "\n" + gl_SHADING_LANGUAGE_VERSION + "\n" + gl_EXTENSIONS;
#endif
}

#ifndef XMA_USE_PAINTER
void GLSharedWidget::initializeGL()
{
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	gl_VENDOR = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	gl_RENDERER = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	gl_VERSION = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	gl_SHADING_LANGUAGE_VERSION = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
	gl_EXTENSIONS = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));

	QString version_string = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VERSION)), 3);
	version = version_string.toDouble();

	hasBlendSubtract = glewGetExtension("GL_EXT_blend_subtract");
	hasBlendExt = glewGetExtension("GL_EXT_blend_minmax");
}
#endif

bool GLSharedWidget::getHasBlendSubtract() const
{
#ifdef XMA_USE_PAINTER
	return false;
#else
	return hasBlendSubtract;
#endif
}

bool GLSharedWidget::getHasBlendExt() const
{
#ifdef XMA_USE_PAINTER
	return false;
#else
	return hasBlendExt;
#endif
}

