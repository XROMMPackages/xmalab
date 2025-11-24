#include "ui/quick3d/Quick3DRendererControl.h"

#include <QQuickWindow>
#include <QSGRendererInterface>

namespace xma::quick3d
{
void RendererControl::configureSurface(QQuickWindow* window)
{
    if (!window)
    {
        return;
    }

#ifndef XMA_ENABLE_QRHI_RENDERING
    Q_UNUSED(window);
    return;
#else
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
    Q_UNUSED(window);
    return;
#endif
#if defined(Q_OS_MACOS)
    window->setGraphicsApi(QSGRendererInterface::Metal);
#elif defined(Q_OS_WIN)
    window->setGraphicsApi(QSGRendererInterface::Direct3D11);
#elif defined(Q_OS_LINUX)
    window->setGraphicsApi(QSGRendererInterface::Vulkan);
#else
    window->setGraphicsApi(QSGRendererInterface::OpenGL);
#endif
#endif // XMA_ENABLE_QRHI_RENDERING
}
} // namespace xma::quick3d
