#ifndef MESHSHADER_H
#define MESHSHADER_H

#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

namespace xma
{
    class MeshShader
    {
    public:
        MeshShader();
        ~MeshShader();

        bool init();
        void bind();
        void release();

        void setMVP(const QMatrix4x4& mvp);
        void setColor(const QColor& color, float alpha);

    private:
        QOpenGLShaderProgram* m_program;
        int m_mvpLoc;
        int m_colorLoc;
    };
}

#endif // MESHSHADER_H
