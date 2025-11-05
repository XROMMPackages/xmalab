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
///\file FrameBuffer.cpp
///\author Benjamin Knorlein
///\date 07/29/2016

#include <GL/glew.h>

#include "gl/VertexBuffer.h"
#include <iostream>
#include <cstring>

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

VertexBuffer::VertexBuffer() : m_initialised(false), vboId(0), nboId(0), tboId(0), iboId(), m_dataReady(false), m_vertices(0), m_normals(0), m_texcoords(0), m_indices(0), m_numvertices(0)
{

}

VertexBuffer::~VertexBuffer()
{
	if (m_initialised)
	{
		if(vboId != 0)glDeleteBuffers(1, &vboId);
		if (nboId != 0)glDeleteBuffers(1, &nboId);
		if (tboId != 0)glDeleteBuffers(1, &tboId);
		if (iboId != 0)glDeleteBuffers(1, &iboId);

		m_initialised = false;
	}
	else if (m_dataReady)
	{
		deleteData();
	}
}

void VertexBuffer::deleteData()
{
	if (m_vertices) free(m_vertices);
	m_vertices = 0;
	if (m_normals) free(m_normals);
	m_normals = 0;
	if (m_texcoords) free(m_texcoords);
	m_texcoords = 0;
	if (m_indices) free(m_indices);
	m_indices = 0;
	m_dataReady = false;
}

void VertexBuffer::setData(unsigned int numvertices, float* vertices, float* normals, float * texcoords, unsigned int* indices)
{
	m_numvertices = numvertices;

	if (vertices){
		m_vertices = (GLfloat*)malloc(sizeof(GLfloat) * 3 * m_numvertices);
		memcpy(m_vertices, vertices, sizeof(GLfloat) * 3 * m_numvertices);
	}

	if (normals){
		m_normals = (GLfloat*)malloc(sizeof(GLfloat) * 3 * m_numvertices);
		memcpy(m_normals, normals, sizeof(GLfloat) * 3 * m_numvertices);
	}

	if (texcoords){
		m_texcoords = (GLfloat*) malloc(sizeof(GLfloat) * 2 * m_numvertices);
		memcpy(m_texcoords, texcoords, sizeof(GLfloat) * 2 * m_numvertices);
	}

	if (indices){
		m_indices = (unsigned int*)malloc(sizeof(GLuint) * m_numvertices);
		memcpy(m_indices, indices, sizeof(GLuint) * m_numvertices);
	}
	m_dataReady = true;
}

void VertexBuffer::render()
{
	mutex.lock();
	if (!m_initialised)
	{
		if (!m_dataReady) 
			return;

		setupVBO();
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	if (nboId) glEnableClientState(GL_NORMAL_ARRAY);
	if (tboId) glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	////vertices
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexPointer(3, GL_FLOAT, 0, NULL);
	
	////Normals
	if (nboId){
		glBindBuffer(GL_ARRAY_BUFFER, nboId);
		glNormalPointer(GL_FLOAT, 0, NULL);
	}
	//Texture
	if (tboId){
		glBindBuffer(GL_ARRAY_BUFFER, tboId);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}

	////Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
	////Draw the mesh
	glDrawElements(GL_TRIANGLES, m_numvertices, GL_UNSIGNED_INT, NULL);

	//unload
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	mutex.unlock();
}

void VertexBuffer::setupVBO()
{
	if (!m_dataReady) return;

	if (m_vertices){
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferData(GL_ARRAY_BUFFER, 3 * m_numvertices * sizeof(float), m_vertices, GL_STATIC_DRAW);
	}

	if (m_normals){
		glGenBuffers(1, &nboId);
		glBindBuffer(GL_ARRAY_BUFFER, nboId);
		glBufferData(GL_ARRAY_BUFFER, 3 * m_numvertices * sizeof(float), m_normals, GL_STATIC_DRAW);
	}

	if (m_texcoords){
		glGenBuffers(1, &tboId);
		glBindBuffer(GL_ARRAY_BUFFER, tboId);
		glBufferData(GL_ARRAY_BUFFER, 2 * m_numvertices * sizeof(float), m_texcoords, GL_STATIC_DRAW);
	}
	if (m_indices){
		glGenBuffers(1, &iboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numvertices * sizeof(GLuint), m_indices, GL_STATIC_DRAW);
	}

	deleteData();

	m_initialised = true;
}
