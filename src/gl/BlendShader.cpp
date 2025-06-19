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
///\file Shader.cpp
///\author Benjamin Knorlein
///\date 7/28/2016

#include <GL/glew.h>
#include "gl/BlendShader.h"

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

BlendShader::BlendShader() : Shader()
{
	m_shader = "Blending";
	m_vertexShader = "varying vec2 texture_coordinate; \n"
			"void main()\n"
			"{\n"
			"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
			"	texture_coordinate = vec2(gl_MultiTexCoord0); \n"
			"}\n";
	m_fragmentShader = "varying vec2 texture_coordinate;\n"
		"uniform float transparency;\n"
		"uniform float useDepthTrans;\n"
		"uniform sampler2D texture;\n"
		"uniform sampler2D depth_tex;\n"
		"void main()\n"
		"{\n"
		"		vec4 color = texture2D(texture, texture_coordinate.xy);\n"
		"		if (useDepthTrans > 0.5){\n"
		"			float d = texture2D(depth_tex, texture_coordinate.xy).x;\n"
		"			color.a =  (d < 1.0 ) ? transparency : 0.0 ;\n"
		"		}\n"
		"		gl_FragColor = color; \n"
		"}\n";
}

BlendShader::~BlendShader()
{
	
}

void BlendShader::draw(unsigned int width, unsigned int height, float transparency, unsigned texture_id, unsigned int depth_texture_id, bool useDepthTrans)
{
	bindProgram();

	GLint loc = glGetUniformLocation(m_programID, "transparency");
	glUniform1f(loc, transparency);

	GLint locDepthTrans = glGetUniformLocation(m_programID, "useDepthTrans");
	if (useDepthTrans){
		glUniform1f(locDepthTrans, 1.0);
	} else
	{
		glUniform1f(locDepthTrans, 0.0);
	}

	GLint texLoc = glGetUniformLocation(m_programID, "texture");
	glUniform1i(texLoc, 0);
	texLoc = glGetUniformLocation(m_programID, "depth_tex");
	glUniform1i(texLoc, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth_texture_id);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2d(-0.5, -0.5);
	glTexCoord2f(0, 1);
	glVertex2d(-0.5, height - 0.5);
	glTexCoord2f(1, 1);
	glVertex2d(width - 0.5, height - 0.5);
	glTexCoord2f(1, 0);
	glVertex2d( width - 0.5, -0.5);
	glEnd();

	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	unbindProgram();
}
