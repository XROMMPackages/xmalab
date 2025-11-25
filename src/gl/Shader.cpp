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

#include "gl/Shader.h"
#include <iostream>
#include <string>
#include <vector>
#include <QOpenGLContext>

using namespace xma;

static void printShaderLog(QOpenGLFunctions_4_1_Core* gl, GLuint shader, const char* stage, const char* name)
{
	GLint logLen = 0;
	gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 1) {
		std::vector<char> buf(static_cast<size_t>(logLen));
		GLsizei outLen = 0;
		gl->glGetShaderInfoLog(shader, logLen, &outLen, buf.data());
		std::cerr << "[Shader] " << stage << " log for '" << (name ? name : "?") << "':\n" << buf.data() << std::endl;
	}
}

static void printProgramLog(QOpenGLFunctions_4_1_Core* gl, GLuint program, const char* name)
{
	GLint logLen = 0;
	gl->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 1) {
		std::vector<char> buf(static_cast<size_t>(logLen));
		GLsizei outLen = 0;
		gl->glGetProgramInfoLog(program, logLen, &outLen, buf.data());
		std::cerr << "[Shader] Link log for program '" << (name ? name : "?") << "':\n" << buf.data() << std::endl;
	}
}

Shader::Shader() : m_programID(0), m_shader(0), m_vertexShader(0), m_fragmentShader(0)
{
	/*m_shader = "Distortion";
	m_vertexShader = "varying vec2 texture_coordinate; \n"
			"void main()\n"
			"{\n"
			"\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
			"\ttexture_coordinate = vec2(gl_MultiTexCoord0); \n"
			"}\n";
	m_fragmentShader = "varying vec2 texture_coordinate;\n"
			"uniform sampler2D displacement;\n"
			"uniform sampler2D texture;\n"
			"void main()\n"
			"{\n"
			"\t\tvec4 disp_coords = texture2D(displacement,texture_coordinate);\n"
			"\t\tgl_FragColor = texture2D(texture, disp_coords.xy);\n"
			"}\n";*/
}

Shader::~Shader()
{
	if (m_programID)
	{
		if (QOpenGLContext::currentContext() && initGLFunctions()) {
			glDeleteProgram(m_programID);
		}
		m_programID = 0;
	}
}

void Shader::bindProgram()
{
	if (!initGLFunctions()) return;
	
	if (!m_programID)
	{
		m_programID = compileShader();
	}
	glUseProgram(m_programID);
}

void Shader::unbindProgram()
{
	if (!initGLFunctions()) return;
	glUseProgram(0);
}

unsigned Shader::getProgram()
{
	if (!m_programID)
	{
		m_programID = compileShader();
	}

	return m_programID;
}

unsigned int Shader::compileShader()
{
	if (!initGLFunctions()) {
		std::cerr << "could not initialize GL functions for shader" << std::endl;
		return 0;
	}
	
	if (!m_shader || !m_vertexShader || !m_fragmentShader) {
		std::cerr << "could not create Shader" << std::endl;
		return 0;
	}

	GLuint programID = glCreateProgram();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &m_vertexShader, NULL);
	glCompileShader(vertexShader);

	GLint shaderCompiled = GL_FALSE;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &shaderCompiled);
	if (shaderCompiled != GL_TRUE)
	{
		std::cerr << "Error compiling vertex shader " << m_shader << std::endl;
		printShaderLog(this, vertexShader, "vertex", m_shader);
		glDeleteProgram(programID);
		glDeleteShader(vertexShader);
		return 0;
	}

	glAttachShader(programID, vertexShader);
	glDeleteShader(vertexShader); // the program hangs onto this once it's attached

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &m_fragmentShader, NULL);
	glCompileShader(fragmentShader);

	shaderCompiled = GL_FALSE;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &shaderCompiled);
	if (shaderCompiled != GL_TRUE)
	{
		std::cerr << "Error compiling fragment shader " << m_shader << std::endl;
		printShaderLog(this, fragmentShader, "fragment", m_shader);
		glDeleteProgram(programID);
		glDeleteShader(fragmentShader);
		return 0;
	}

	glAttachShader(programID, fragmentShader);
	glDeleteShader(fragmentShader); // the program hangs onto this once it's attached

	glLinkProgram(programID);

	GLint success = GL_TRUE;
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (success != GL_TRUE)
	{
		std::cerr << "Error linking program " << m_shader << std::endl;
		printProgramLog(this, programID, m_shader);
		glDeleteProgram(programID); // FIX: delete correct programID
		return 0;
	}

	glUseProgram(programID);
	glUseProgram(0);

	return programID;
}
