//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//  ----------------------------------
//
///\file SimpleColorShader.cpp
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#include "gl/SimpleColorShader.h"
#include <iostream>

using namespace xma;

static const char* vertexShaderSource = R"(
#version 410 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 410 core

uniform vec4 u_color;

out vec4 fragColor;

void main()
{
    fragColor = u_color;
}
)";

static const char* shaderName = "SimpleColorShader";

SimpleColorShader::SimpleColorShader()
    : m_lineVAO(0)
    , m_lineVBO(0)
    , m_lineVAOInitialized(false)
    , m_mvpLocation(-1)
    , m_colorLocation(-1)
{
    // Set shader sources for base class compilation
    m_shader = shaderName;
    m_vertexShader = vertexShaderSource;
    m_fragmentShader = fragmentShaderSource;
}

SimpleColorShader::~SimpleColorShader()
{
    if (m_lineVAOInitialized) {
        glDeleteVertexArrays(1, &m_lineVAO);
        glDeleteBuffers(1, &m_lineVBO);
    }
}

void SimpleColorShader::initShader()
{
    if (!initGLFunctions()) {
        std::cerr << "Failed to initialize GL functions for SimpleColorShader" << std::endl;
        return;
    }
    
    // Force shader compilation
    bindProgram();

    m_mvpLocation = glGetUniformLocation(getProgram(), "u_mvp");
    m_colorLocation = glGetUniformLocation(getProgram(), "u_color");
    
    unbindProgram();
}

void SimpleColorShader::initLineVAO()
{
    if (m_lineVAOInitialized) return;

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    
    // Allocate space for dynamic line data (will be updated per draw call)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_lineVAOInitialized = true;
}

void SimpleColorShader::bind()
{
    if (m_mvpLocation < 0) {
        initShader();
    }
    bindProgram();
}

void SimpleColorShader::release()
{
    unbindProgram();
}

void SimpleColorShader::setMVP(const QMatrix4x4& mvp)
{
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, mvp.constData());
}

void SimpleColorShader::setColor(float r, float g, float b, float a)
{
    glUniform4f(m_colorLocation, r, g, b, a);
}

void SimpleColorShader::setColor(const QVector4D& color)
{
    glUniform4f(m_colorLocation, color.x(), color.y(), color.z(), color.w());
}

void SimpleColorShader::drawLine(const QVector3D& start, const QVector3D& end)
{
    initLineVAO();

    float vertices[6] = {
        start.x(), start.y(), start.z(),
        end.x(), end.y(), end.z()
    };

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawArrays(GL_LINES, 0, 2);
    
    glBindVertexArray(0);
}

void SimpleColorShader::drawLines(const std::vector<QVector3D>& points)
{
    if (points.size() < 2) return;
    
    initLineVAO();

    std::vector<float> vertices;
    vertices.reserve(points.size() * 3);
    for (const auto& p : points) {
        vertices.push_back(p.x());
        vertices.push_back(p.y());
        vertices.push_back(p.z());
    }

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    
    // Resize buffer if needed
    if (vertices.size() * sizeof(float) > 1024 * 3 * sizeof(float)) {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    }
    
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(points.size()));
    
    glBindVertexArray(0);
}

void SimpleColorShader::drawLineLoop(const std::vector<QVector3D>& points)
{
    if (points.size() < 2) return;
    
    initLineVAO();

    std::vector<float> vertices;
    vertices.reserve(points.size() * 3);
    for (const auto& p : points) {
        vertices.push_back(p.x());
        vertices.push_back(p.y());
        vertices.push_back(p.z());
    }

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    
    if (vertices.size() * sizeof(float) > 1024 * 3 * sizeof(float)) {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    }
    
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(points.size()));
    
    glBindVertexArray(0);
}

void SimpleColorShader::drawPoint(const QVector3D& point, float size)
{
    initLineVAO();

    float vertices[3] = { point.x(), point.y(), point.z() };

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, 1);
    
    glBindVertexArray(0);
}
