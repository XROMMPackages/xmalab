//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
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
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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

#include <GL/glew.h>
#include "gl/DistortionShader.h"
#include "gl/VertexBuffer.h"
#include "core/Camera.h"

#include <QtCore>

#include <iostream>


#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif

using namespace xma;

int DistortionShader::nbInstances = 0;
bool DistortionShader::m_distortionComplete = false;

DistortionShader::DistortionShader(Camera * camera) : QObject(), FrameBuffer(camera->getWidth(), camera->getHeight()), Shader(), m_vbo(0), m_camera(camera), m_distortionRunning(false), m_numpoints(0)
{
	m_shader = "Distortion";
	m_vertexShader = "varying vec2 texture_coordinate; \n"
			"void main()\n"
			"{\n"
			"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
			"	texture_coordinate = vec2(gl_MultiTexCoord0); \n"
			"}\n";
	m_fragmentShader = "varying vec2 texture_coordinate;\n"
			"uniform float transparency;\n"
			"uniform sampler2D texture;\n"
			"uniform sampler2D depth_tex;\n"
			"void main()\n"
			"{\n"
			"		vec4 color = texture2D(texture, texture_coordinate.xy);\n"
			"		float d = texture2D(depth_tex, texture_coordinate.xy).x;\n"
			"		color.a =  (d < 1.0 ) ? transparency : 0.0 ;\n"
			"		gl_FragColor = color; \n"
			"}\n";
	stopped = false;
}

DistortionShader::~DistortionShader()
{
	stopped = true;
	//loading.lock(); 
	//loading.unlock(); 
	if (m_vbo)
	{
		delete m_vbo;
	}

	m_vertices.clear();
	m_texcoords.clear();
	m_indices.clear();
	m_distortionComplete = false;
}

void DistortionShader::draw(unsigned texture_id, unsigned depth_texture_id, float transparency)
{
	if (!m_distortionComplete && !m_distortionRunning && (m_camera->hasUndistortion() || m_camera->hasModelDistortion()))
	{
		m_distortionRunning = true;
		nbInstances++;
		m_FutureWatcher = new QFutureWatcher<void>();
		connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(loadComplete()));

		QFuture<void> future = QtConcurrent::run(this, &DistortionShader::setDistortionMap);		
		m_FutureWatcher->setFuture(future);
	}

	
	if (m_vbo == 0 && m_distortionComplete)
	{
		intialiseVBO();
	}

	if (m_vbo){
		bindProgram();
		GLint loc = glGetUniformLocation(m_programID, "transparency");
		glUniform1f(loc, transparency);

		GLint texLoc = glGetUniformLocation(m_programID, "texture");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(m_programID, "depth_tex");
		glUniform1i(texLoc, 1);

		bindFrameBuffer();
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST); // Enables Depth Testing
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations

		glViewport(0, 0, getWidth(), getHeight());
		glDisable(GL_LIGHTING);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, getWidth(), 0, getHeight(),-1000,1000);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_texture_id);

		glColor3f(1.0, 1.0, 1.0);
		m_vbo->render();
		
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

	//loading.lock();
	std::vector <GLfloat> vertices;
	std::vector <GLfloat> texcoords;

	for (int y = 0; y < getHeight(); y++)
	{
		for (int x = 0; x < getWidth(); x++)
		{
			cv::Point2d pt = m_camera->undistortPoint(cv::Point2d(x, y), true, false, false);
			vertices.push_back(x + 0.5);
			vertices.push_back(y + 0.5);
			vertices.push_back(1);
			if (pt.x < 0 || pt.y < 0)
			{
				texcoords.push_back(0.0);
				texcoords.push_back(0.0);
			}
			else
			{
				texcoords.push_back((pt.x + 0.5) / getWidth());
				texcoords.push_back((pt.y + 0.5) / getHeight());
			}

			if (stopped) {
				vertices.clear();
				texcoords.clear();
				nbInstances--;
				//loading.unlock();
				return;
			}
		}
	}
	m_numpoints = 0;
	int start;
	for (int y = 0; y < getHeight() - 1; y++)
	{
		for (int x = 0; x < getWidth() - 1; x++)
		{
			//Triangle 1
			start = y * getHeight() + x;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			start = y * getHeight() + x + 1;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			start = (y + 1) * getHeight() + 1 + x;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			//Triangle 2
			start = y * getHeight() + x + 1;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			start = (y + 1) * getHeight() + x + 1;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			start = (y + 1) * getHeight() + 1 + x;
			m_vertices.push_back(vertices[3 * start]);
			m_vertices.push_back(vertices[3 * start + 1]);
			m_vertices.push_back(vertices[3 * start + 2]);
			m_texcoords.push_back(texcoords[2 * start]);
			m_texcoords.push_back(texcoords[2 * start + 1]);
			m_indices.push_back(m_numpoints);
			m_numpoints++;

			if (stopped) {
				vertices.clear();
				texcoords.clear();
				nbInstances--;
				//loading.unlock();
				return;
			}
		}
	}
	vertices.clear();
	texcoords.clear();
	//loading.unlock();
}

bool DistortionShader::canRender()
{
	return m_vbo != 0;
}

void DistortionShader::intialiseVBO()
{
	m_vbo = new VertexBuffer();
	m_vbo->setData(m_numpoints, &m_vertices[0], 0, &m_texcoords[0], &m_indices[0]);

	m_vertices.clear();
	m_texcoords.clear();
	m_indices.clear();
}

void DistortionShader::loadComplete()
{
	if (!stopped){
		nbInstances--;
		if (nbInstances == 0)
		{
			m_distortionComplete = true;
		}
	}
}
