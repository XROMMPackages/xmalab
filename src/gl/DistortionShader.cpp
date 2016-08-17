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

DistortionShader::DistortionShader(Camera * camera) : QObject(), FrameBuffer(camera->getWidth(), camera->getHeight()), Shader(), m_tex(0), m_camera(camera), m_distortionRunning(false), m_numpoints(0), m_coords(NULL)
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
		"uniform sampler2D texture_coords;\n"
		"uniform sampler2D texture;\n"
		"uniform sampler2D depth_tex;\n"
		"void main()\n"
		"{\n"
			"		vec4 coords4 = texture2D(texture_coords, texture_coordinate.xy);\n"
			"		vec2 coords2;\n"
			"		coords2.x = (coords4.x*255.0 + coords4.y*255.0/256.0)/256.0;\n"
			"		coords2.y = (coords4.z*255.0 + coords4.w*255.0/256.0)/256.0;\n"
			"		vec4 color = texture2D(texture, coords2.xy);\n"
			"		float d = texture2D(depth_tex, coords2.xy).x;\n"
			"		color.a =  (d < 1.0 ) ? transparency : 0.0 ;\n"
			"		gl_FragColor = color; \n"
			"}\n";
	stopped = false;
}

void to2Char(double value, unsigned char * pt)
{
	unsigned int uivalue = value * 256 * 256 + 0.5;
	pt[0] = uivalue / 256;
	pt[1] = uivalue % 256;
}

DistortionShader::~DistortionShader()
{
	stopped = true;
	if (m_tex)
	{
		glDeleteTextures(1, &m_tex);
		m_tex = 0;
	}

	if (m_coords) delete[] m_coords;
	m_coords = NULL;

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

	
	if (m_tex == 0 && m_distortionComplete && m_coords)
	{
		intializeTexture();
	}

	if (m_tex){
		bindProgram();
		GLint loc = glGetUniformLocation(m_programID, "transparency");
		glUniform1f(loc, transparency);

		GLint texLoc = glGetUniformLocation(m_programID, "texture");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(m_programID, "depth_tex");
		glUniform1i(texLoc, 1);
		texLoc = glGetUniformLocation(m_programID, "texture_coords");
		glUniform1i(texLoc, 2);


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

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_tex);


		glColor3f(1.0, 1.0, 1.0);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2d(-0.5, -0.5);
		glTexCoord2f(0, 1);
		glVertex2d(-0.5, getHeight() - 0.5);
		glTexCoord2f(1, 1);
		glVertex2d(getWidth() - 0.5, getHeight() - 0.5);
		glTexCoord2f(1, 0);
		glVertex2d(getWidth() - 0.5, -0.5);
		glEnd();

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
	m_coords = new unsigned char[getWidth() * getHeight() * 4];

	float x_dist, y_dist;
	for (int y = 0; y < getHeight(); y++)
	{
		for (int x = 0; x < getWidth(); x++)
		{
			cv::Point2d pt = m_camera->undistortPoint(cv::Point2d(x, y), true, false, false);
			
			if (pt.x < 0 || pt.y < 0)
			{
				x_dist = (0.0);
				y_dist = (0.0);
			}
			else
			{
				x_dist = ((pt.x + 0.5) / getWidth());
				y_dist  = ((pt.y + 0.5) / getHeight());
			}

			to2Char(x_dist, &m_coords[(y*getWidth() + x) * 4]);
			to2Char(y_dist, &m_coords[(y*getWidth() + x) * 4 + 2]);

			/*double t1, t2, t3, t4;
			t1 = 1.0 / 255.0f * m_coords[(y*getWidth() + x) * 4];
			t2 = 1.0 / 255.0f * m_coords[(y*getWidth() + x) * 4 + 1];
			t3 = 1.0 / 255.0f * m_coords[(y*getWidth() + x) * 4 + 2];
			t4 = 1.0 / 255.0f * m_coords[(y*getWidth() + x) * 4 + 3];


			std::cerr << x_dist - (t1 * 255.0 + t2 * 255.0 / 256.0) / 256.0 << std::endl;
			std::cerr << y_dist - (t3 * 255.0 + t4 * 255.0 / 256.0) / 256.0 << std::endl;

			*/
			if (stopped) {
				nbInstances--;
				
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
		nbInstances--;

		if (nbInstances == 0)
		{
			m_distortionComplete = true;
		}
	}
}
