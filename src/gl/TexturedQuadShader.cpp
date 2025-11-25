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
///\file TexturedQuadShader.cpp
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#include "gl/TexturedQuadShader.h"
#include <iostream>

using namespace xma;

static const char* vertexShaderSource = R"(
#version 410 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

uniform mat4 u_mvp;

out vec2 v_texCoord;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 410 core

in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_scale;
uniform float u_bias;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture(u_texture, v_texCoord);
    // Apply scale and bias for brightness/contrast
    fragColor = vec4(texColor.rgb * u_scale + u_bias, texColor.a);
}
)";

static const char* shaderName = "TexturedQuadShader";

TexturedQuadShader::TexturedQuadShader()
    : m_vao(0)
    , m_vbo(0)
    , m_vaoInitialized(false)
    , m_mvpLocation(-1)
    , m_textureLocation(-1)
    , m_scaleLocation(-1)
    , m_biasLocation(-1)
{
    // Set shader sources for base class compilation
    m_shader = shaderName;
    m_vertexShader = vertexShaderSource;
    m_fragmentShader = fragmentShaderSource;
}

TexturedQuadShader::~TexturedQuadShader()
{
    if (m_vaoInitialized) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
    }
}

void TexturedQuadShader::initShader()
{
    if (!initGLFunctions()) {
        std::cerr << "Failed to initialize GL functions for TexturedQuadShader" << std::endl;
        return;
    }
    
    // Force shader compilation
    bindProgram();

    m_mvpLocation = glGetUniformLocation(getProgram(), "u_mvp");
    m_textureLocation = glGetUniformLocation(getProgram(), "u_texture");
    m_scaleLocation = glGetUniformLocation(getProgram(), "u_scale");
    m_biasLocation = glGetUniformLocation(getProgram(), "u_bias");

    // Set default values
    setScaleBias(1.0f, 0.0f);
    
    unbindProgram();
}

void TexturedQuadShader::initQuadVAO()
{
    if (m_vaoInitialized) return;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // Quad data: position (x, y) + texCoord (u, v)
    // Will be updated per draw call
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_vaoInitialized = true;
}

void TexturedQuadShader::bind()
{
    if (m_mvpLocation < 0) {
        initShader();
    }
    bindProgram();
}

void TexturedQuadShader::release()
{
    unbindProgram();
}

void TexturedQuadShader::setMVP(const QMatrix4x4& mvp)
{
    glUniformMatrix4fv(m_mvpLocation, 1, GL_FALSE, mvp.constData());
}

void TexturedQuadShader::setTexture(GLuint textureId, int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(m_textureLocation, unit);
}

void TexturedQuadShader::setScaleBias(float scale, float bias)
{
    glUniform1f(m_scaleLocation, scale);
    glUniform1f(m_biasLocation, bias);
}

void TexturedQuadShader::drawQuad(float x1, float y1, float x2, float y2)
{
    drawQuadWithTexCoords(x1, y1, x2, y2, 0.0f, 0.0f, 1.0f, 1.0f);
}

void TexturedQuadShader::drawQuadWithTexCoords(float x1, float y1, float x2, float y2,
                                                float tx1, float ty1, float tx2, float ty2)
{
    initQuadVAO();

    // Two triangles forming a quad
    // Triangle 1: bottom-left, bottom-right, top-right
    // Triangle 2: bottom-left, top-right, top-left
    float vertices[] = {
        // Position      // TexCoord
        x1, y1,          tx1, ty1,   // bottom-left
        x2, y1,          tx2, ty1,   // bottom-right
        x2, y2,          tx2, ty2,   // top-right
        
        x1, y1,          tx1, ty1,   // bottom-left
        x2, y2,          tx2, ty2,   // top-right
        x1, y2,          tx1, ty2,   // top-left
    };

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
}
