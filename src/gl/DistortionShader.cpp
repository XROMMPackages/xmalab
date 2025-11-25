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
///\file DistortionShader.cpp
///\author Benjamin Knorlein
///\date 7/28/2016

#include "gl/DistortionShader.h"
#include "gl/VertexBuffer.h"
#include "core/Camera.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <QOpenGLContext>

#include <iostream>
#include <new>
#include <vector>
#include <algorithm>

using namespace xma;

static QBasicMutex s_distortionMutex;

int DistortionShader::nbInstances = 0;
bool DistortionShader::m_distortionComplete = false;

// Modern GLSL 410 vertex shader
static const char* distortionVertexShader = 
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
static const char* distortionFragmentShader = 
	"#version 410 core\n"
	"in vec2 v_texcoord;\n"
	"uniform float u_transparency;\n"
	"uniform sampler2D u_texture_coords;\n"
	"uniform sampler2D u_texture;\n"
	"uniform sampler2D u_depth_tex;\n"
	"out vec4 fragColor;\n"
	"void main()\n"
	"{\n"
	"    vec4 coords4 = texture(u_texture_coords, v_texcoord);\n"
	"    vec2 coords2;\n"
	"    coords2.x = (coords4.x * 255.0 + coords4.y * 255.0 / 256.0) / 256.0;\n"
	"    coords2.y = (coords4.z * 255.0 + coords4.w * 255.0 / 256.0) / 256.0;\n"
	"    vec4 color = texture(u_texture, coords2);\n"
	"    float d = texture(u_depth_tex, coords2).x;\n"
	"    color.a = (d < 1.0) ? u_transparency : 0.0;\n"
	"    fragColor = color;\n"
	"}\n";

DistortionShader::DistortionShader(Camera * camera) 
	: QObject()
	, FrameBuffer(camera->getWidth(), camera->getHeight())
	, Shader()
	, m_tex(0)
	, m_camera(camera)
	, m_distortionRunning(false)
	, m_numpoints(0)
	, m_coords(NULL)
	, m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_quadInitialized(false)
{
	m_shader = "Distortion";
	m_vertexShader = distortionVertexShader;
	m_fragmentShader = distortionFragmentShader;
	stopped = false;
}

DistortionShader::~DistortionShader()
{
	stopped = true;
	if (m_tex)
	{
		if (QOpenGLContext::currentContext() && initGLFunctions()) {
			glDeleteTextures(1, &m_tex);
		}
		m_tex = 0;
	}

	if (m_coords) delete[] m_coords;
	m_coords = NULL;

	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();

	{
		QMutexLocker lock(&s_distortionMutex);
		m_distortionComplete = false;
	}
}

void DistortionShader::initializeQuad()
{
	if (m_quadInitialized) return;
	if (!QOpenGLContext::currentContext()) return;
	if (!initGLFunctions()) return;

	// Create VAO/VBO for the quad
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

static void to2Char(double value, unsigned char * pt)
{
	unsigned int uivalue = value * 256 * 256 + 0.5;
	pt[0] = uivalue / 256;
	pt[1] = uivalue % 256;
}

void DistortionShader::draw(unsigned texture_id, unsigned depth_texture_id, float transparency)
{
	if (!m_distortionComplete && !m_distortionRunning && (m_camera->hasUndistortion() || m_camera->hasModelDistortion()))
	{
		m_distortionRunning = true;
		{
			QMutexLocker lock(&s_distortionMutex);
			nbInstances++;
		}

		m_FutureWatcher = new QFutureWatcher<void>(this);
		connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(loadComplete()));

		QFuture<void> future = QtConcurrent::run(&DistortionShader::setDistortionMap, this);
		m_FutureWatcher->setFuture(future);
	}

	if (m_tex == 0 && m_distortionComplete && m_coords)
	{
		intializeTexture();
	}

	if (m_tex){
		if (!QOpenGLContext::currentContext()) return;
		if (!initGLFunctions()) return;
		
		if (!m_quadInitialized) {
			initializeQuad();
		}
		
		bindProgram();
		
		// Set uniforms
		GLint loc = glGetUniformLocation(m_programID, "u_transparency");
		glUniform1f(loc, transparency);

		GLint texLoc = glGetUniformLocation(m_programID, "u_texture");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(m_programID, "u_depth_tex");
		glUniform1i(texLoc, 1);
		texLoc = glGetUniformLocation(m_programID, "u_texture_coords");
		glUniform1i(texLoc, 2);


		bindFrameBuffer();
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		glViewport(0, 0, getWidth(), getHeight());
		
		// Set up orthographic projection matrix for 2D rendering
		float w = (float)getWidth();
		float h = (float)getHeight();
		
		// Create MVP that maps unit quad [0,1] to screen coords, then to clip space
		// Ortho projection: maps (0,0)-(width,height) to (-1,-1)-(1,1)
		// Scale: scales unit quad to (width, height)
		// The combined MVP transforms unit quad to fill the framebuffer
		float mvp[16] = {
			2.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			-1.0f - 0.5f * 2.0f / w, -1.0f - 0.5f * 2.0f / h, 0.0f, 1.0f
		};
		
		GLint mvpLoc = glGetUniformLocation(m_programID, "u_mvp");
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);

		// Bind textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_texture_id);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_tex);

		// Draw the quad using VAO
		m_vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		m_vao.release();

		// Unbind textures
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		unbindFrameBuffer();
		unbindProgram();
	}
}

