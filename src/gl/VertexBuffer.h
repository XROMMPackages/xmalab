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
///\file VertexBuffer.h
///\author Benjamin Knorlein
///\date 07/29/2016

#ifndef VERTEXBUFFER_H_
#define VERTEXBUFFER_H_
#include <QtCore/QMutex>
#include "gl/GLFunctions.h"

namespace xma
{

	class VertexBuffer : public GLFunctions
	{
	public:
		VertexBuffer();

		~VertexBuffer();

		void setData(unsigned int numvertices, float* vertices, float* normals, float * texcoords, unsigned int* indices);

		void render();

	private:
		bool m_initialised;

		void setupVBO();
		unsigned int vaoId;
		unsigned int vboId;
		unsigned int nboId;
		unsigned int tboId;
		unsigned int iboId;

		void deleteData();
		bool m_dataReady;
		unsigned int m_numvertices;
		float* m_vertices;
		float* m_normals;
		float* m_texcoords;
		unsigned int * m_indices;

		QMutex mutex;

	};
}

#endif // FRAMEBUFFER_H_