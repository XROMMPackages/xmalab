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
///\file FrameBuffer.cpp
///\author Benjamin Knorlein
///\date 07/29/2016

#include <GL/glew.h>

#include "gl/FrameBuffer.h"
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

FrameBuffer::FrameBuffer(int width, int height, int samples) : m_width(width), m_height(height), m_initialised(false), m_samples(samples)
{

}

FrameBuffer::~FrameBuffer()
{
	if (m_initialised)
	{
		//Bind 0, which means render to back buffer, as a result, fb is unbound
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		//Delete resources
		glDeleteTextures(1, &m_texture_id);
		glDeleteRenderbuffersEXT(1, &m_depth_id);
		glDeleteFramebuffersEXT(1, &m_fbo);
	}
}

void FrameBuffer::setupFBO()
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	//Color buffer
	glGenTextures(1, &m_texture_id);
	if (m_samples == 0){
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture_id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA8, m_width, m_height, true);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//depth buffer
	glGenTextures(1, &m_depth_id);
	if (m_samples == 0){
		glBindTexture(GL_TEXTURE_2D, m_depth_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_depth_id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_DEPTH_COMPONENT32F, m_width, m_height, true);
	}
	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//Attach
	if (m_samples == 0){
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth_id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_depth_id, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texture_id, 0);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer not complete" << std::endl;
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (m_samples == 0){
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	m_initialised = true;
}


void FrameBuffer::bindTexture()
{
	if (!m_initialised) setupFBO();

	if (m_samples == 0){
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture_id);
	}
}

void FrameBuffer::unbindTexture()
{
	if (m_samples == 0){
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
}

unsigned FrameBuffer::getTextureID()
{
	if (!m_initialised) setupFBO();

	return m_texture_id;
}

unsigned FrameBuffer::getDepthTextureID()
{
	if (!m_initialised) setupFBO();

	return m_depth_id;
}

void FrameBuffer::bindFrameBuffer()
{
	if (!m_initialised) setupFBO();
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	if (m_samples > 0)glEnable(GL_MULTISAMPLE);
}

void FrameBuffer::unbindFrameBuffer()
{
	if (m_samples > 0)glDisable(GL_MULTISAMPLE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int FrameBuffer::getWidth()
{
	return m_width;
}

int FrameBuffer::getHeight()
{
	return m_height;
}

int FrameBuffer::getFBO()
{
	if (!m_initialised) setupFBO();

	return m_fbo;
}