void DistortionShader::setDistortionMap()
{
	// Safe allocation for potentially huge images
	size_t w = static_cast<size_t>(getWidth());
	size_t h = static_cast<size_t>(getHeight());
	if (w == 0 || h == 0 || w > SIZE_MAX / 4 / h) {
		QMutexLocker lock(&s_distortionMutex);
		nbInstances = (nbInstances > 0 ? nbInstances - 1 : 0);
		m_distortionComplete = (nbInstances == 0);
		return;
	}

	try {
		m_coords = new unsigned char[w * h * 4];
	}
	catch (const std::bad_alloc&) {
		std::cerr << "[DistortionShader] Failed to allocate distortion map for " << w << "x" << h << std::endl;
		QMutexLocker lock(&s_distortionMutex);
		nbInstances = (nbInstances > 0 ? nbInstances - 1 : 0);
		m_distortionComplete = (nbInstances == 0);
		return;
	}

	float x_dist, y_dist;
	for (int y = 0; y < getHeight(); y++)
	{
		for (int x = 0; x < getWidth(); x++)
		{
			cv::Point2d pt = m_camera->undistortPoint(cv::Point2d(x, y), true, false, false);
			
			if (pt.x < 0 || pt.y < 0)
			{
				x_dist = (0.0f);
				y_dist = (0.0f);
			}
			else
			{
				x_dist = static_cast<float>(((pt.x + 0.5) / getWidth()));
				y_dist  = static_cast<float>(((pt.y + 0.5) / getHeight()));
			}

			to2Char(x_dist, &m_coords[(static_cast<size_t>(y)*w + static_cast<size_t>(x)) * 4]);
			to2Char(y_dist, &m_coords[(static_cast<size_t>(y)*w + static_cast<size_t>(x)) * 4 + 2]);

			if (stopped) {
				QMutexLocker lock(&s_distortionMutex);
				nbInstances = (nbInstances > 0 ? nbInstances - 1 : 0);
				return;
			}
		}
	}
}

bool DistortionShader::canRender()
{
	return m_tex != 0;
}

void DistortionShader::intializeTexture()
{
	if (!QOpenGLContext::currentContext()) return;
	if (!initGLFunctions()) return;
	
	glGenTextures(1, &m_tex);
	glBindTexture(GL_TEXTURE_2D, m_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_coords);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] m_coords;
	m_coords = NULL;
}

void DistortionShader::loadComplete()
{
	if (!stopped){
		{
			QMutexLocker lock(&s_distortionMutex);
			nbInstances = (nbInstances > 0 ? nbInstances - 1 : 0);
			if (nbInstances == 0)
			{
				m_distortionComplete = true;
			}
		}
		m_distortionRunning = false;
	}
}
