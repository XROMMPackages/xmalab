#include "gl/MeshShader.h"
#include <iostream>

using namespace xma;

MeshShader::MeshShader() : m_program(nullptr), m_mvpLoc(-1), m_colorLoc(-1)
{
}

MeshShader::~MeshShader()
{
    if (m_program) delete m_program;
}

bool MeshShader::init()
{
    m_program = new QOpenGLShaderProgram();

    // Vertex Shader
    const char* vshader = 
        "#version 150\n"
        "in vec3 vertex;\n"
        "in vec3 normal;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "   gl_Position = mvp * vec4(vertex, 1.0);\n"
        "}\n";

    // Fragment Shader
    const char* fshader = 
        "#version 150\n"
        "uniform vec4 color;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   fragColor = color;\n"
        "}\n";

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vshader)) {
        std::cerr << "Vertex shader error: " << m_program->log().toStdString() << std::endl;
        return false;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fshader)) {
        std::cerr << "Fragment shader error: " << m_program->log().toStdString() << std::endl;
        return false;
    }

    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);

    if (!m_program->link()) {
        std::cerr << "Shader link error: " << m_program->log().toStdString() << std::endl;
        return false;
    }

    m_mvpLoc = m_program->uniformLocation("mvp");
    m_colorLoc = m_program->uniformLocation("color");

    return true;
}

void MeshShader::bind()
{
    if (m_program) m_program->bind();
}

void MeshShader::release()
{
    if (m_program) m_program->release();
}

void MeshShader::setMVP(const QMatrix4x4& mvp)
{
    if (m_program) m_program->setUniformValue(m_mvpLoc, mvp);
}

void MeshShader::setColor(const QColor& color, float alpha)
{
    if (m_program) {
        QVector4D c(color.redF(), color.greenF(), color.blueF(), alpha);
        m_program->setUniformValue(m_colorLoc, c);
    }
}
