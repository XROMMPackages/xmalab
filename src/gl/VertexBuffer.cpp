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

VertexBuffer::VertexBuffer() 
	: m_initialised(false)
	, m_vaoQt(nullptr)
	, m_vboQt(nullptr)
	, m_nboQt(nullptr)
	, m_tboQt(nullptr)
	, m_iboQt(nullptr)
	, m_dataReady(false)
	, m_vertices(0)
	, m_normals(0)
	, m_texcoords(0)
	, m_indices(0)
	, m_numvertices(0)
{

}

VertexBuffer::~VertexBuffer()
{
	if (m_initialised)
	{
		if (m_vaoQt) delete m_vaoQt;
		if (m_vboQt) delete m_vboQt;
		if (m_nboQt) delete m_nboQt;
		if (m_tboQt) delete m_tboQt;
		if (m_iboQt) delete m_iboQt;

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
		{
			mutex.unlock();
			return;
		}

		setupVBO();
	}

	// Legacy render support using the Qt buffers
	// Note: This assumes a compatibility profile context if mixing with fixed function
	if (m_vboQt) {
		m_vboQt->bind();
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, NULL);
	}
	
	if (m_nboQt){
		m_nboQt->bind();
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, NULL);
	}

	if (m_tboQt){
		m_tboQt->bind();
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}

	if (m_iboQt) {
		m_iboQt->bind();
		glDrawElements(GL_TRIANGLES, m_numvertices, GL_UNSIGNED_INT, NULL);
		m_iboQt->release();
	} else {
		glDrawArrays(GL_TRIANGLES, 0, m_numvertices);
	}

	if (m_tboQt) {
		m_tboQt->release();
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (m_nboQt) {
		m_nboQt->release();
		glDisableClientState(GL_NORMAL_ARRAY);
	}
	if (m_vboQt) {
		m_vboQt->release();
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	mutex.unlock();
}

bool VertexBuffer::bindVAO()
{
	mutex.lock();
	if (!m_initialised)
	{
		if (!m_dataReady) 
		{
			mutex.unlock();
			return false;
		}
		setupVBO();
	}
	mutex.unlock();

	if (m_vaoQt) {
		m_vaoQt->bind();
		return true;
	}
	return false;
}

void VertexBuffer::releaseVAO()
{
	if (m_vaoQt) {
		m_vaoQt->release();
	}
}

void VertexBuffer::setupVBO()
{
	if (!m_dataReady) return;

	initializeOpenGLFunctions();

	m_vaoQt = new QOpenGLVertexArrayObject();
	if (!m_vaoQt->create()) {
		std::cerr << "Error creating VAO" << std::endl;
	}
	m_vaoQt->bind();

	if (m_vertices){
		m_vboQt = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_vboQt->create();
		m_vboQt->bind();
		m_vboQt->allocate(m_vertices, 3 * m_numvertices * sizeof(float));
		
		// Enable attribute 0 (Vertex Position)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_vboQt->release();
	}

	if (m_normals){
		m_nboQt = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_nboQt->create();
		m_nboQt->bind();
		m_nboQt->allocate(m_normals, 3 * m_numvertices * sizeof(float));
		
		// Enable attribute 1 (Vertex Normal)
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_nboQt->release();
	}

	if (m_texcoords){
		m_tboQt = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_tboQt->create();
		m_tboQt->bind();
		m_tboQt->allocate(m_texcoords, 2 * m_numvertices * sizeof(float));
		
		// Enable attribute 2 (Texture Coord)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_tboQt->release();
	}

	if (m_indices){
		m_iboQt = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		m_iboQt->create();
		m_iboQt->bind();
		m_iboQt->allocate(m_indices, m_numvertices * sizeof(GLuint));
		// Index buffer is part of VAO state, so we don't release it while VAO is bound
		// But for QOpenGLBuffer, we might need to be careful. 
		// QOpenGLVertexArrayObject documentation says:
		// "The index buffer is also part of the VAO state."
	}

	m_vaoQt->release();
	
	// Release IBO after VAO
	if (m_iboQt) m_iboQt->release();

	deleteData();

	m_initialised = true;
}

