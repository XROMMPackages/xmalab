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

#include "ui/GLSharedWidget.h"

#include <iostream>
#include <stdio.h>

using namespace xma;

GLSharedWidget* GLSharedWidget::instance = NULL;

GLSharedWidget::GLSharedWidget(QWidget* parent)
	: QOpenGLWidget(parent), version(0.0f), hasBlendSubtract(false), hasBlendExt(false)
{
	setAutoFillBackground(false);
	
	// Set format explicitly for this widget
	QSurfaceFormat format;
	format.setVersion(4, 1);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setSamples(0);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	setFormat(format);
	
	// Don't show() or initialize here - let caller do it when ready
	// This prevents crashes during early initialization
}

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
	return version;
}

QString GLSharedWidget::getInfo() const
{
	return gl_VENDOR + "\n" + gl_RENDERER + "\n" + gl_VERSION + "\n" + gl_SHADING_LANGUAGE_VERSION + "\n" + gl_EXTENSIONS;
}


void GLSharedWidget::initializeGL()
{
	// Initialize OpenGL 4.1 Core functions using the base class helper
	if (!initGLFunctions()) {
		fprintf(stderr, "Error: Failed to initialize OpenGL 4.1 Core functions\n");
		return;
	}

	gl_VENDOR = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	gl_RENDERER = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	gl_VERSION = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	gl_SHADING_LANGUAGE_VERSION = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
	// GL_EXTENSIONS is deprecated in core profile, use glGetStringi instead
	gl_EXTENSIONS = QString();

	QString version_string = QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_VERSION)), 3);
	version = version_string.toDouble();

	// In OpenGL 4.1 Core, these blend operations are always available
	hasBlendSubtract = true;
	hasBlendExt = true;
}

bool GLSharedWidget::getHasBlendSubtract() const
{
	return hasBlendSubtract;
}

bool GLSharedWidget::getHasBlendExt() const
{
	return hasBlendExt;
}

