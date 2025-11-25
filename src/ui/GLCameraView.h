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
///\file GLCameraView.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QMatrix4x4>

namespace xma
{
	class Camera;
	class FrameBuffer;
	class DistortionShader;
	class BlendShader;
	class TexturedQuadShader;
	
	class GLCameraView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core
	{
		Q_OBJECT

	public:
		explicit GLCameraView(QWidget* parent = nullptr);
		~GLCameraView() override;

		void setCamera(Camera* _camera);

		void setMinimumWidthGL(bool set);
		void setAutoZoom(bool on);
		void setZoom(int value);

		void setDetailedView();

		void setScale(double value);
		void setBias(double value);
		void setTransparency(double value);
		void setRenderTransparentModels(bool value);
		void centerViewToPoint(bool resetZoom = false);
		void UseStatusColors(bool value);

	protected:
		void paintGL() override;
		void initializeGL() override;
		void resizeGL(int w, int h) override;

		void mouseMoveEvent(QMouseEvent* e) override;
		void mousePressEvent(QMouseEvent* e) override;
		void wheelEvent(QWheelEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;

		void setZoomToFit();
		void setZoomTo100();


	private:
		bool detailedView;
		Camera* camera;
		void clampXY();

		int window_width, window_height;
		int camera_width, camera_height;
		double x_offset, y_offset;
		double prev_x, prev_y;

		double zoomRatio;
		bool autozoom;
		bool showStatusColors;

		void setZoomRatio(double newZoomRation, bool autozoom = false);

		void renderTextCentered(QString string);
		bool projectTextPos(double objx, double objy, double objz, const QMatrix4x4& mvp, const int viewport[4], double* winx, double* winy, double* winz);
		void renderText(double x, double y, double z, const QString &str, QColor fontColor, const QFont & font = QFont());
		void renderPointText(bool calibration);
		void drawTexture();
		void drawQuad();

		double x_test, y_test;
		double bias;
		double scale;
		double transparency;
		bool renderTransparentModels; 

		BlendShader* blendShader;
		DistortionShader * distortionShader;
		FrameBuffer * rigidbodyBufferUndistorted;
		bool doDistortion;
		bool renderMeshes;
		
		// Modern OpenGL state
		QMatrix4x4 m_projection;
		QMatrix4x4 m_view;
		QMatrix4x4 m_model;
		
		// Quad VAO for drawing textured quads
		GLuint m_quadVAO;
		GLuint m_quadVBO;
		bool m_quadInitialized;
		void initQuadVAO();
		
	public:
		signals :

		void autozoomChanged(bool on);
		void zoomChanged(int zoom);
		void transparencyChanged(double zoom);

	private:
		float LightAmbient[4];
		float LightDiffuse[4];
		float LightPosition_front[4];
		float LightPosition_back[4];
	};
}

#endif /* PROGRESSDIALOG_H_ */

