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
///\file MeshShader.cpp
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#include "gl/MeshShader.h"
#include <iostream>

using namespace xma;

static const char* shaderName = "MeshShader";

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

uniform vec3 u_light1Pos;
uniform vec3 u_light1Ambient;
uniform vec3 u_light1Diffuse;

uniform vec3 u_light2Pos;
uniform vec3 u_light2Ambient;
uniform vec3 u_light2Diffuse;

out vec4 fragColor;

void main()
{
    vec3 norm = normalize(v_normal);
    
    // Light 1
    vec3 lightDir1 = normalize(u_light1Pos - v_fragPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 ambient1 = u_light1Ambient * u_color.rgb;
    vec3 diffuse1 = u_light1Diffuse * diff1 * u_color.rgb;
    
    // Light 2
    vec3 lightDir2 = normalize(u_light2Pos - v_fragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 ambient2 = u_light2Ambient * u_color.rgb;
    vec3 diffuse2 = u_light2Diffuse * diff2 * u_color.rgb;
    
    vec3 result = ambient1 + diffuse1 + ambient2 + diffuse2;
    fragColor = vec4(result, u_color.a);
}
)";

MeshShader::MeshShader()
    : m_projectionLocation(-1)
    , m_viewLocation(-1)
    , m_modelLocation(-1)
    , m_normalMatrixLocation(-1)
    , m_colorLocation(-1)
    , m_light1PosLocation(-1)
    , m_light1AmbientLocation(-1)
    , m_light1DiffuseLocation(-1)
    , m_light2PosLocation(-1)
    , m_light2AmbientLocation(-1)
    , m_light2DiffuseLocation(-1)
{
    // Set shader sources for base class compilation
    m_shader = shaderName;
    m_vertexShader = vertexShaderSource;
    m_fragmentShader = fragmentShaderSource;
}

MeshShader::~MeshShader()
{
}

void MeshShader::initShader()
{
    if (!initGLFunctions()) {
        std::cerr << "Failed to initialize GL functions for MeshShader" << std::endl;
        return;
    }
    
    // Force shader compilation
    bindProgram();

    m_projectionLocation = glGetUniformLocation(getProgram(), "u_projection");
    m_viewLocation = glGetUniformLocation(getProgram(), "u_view");
    m_modelLocation = glGetUniformLocation(getProgram(), "u_model");
    m_normalMatrixLocation = glGetUniformLocation(getProgram(), "u_normalMatrix");
    m_colorLocation = glGetUniformLocation(getProgram(), "u_color");
    
    m_light1PosLocation = glGetUniformLocation(getProgram(), "u_light1Pos");
    m_light1AmbientLocation = glGetUniformLocation(getProgram(), "u_light1Ambient");
    m_light1DiffuseLocation = glGetUniformLocation(getProgram(), "u_light1Diffuse");
    
    m_light2PosLocation = glGetUniformLocation(getProgram(), "u_light2Pos");
    m_light2AmbientLocation = glGetUniformLocation(getProgram(), "u_light2Ambient");
    m_light2DiffuseLocation = glGetUniformLocation(getProgram(), "u_light2Diffuse");

    // Set default lighting (matching original)
    setLight1Position(QVector3D(0.0f, 0.0f, 1.0f));
    setLight1Ambient(QVector3D(0.1f, 0.1f, 0.1f));
    setLight1Diffuse(QVector3D(0.7f, 0.7f, 0.7f));
    
    setLight2Position(QVector3D(0.0f, 0.0f, -1.0f));
    setLight2Ambient(QVector3D(0.1f, 0.1f, 0.1f));
    setLight2Diffuse(QVector3D(0.7f, 0.7f, 0.7f));
    
    setColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    unbindProgram();
}

void MeshShader::bind()
{
    if (m_projectionLocation < 0) {
        initShader();
    }
    bindProgram();
}

void MeshShader::release()
{
    unbindProgram();
}

void MeshShader::setProjectionMatrix(const QMatrix4x4& projection)
{
    glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, projection.constData());
}

void MeshShader::setViewMatrix(const QMatrix4x4& view)
{
    glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, view.constData());
}

void MeshShader::setModelMatrix(const QMatrix4x4& model)
{
    glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, model.constData());
    
    // Calculate normal matrix (inverse transpose of upper-left 3x3 of model matrix)
    QMatrix3x3 normalMatrix = model.normalMatrix();
    glUniformMatrix3fv(m_normalMatrixLocation, 1, GL_FALSE, normalMatrix.constData());
}

void MeshShader::setColor(float r, float g, float b, float a)
{
    glUniform4f(m_colorLocation, r, g, b, a);
}

void MeshShader::setColor(const QVector4D& color)
{
    glUniform4f(m_colorLocation, color.x(), color.y(), color.z(), color.w());
}

void MeshShader::setLight1Position(const QVector3D& pos)
{
    glUniform3f(m_light1PosLocation, pos.x(), pos.y(), pos.z());
}

void MeshShader::setLight1Ambient(const QVector3D& ambient)
{
    glUniform3f(m_light1AmbientLocation, ambient.x(), ambient.y(), ambient.z());
}

void MeshShader::setLight1Diffuse(const QVector3D& diffuse)
{
    glUniform3f(m_light1DiffuseLocation, diffuse.x(), diffuse.y(), diffuse.z());
}

void MeshShader::setLight2Position(const QVector3D& pos)
{
    glUniform3f(m_light2PosLocation, pos.x(), pos.y(), pos.z());
}

void MeshShader::setLight2Ambient(const QVector3D& ambient)
{
    glUniform3f(m_light2AmbientLocation, ambient.x(), ambient.y(), ambient.z());
}

void MeshShader::setLight2Diffuse(const QVector3D& diffuse)
{
    glUniform3f(m_light2DiffuseLocation, diffuse.x(), diffuse.y(), diffuse.z());
}
