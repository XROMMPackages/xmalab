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
///\file LitColorShader.cpp
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#include "gl/LitColorShader.h"
#include "gl/GLPrimitives.h"
#include <iostream>

using namespace xma;

static const char* shaderName = "LitColorShader";

static const char* vertexShaderSource = R"(
#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat3 u_normalMatrix;

out vec3 v_normal;
out vec3 v_fragPos;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_fragPos = worldPos.xyz;
    v_normal = u_normalMatrix * a_normal;
    gl_Position = u_projection * u_view * worldPos;
}
)";

static const char* fragmentShaderSource = R"(
#version 410 core

in vec3 v_normal;
in vec3 v_fragPos;

uniform vec4 u_color;
uniform vec3 u_lightPos;
uniform vec3 u_ambient;
uniform vec3 u_diffuse;

out vec4 fragColor;

void main()
{
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(u_lightPos - v_fragPos);
    
    // Ambient
    vec3 ambient = u_ambient * u_color.rgb;
    
    // Diffuse (two-sided lighting)
    float diff = abs(dot(norm, lightDir));
    vec3 diffuse = u_diffuse * diff * u_color.rgb;
    
    vec3 result = ambient + diffuse;
    fragColor = vec4(result, u_color.a);
}
)";

LitColorShader::LitColorShader()
    : m_projectionLocation(-1)
    , m_viewLocation(-1)
    , m_modelLocation(-1)
    , m_normalMatrixLocation(-1)
    , m_colorLocation(-1)
    , m_lightPosLocation(-1)
    , m_ambientLocation(-1)
    , m_diffuseLocation(-1)
{
    // Set shader sources for base class compilation
    m_shader = shaderName;
    m_vertexShader = vertexShaderSource;
    m_fragmentShader = fragmentShaderSource;
}

LitColorShader::~LitColorShader()
{
}

void LitColorShader::initShader()
{
    if (!initGLFunctions()) {
        std::cerr << "Failed to initialize GL functions for LitColorShader" << std::endl;
        return;
    }
    
    // Force shader compilation
    bindProgram();

    m_projectionLocation = glGetUniformLocation(getProgram(), "u_projection");
    m_viewLocation = glGetUniformLocation(getProgram(), "u_view");
    m_modelLocation = glGetUniformLocation(getProgram(), "u_model");
    m_normalMatrixLocation = glGetUniformLocation(getProgram(), "u_normalMatrix");
    m_colorLocation = glGetUniformLocation(getProgram(), "u_color");
    m_lightPosLocation = glGetUniformLocation(getProgram(), "u_lightPos");
    m_ambientLocation = glGetUniformLocation(getProgram(), "u_ambient");
    m_diffuseLocation = glGetUniformLocation(getProgram(), "u_diffuse");

    // Set default lighting
    setAmbientColor(QVector3D(0.3f, 0.3f, 0.3f));
    setDiffuseColor(QVector3D(0.5f, 0.5f, 0.5f));
    setLightPosition(QVector3D(0.0f, 10.0f, 0.0f));
    
    unbindProgram();
}

void LitColorShader::bind()
{
    if (m_projectionLocation < 0) {
        initShader();
    }
    bindProgram();
}

void LitColorShader::release()
{
    unbindProgram();
}

void LitColorShader::setProjectionMatrix(const QMatrix4x4& projection)
{
    m_projection = projection;
    glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, projection.constData());
}

void LitColorShader::setViewMatrix(const QMatrix4x4& view)
{
    m_view = view;
    glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, view.constData());
}

void LitColorShader::setModelMatrix(const QMatrix4x4& model)
{
    m_model = model;
    glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, model.constData());
    
    // Calculate normal matrix (inverse transpose of upper-left 3x3 of model matrix)
    QMatrix3x3 normalMatrix = model.normalMatrix();
    glUniformMatrix3fv(m_normalMatrixLocation, 1, GL_FALSE, normalMatrix.constData());
}

void LitColorShader::setColor(float r, float g, float b, float a)
{
    glUniform4f(m_colorLocation, r, g, b, a);
}

void LitColorShader::setColor(const QVector4D& color)
{
    glUniform4f(m_colorLocation, color.x(), color.y(), color.z(), color.w());
}

void LitColorShader::setLightPosition(const QVector3D& pos)
{
    glUniform3f(m_lightPosLocation, pos.x(), pos.y(), pos.z());
}

void LitColorShader::setAmbientColor(const QVector3D& ambient)
{
    glUniform3f(m_ambientLocation, ambient.x(), ambient.y(), ambient.z());
}

void LitColorShader::setDiffuseColor(const QVector3D& diffuse)
{
    glUniform3f(m_diffuseLocation, diffuse.x(), diffuse.y(), diffuse.z());
}

void LitColorShader::drawSphere(const QVector3D& position, float radius)
{
    QMatrix4x4 model;
    model.translate(position);
    model.scale(radius);
    setModelMatrix(model);
    
    // Use the GLSphere singleton
    QMatrix4x4 mvp = m_projection * m_view * model;
    GLSphere::instance()->draw(mvp, model, QVector4D(1, 1, 1, 1), true);
}
