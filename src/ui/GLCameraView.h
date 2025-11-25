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
#ifndef XMA_USE_PAINTER
#include <QOpenGLWidget>
#else
#include <QWidget>
#endif

#include <vector>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QImage>
#include <QColor>
#include <QVector>
#include <opencv2/core.hpp>

#ifdef XMA_ENABLE_QRHI_RENDERING
QT_FORWARD_DECLARE_CLASS(QQuickView)
#endif

namespace xma
{
    class Camera;
    class FrameBuffer;
    class DistortionShader;
    class BlendShader;
    class Trial;
#ifdef XMA_ENABLE_QRHI_RENDERING
    class CameraOverlaySceneBridge;
#endif

#ifdef XMA_USE_PAINTER
    // Painter-based implementation (no OpenGL)
    class GLCameraView : public QWidget
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
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
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

        void renderPointText(bool calibration);
        // Overlays draw in image coordinates using the painter's current transform
        void drawUndistortionOverlays(QPainter& p);
        void drawDigitizationOverlays(QPainter& p);
        void drawCalibrationOverlays(QPainter& p);
        void drawRigidBodyMeshes(QPainter& p);
        void invalidateMeshCache();
        bool meshCacheNeedsUpdate(xma::Trial* trial, int frame, bool filteredSetting) const;
        bool rebuildMeshCache(xma::Trial* trial, int frame, bool filteredSetting);
        bool ensureDistortionLookup();
        QImage distortOverlayImage(const QImage& undistortedOverlay);
        QRgb sampleOverlayBilinear(const QImage& src, qreal x, qreal y) const;

    #ifdef XMA_ENABLE_QRHI_RENDERING
        void initializeQuickOverlay();
        void updateQuickOverlayGeometry();
            void updateQuickOverlayVisibility();
    #endif

        double bias;
        double scale;
        double transparency;
        bool renderTransparentModels; 

        struct PainterMeshTriangle
        {
            QPointF points[3];
            double depth;
            QColor color;
        };
        std::vector<PainterMeshTriangle> m_meshTriangles;

        QImage m_meshCacheImage;
        xma::Trial* m_meshCacheTrial;
        int m_meshCacheFrame;
        int m_meshCacheCameraId;
        bool m_meshCacheFilteredSetting;
        QSize m_meshCacheSize;

        cv::Mat m_remapX;
        cv::Mat m_remapY;
        int m_distortionLookupCameraId = -1;
        QSize m_distortionLookupSize;

    #ifdef XMA_ENABLE_QRHI_RENDERING
            CameraOverlaySceneBridge* m_overlayBridge;
            QQuickView* m_overlayView;
            QWidget* m_overlayContainer;
    #endif
    signals:
        void autozoomChanged(bool on);
        void zoomChanged(int zoom);
        void transparencyChanged(double zoom);
    };
#else
    // Original OpenGL-backed implementation on non-macOS
    class GLCameraView : public QOpenGLWidget
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
        inline bool projectTextPos(GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble model[16], const GLdouble proj[16], const GLint viewport[4], GLdouble * winx, GLdouble * winy, GLdouble * winz);
        inline void transformTextPos(GLdouble out[4], const GLdouble m[16], const GLdouble in[4]);
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
    public:
    signals:
        void autozoomChanged(bool on);
        void zoomChanged(int zoom);
        void transparencyChanged(double zoom);

    private:
        GLfloat LightAmbient[4];
        GLfloat LightDiffuse[4];
        GLfloat LightPosition_front[4];
        GLfloat LightPosition_back[4];
    };
#endif
}

#endif /* GLWIDGET_H_ */

