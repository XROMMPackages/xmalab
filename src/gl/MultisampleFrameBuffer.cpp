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
///\file MultisampleFrameBuffer.cpp
///\author Benjamin Knorlein
///\date 08/01/2016

#include <GL/glew.h>

#include "gl/MultisampleFrameBuffer.h"
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

MultisampleFrameBuffer::MultisampleFrameBuffer(int width, int height, int samples) : FrameBuffer(width,height, samples)
{
	outbuffer = new FrameBuffer(width, height); 
}



MultisampleFrameBuffer::~MultisampleFrameBuffer()
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
	delete outbuffer;
}

void MultisampleFrameBuffer::blitFramebuffer()
{

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outbuffer->getFBO());   // Make sure no FBO is set as the draw framebuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, getFBO()); // Make sure your multisampled FBO is the read framebuffer
	glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT,GL_NEAREST);
	glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // Make sure no FBO is set as the draw framebuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // Make sure your multisampled FBO is the read framebuffer
}

void MultisampleFrameBuffer::bindTexture()
{
	glBindTexture(GL_TEXTURE_2D, outbuffer->getTextureID());
}

void MultisampleFrameBuffer::unbindTexture()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned MultisampleFrameBuffer::getTextureID()
{
	return outbuffer->getTextureID();
}

unsigned int MultisampleFrameBuffer::getDepthTextureID()
{
	return outbuffer->getDepthTextureID();
}