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
///\file FrameBuffer.h
///\author Benjamin Knorlein
///\date 07/29/2016

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

namespace xma
{

	class FrameBuffer
	{
	public:
		FrameBuffer(int width, int height, int samples = 0);

		virtual ~FrameBuffer();

		virtual void bindTexture();
		virtual void unbindTexture();

		virtual void blitFramebuffer(){}

		virtual unsigned int getTextureID();
		virtual unsigned int getDepthTextureID();

		void bindFrameBuffer();
		void unbindFrameBuffer();

		int getWidth();
		int getHeight();
		int getFBO();


	protected:
		bool m_initialised;
		unsigned int m_width;
		unsigned int m_height;
		unsigned int m_samples;

		virtual void setupFBO();
		unsigned int m_texture_id;
		unsigned int m_depth_id;
		unsigned int m_fbo;
		
		int m_pdrawFboId;
		int m_preadFboId;
	};
}

#endif // FRAMEBUFFER_H_