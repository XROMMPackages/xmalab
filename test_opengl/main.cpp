#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QWindow>
#include <QSurfaceFormat>
#include <QDebug>
#include <QTimer>

class OpenGLWindow : public QWindow, protected QOpenGLFunctions {
public:
    OpenGLWindow() {
        setSurfaceType(QWindow::OpenGLSurface);
        
        // Try requesting OpenGL ES instead of desktop OpenGL
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setVersion(3, 0);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        setFormat(format);
        
        m_context = new QOpenGLContext(this);
        m_context->setFormat(format);
        if (!m_context->create()) {
            qDebug() << "Failed to create OpenGL ES context, trying desktop OpenGL";
            format.setRenderableType(QSurfaceFormat::OpenGL);
            format.setVersion(4, 1);
            format.setProfile(QSurfaceFormat::CoreProfile);
            setFormat(format);
            m_context->setFormat(format);
            m_context->create();
        }
        
        qDebug() << "OpenGLWindow created, context valid:" << m_context->isValid();
        qDebug() << "Renderable type:" << m_context->format().renderableType();
    }
    
    void exposeEvent(QExposeEvent *) override {
        if (isExposed()) {
            render();
        }
    }
    
    void render() {
        if (!m_context->makeCurrent(this)) {
            qDebug() << "Failed to make context current";
            return;
        }
        
        if (!m_initialized) {
            initializeOpenGLFunctions();
            qDebug() << "OpenGL Version:" << (const char*)glGetString(GL_VERSION);
            qDebug() << "OpenGL Renderer:" << (const char*)glGetString(GL_RENDERER);
            m_initialized = true;
        }
        
        m_frameCount++;
        qDebug() << "Rendering frame" << m_frameCount;
        
        glViewport(0, 0, width(), height());
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glFinish();  // Wait for all GL commands to complete
        
        qDebug() << "About to swap buffers...";
        m_context->swapBuffers(this);
        qDebug() << "Swap buffers completed";
        
        if (m_frameCount < 10) {
            QTimer::singleShot(100, this, [this]() { render(); });
        } else {
            qDebug() << "10 frames rendered successfully!";
        }
    }
    
private:
    QOpenGLContext *m_context = nullptr;
    bool m_initialized = false;
    int m_frameCount = 0;
};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    OpenGLWindow window;
    window.resize(400, 300);
    window.show();
    
    qDebug() << "Entering event loop...";
    return app.exec();
}
