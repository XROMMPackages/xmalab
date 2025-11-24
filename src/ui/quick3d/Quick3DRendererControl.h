#pragma once

class QQuickWindow;

namespace xma::quick3d
{
class RendererControl
{
public:
    static void configureSurface(QQuickWindow* window);
};
}
