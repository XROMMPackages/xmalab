//  ----------------------------------
//  XMALab -- Copyright Â(c) 2015, Brown University, Providence, RI.
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
//  PROVIDED â€œAS ISâ€�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file BlendShader.cpp
///\author Benjamin Knorlein
///\date 7/28/2016

#include "gl/BlendShader.h"
#include <iostream>
#include <QOpenGLContext>

using namespace xma;

// Modern GLSL 410 vertex shader
static const char* blendVertexShader = 
	"#version 410 core\n"
	"layout(location = 0) in vec2 a_position;\n"
	"layout(location = 1) in vec2 a_texcoord;\n"
	"uniform mat4 u_mvp;\n"
	"out vec2 v_texcoord;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);\n"
	"    v_texcoord = a_texcoord;\n"
	"}\n";

// Modern GLSL 410 fragment shader
static const char* blendFragmentShader = 
	"#version 410 core\n"
	"in vec2 v_texcoord;\n"
	"uniform float u_transparency;\n"
	"uniform float u_useDepthTrans;\n"
	"uniform sampler2D u_texture;\n"
	"uniform sampler2D u_depth_tex;\n"
	"out vec4 fragColor;\n"
	"void main()\n"
	"{\n"
	"    vec4 color = texture(u_texture, v_texcoord);\n"
	"    if (u_useDepthTrans > 0.5) {\n"
	"        float d = texture(u_depth_tex, v_texcoord).x;\n"
	"        color.a = (d < 1.0) ? u_transparency : 0.0;\n"
	"    }\n"
	"    fragColor = color;\n"
	"}\n";

BlendShader::BlendShader() : Shader(), m_vbo(QOpenGLBuffer::VertexBuffer), m_quadInitialized(false)
{
	m_shader = "Blending";
	m_vertexShader = blendVertexShader;
	m_fragmentShader = blendFragmentShader;
}

BlendShader::~BlendShader()
{
	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();
}

void BlendShader::initializeQuad()
{
	if (m_quadInitialized) return;
	if (!QOpenGLContext::currentContext()) return;
	if (!initGLFunctions()) return;

	// Create VAO/VBO for the quad
	// Unit quad that will be scaled by the MVP matrix
	// Vertices: position (2) + texcoord (2) = 4 floats per vertex
	float vertices[] = {
		// Position     // TexCoord
		0.0f, 0.0f,     0.0f, 0.0f,
		1.0f, 0.0f,     1.0f, 0.0f,
		1.0f, 1.0f,     1.0f, 1.0f,
		0.0f, 0.0f,     0.0f, 0.0f,
		1.0f, 1.0f,     1.0f, 1.0f,
		0.0f, 1.0f,     0.0f, 1.0f
	};

	m_vao.create();
	m_vao.bind();

	m_vbo.create();
	m_vbo.bind();
	m_vbo.allocate(vertices, sizeof(vertices));

	// Position attribute (location = 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

	// TexCoord attribute (location = 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
	                      reinterpret_cast<void*>(2 * sizeof(float)));

	m_vao.release();
	m_quadInitialized = true;
}

void BlendShader::draw(unsigned int width, unsigned int height, float transparency, unsigned texture_id, unsigned int depth_texture_id, bool useDepthTrans)
{
	if (!QOpenGLContext::currentContext()) return;
	if (!initGLFunctions()) return;
	
	if (!m_quadInitialized) {
		initializeQuad();
	}
	
	bindProgram();

	// Set uniforms
	GLint loc = glGetUniformLocation(m_programID, "u_transparency");
	glUniform1f(loc, transparency);

	GLint locDepthTrans = glGetUniformLocation(m_programID, "u_useDepthTrans");
	glUniform1f(locDepthTrans, useDepthTrans ? 1.0f : 0.0f);

	GLint texLoc = glGetUniformLocation(m_programID, "u_texture");
	glUniform1i(texLoc, 0);
	texLoc = glGetUniformLocation(m_programID, "u_depth_tex");
	glUniform1i(texLoc, 1);

	// Set up orthographic projection matrix for 2D rendering
	// Maps (0,0)-(width,height) to clip space, with 0.5 offset for pixel-perfect rendering
	float mvp[16] = {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f
	};
	// Apply scale to match the quad size (width x height) and offset by -0.5
	// The quad is unit size [0,1], so we scale it to [0,width] x [0,height]
	// Then offset by -0.5 for pixel alignment
	float scale[16] = {
		(float)width, 0.0f, 0.0f, 0.0f,
		0.0f, (float)height, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 1.0f
	};
	// Multiply mvp * scale manually
	float finalMvp[16];
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			finalMvp[i * 4 + j] = 0;
			for (int k = 0; k < 4; ++k) {
				finalMvp[i * 4 + j] += mvp[i * 4 + k] * scale[k * 4 + j];
			}
		}
	}
	
	GLint mvpLoc = glGetUniformLocation(m_programID, "u_mvp");
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, finalMvp);

	// Bind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth_texture_id);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the quad using VAO
	m_vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	m_vao.release();

	glDisable(GL_BLEND);

	// Unbind textures
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	unbindProgram();
}
