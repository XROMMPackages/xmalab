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
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

namespace xma
{

	class VertexBuffer : protected QOpenGLFunctions
	{
	public:
		VertexBuffer();

		~VertexBuffer();

		void setData(unsigned int numvertices, float* vertices, float* normals, float * texcoords, unsigned int* indices);

		// Legacy render (fixed function)
		void render();

		// Modern render support
		bool bindVAO();
		void releaseVAO();
		unsigned int getNumVertices() const { return m_numvertices; }

	private:
		bool m_initialised;

		void setupVBO();
		
		// Qt OpenGL wrappers
		QOpenGLVertexArrayObject* m_vaoQt;
		QOpenGLBuffer* m_vboQt;
		QOpenGLBuffer* m_nboQt;
		QOpenGLBuffer* m_tboQt;
		QOpenGLBuffer* m_iboQt;

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

#endif // VERTEXBUFFER_H_