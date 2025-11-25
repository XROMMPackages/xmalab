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
///\file GLSharedWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef GLSHAREDWIDGET_H
#define GLSHAREDWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include "gl/GLFunctions.h"

namespace xma
{
	class GLSharedWidget : public QOpenGLWidget, protected GLFunctions
	{
		Q_OBJECT

	public:
		~GLSharedWidget() override;
		static GLSharedWidget* getInstance();
		double getVersion() const;
		QString getInfo() const;
	public slots:
		bool getHasBlendExt() const;
		bool getHasBlendSubtract() const;

	protected:
		explicit GLSharedWidget(QWidget* parent = nullptr);
		void initializeGL() override;

		void paintEvent(QPaintEvent* event) override {}
		void resizeEvent(QResizeEvent* event) override {}

	private:
		static GLSharedWidget* instance;

		bool hasBlendSubtract;
		bool hasBlendExt;
		double version;

		QString gl_VENDOR;
		QString gl_RENDERER;
		QString gl_VERSION;
		QString gl_SHADING_LANGUAGE_VERSION;
		QString gl_EXTENSIONS;

	};
}


#endif /* GLSHAREDWIDGET_H */

