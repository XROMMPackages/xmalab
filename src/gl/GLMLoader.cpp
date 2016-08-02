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
///\file GLMLoader.cpp
///\author Benjamin Knorlein
///\date 07/29/2016

/// The contents on this file are based on the GLM Loader by Nate Robins
/// Nate Robins, 1997
/// ndr@pobox.com, http://www.pobox.com/~ndr/


#include <GL/glew.h>

#include "gl/GLMLoader.h"
#include <iostream>
#include <cassert>
#include <math.h>

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

#ifndef ANGLE_THRES
#define ANGLE_THRES 0
#endif

#ifndef M_PI
#define M_PI 3.14159265
#endif

using namespace xma;

	/* GLMtriangle: Structure that defines a triangle in a model.
	*/
	typedef struct _GLMtriangle {
		GLuint vindices[3];			/* array of triangle vertex indices */
		GLuint nindices[3];			/* array of triangle normal indices */
		GLuint findex;			/* index of triangle facet normal */
	} GLMtriangle;

	/* _GLMnode: general purpose node
	*/
	typedef struct _GLMnode {
		GLuint           index;
		GLboolean        averaged;
		struct _GLMnode* next;
	} GLMnode;

	/* glmCross: compute the cross product of two vectors
	*
	* u - array of 3 GLfloats (GLfloat u[3])
	* v - array of 3 GLfloats (GLfloat v[3])
	* n - array of 3 GLfloats (GLfloat n[3]) to return the cross product in
	*/
	static GLvoid
		glmCross(GLfloat* u, GLfloat* v, GLfloat* n)
	{
		assert(u); assert(v); assert(n);

		n[0] = u[1] * v[2] - u[2] * v[1];
		n[1] = u[2] * v[0] - u[0] * v[2];
		n[2] = u[0] * v[1] - u[1] * v[0];
	}

	/* glmNormalize: normalize a vector
	*
	* v - array of 3 GLfloats (GLfloat v[3]) to be normalized
	*/
	static GLvoid
		glmNormalize(GLfloat* v)
	{
		GLfloat l;

		assert(v);

		l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}

	/* glmDot: compute the dot product of two vectors
	*
	* u - array of 3 GLfloats (GLfloat u[3])
	* v - array of 3 GLfloats (GLfloat v[3])
	*/
	static GLfloat
		glmDot(GLfloat* u, GLfloat* v)
	{
		assert(u); assert(v);

		return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
	}

VertexBuffer* GLMLoader::load(QString filename)
{
	FILE*     file;

	/* open the file */
	file = fopen(filename.toAscii().data(), "r");
	if (!file)
		return NULL;

	unsigned int m_numvertices;
	float* m_vertices;
	float* m_normals;
	unsigned int * m_indices;

	GLuint    numvertices;		/* number of vertices in model */
	GLuint    numnormals;		/* number of normals in model */
	GLuint    numtriangles;		/* number of triangles in model */
	char      buf[128];
	unsigned  v, n, t;

	numvertices = numnormals = numtriangles = 0;
	while (fscanf(file, "%s", buf) != EOF) {
		switch (buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'v':				/* v, vn, vt */
			switch (buf[1]) {
			case '\0':			/* vertex */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				numvertices++;
				break;
			case 'n':				/* normal */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				numnormals++;
				break;
			default:
				fgets(buf, sizeof(buf), file);
				break;
			}
			break;
		case 'f':				/* face */
			v = n = t = 0;
			fscanf(file, "%s", buf);
			/* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
			if (strstr(buf, "//")) {
				/* v//n */
				sscanf(buf, "%d//%d", &v, &n);
				fscanf(file, "%d//%d", &v, &n);
				fscanf(file, "%d//%d", &v, &n);
				numtriangles++;
				while (fscanf(file, "%d//%d", &v, &n) > 0) {
					numtriangles++;
				}
			}
			else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
				/* v/t/n */
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				numtriangles++;
				while (fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
					numtriangles++;
				}
			}
			else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
				/* v/t */
				fscanf(file, "%d/%d", &v, &t);
				fscanf(file, "%d/%d", &v, &t);
				numtriangles++;
				while (fscanf(file, "%d/%d", &v, &t) > 0) {
					numtriangles++;
				}
			}
			else {
				/* v */
				fscanf(file, "%d", &v);
				fscanf(file, "%d", &v);
				numtriangles++;
				while (fscanf(file, "%d", &v) > 0) {
					numtriangles++;
				}
			}
			break;
		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}

	/* allocate memory */
	GLfloat* vertices = (GLfloat*)malloc(sizeof(GLfloat) * 3 * numvertices);
	GLMtriangle* triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) * numtriangles);
	GLfloat* normals;

	if (numnormals > 0) {
		normals = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (numnormals));
	}

	rewind(file);

	numtriangles = numvertices = numnormals = 0;

	while (fscanf(file, "%s", buf) != EOF) {
		switch (buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'v':				/* v, vn, vt */
			switch (buf[1]) {
			case '\0':			/* vertex */
				fscanf(file, "%f %f %f",
					&vertices[3 * numvertices + 0],
					&vertices[3 * numvertices + 1],
					&vertices[3 * numvertices + 2]);
				numvertices++;
				break;
			case 'n':				/* normal */
				fscanf(file, "%f %f %f",
					&normals[3 * numnormals + 0],
					&normals[3 * numnormals + 1],
					&normals[3 * numnormals + 2]);
				numnormals++;
				break;
			default:
				fgets(buf, sizeof(buf), file);
				break;
			}
			break;
		case 'f':				/* face */
			v = n = t = 0;
			fscanf(file, "%s", buf);
			/* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
			if (strstr(buf, "//")) {
				/* v//n */
				sscanf(buf, "%d//%d", &v, &n);
				triangles[numtriangles].vindices[0] = v;
				triangles[numtriangles].nindices[0] = n;
				fscanf(file, "%d//%d", &v, &n);
				triangles[numtriangles].vindices[1] = v;
				triangles[numtriangles].nindices[1] = n;
				fscanf(file, "%d//%d", &v, &n);
				triangles[numtriangles].vindices[2] = v;
				triangles[numtriangles].nindices[2] = n;
				numtriangles++;
				while (fscanf(file, "%d//%d", &v, &n) > 0) {
					triangles[numtriangles].vindices[0] = triangles[numtriangles - 1].vindices[0];
					triangles[numtriangles].nindices[0] = triangles[numtriangles - 1].nindices[0];
					triangles[numtriangles].vindices[1] = triangles[numtriangles - 1].vindices[2];
					triangles[numtriangles].nindices[1] = triangles[numtriangles - 1].nindices[2];
					triangles[numtriangles].vindices[2] = v;
					triangles[numtriangles].nindices[2] = n;
					numtriangles++;
				}
			}
			else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
				/* v/t/n */
				triangles[numtriangles].vindices[0] = v;
				triangles[numtriangles].nindices[0] = n;
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				triangles[numtriangles].vindices[1] = v;
				triangles[numtriangles].nindices[1] = n;
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				triangles[numtriangles].vindices[2] = v;
				triangles[numtriangles].nindices[2] = n;
				numtriangles++;
				while (fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
					triangles[numtriangles].vindices[0] = triangles[numtriangles - 1].vindices[0];
					triangles[numtriangles].nindices[0] = triangles[numtriangles - 1].nindices[0];
					triangles[numtriangles].vindices[1] = triangles[numtriangles - 1].vindices[2];
					triangles[numtriangles].nindices[1] = triangles[numtriangles - 1].nindices[2];
					triangles[numtriangles].vindices[2] = v;
					triangles[numtriangles].nindices[2] = n;
					numtriangles++;
				}
			}
			else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
				/* v/t */
				triangles[numtriangles].vindices[0] = v;
				fscanf(file, "%d/%d", &v, &t);
				triangles[numtriangles].vindices[1] = v;
				fscanf(file, "%d/%d", &v, &t);
				triangles[numtriangles].vindices[2] = v;
				numtriangles++;
				while (fscanf(file, "%d/%d", &v, &t) > 0) {
					triangles[numtriangles].vindices[0] = triangles[numtriangles - 1].vindices[0];
					triangles[numtriangles].vindices[1] = triangles[numtriangles - 1].vindices[2];
					triangles[numtriangles].vindices[2] = v;
					numtriangles++;
				}
			}
			else {
				/* v */
				sscanf(buf, "%d", &v);
				triangles[numtriangles].vindices[0] = v;
				fscanf(file, "%d", &v);
				triangles[numtriangles].vindices[1] = v;
				fscanf(file, "%d", &v);
				triangles[numtriangles].vindices[2] = v;
				numtriangles++;
				while (fscanf(file, "%d", &v) > 0) {
					triangles[numtriangles].vindices[0] = triangles[numtriangles - 1].vindices[0];
					triangles[numtriangles].vindices[1] = triangles[numtriangles - 1].vindices[2];
					triangles[numtriangles].vindices[2] = v;
					numtriangles++;
				}
			}
			break;

		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}
	fclose(file);

	//compute vertex normals
	if (numnormals == 0)
	{
		int numfacetnorms = numtriangles;
		GLfloat* facetnorms = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (numfacetnorms));

		GLfloat u[3];
		GLfloat v[3];

		for (int i = 0; i < numtriangles; i++) {
			triangles[i].findex = i;

			u[0] = vertices[3 * (triangles[i].vindices[1] - 1) + 0] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 0];
			u[1] = vertices[3 * (triangles[i].vindices[1] - 1) + 1] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 1];
			u[2] = vertices[3 * (triangles[i].vindices[1] - 1) + 2] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 2];

			v[0] = vertices[3 * (triangles[i].vindices[2] - 1) + 0] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 0];
			v[1] = vertices[3 * (triangles[i].vindices[2] - 1) + 1] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 1];
			v[2] = vertices[3 * (triangles[i].vindices[2] - 1) + 2] -
				vertices[3 * (triangles[i].vindices[0] - 1) + 2];

			glmCross(u, v, &facetnorms[3 * i]);
			glmNormalize(&facetnorms[3 * i]);
		}

		/* calculate the cosine of the angle (in degrees) */
		GLfloat cos_angle = cos(ANGLE_THRES * M_PI / 180.0);

		GLMnode** members;
		GLMnode*  node;
		GLMnode*  tail;
		GLfloat   average[3];
		GLuint avg;
		GLfloat   dot;

		normals = (GLfloat*)malloc(sizeof(GLfloat) * 9 * (numtriangles));

		/* allocate a structure that will hold a linked list of triangle
		indices for each vertex */
		members = (GLMnode**)malloc(sizeof(GLMnode*) * (numvertices));
		for (int i = 0; i < numvertices; i++)
			members[i] = NULL;


		/* for every triangle, create a node for each vertex in it */
		for (int i = 0; i < numtriangles; i++) {
			node = (GLMnode*)malloc(sizeof(GLMnode));
			node->index = i;
			node->next = members[triangles[i].vindices[0] - 1];
			members[triangles[i].vindices[0] - 1] = node;

			node = (GLMnode*)malloc(sizeof(GLMnode));
			node->index = i;
			node->next = members[triangles[i].vindices[1] - 1];
			members[triangles[i].vindices[1] - 1] = node;

			node = (GLMnode*)malloc(sizeof(GLMnode));
			node->index = i;
			node->next = members[triangles[i].vindices[2] - 1];
			members[triangles[i].vindices[2] - 1] = node;
		}

		/* calculate the average normal for each vertex */
		numnormals = 0;
		for (int i = 0; i < numvertices; i++) {
			/* calculate an average normal for this vertex by averaging the
			facet normal of every triangle this vertex is in */
			node = members[i];
			if (!node)
				fprintf(stderr, "glmVertexNormals(): vertex w/o a triangle\n");
			average[0] = 0.0; average[1] = 0.0; average[2] = 0.0;
			avg = 0;
			while (node) {
				/* only average if the dot product of the angle between the two
				facet normals is greater than the cosine of the threshold
				angle -- or, said another way, the angle between the two
				facet normals is less than (or equal to) the threshold angle */
				dot = glmDot(&facetnorms[3 * triangles[node->index].findex],
					&facetnorms[3 * triangles[members[i]->index].findex]);
				if (dot > cos_angle) {
					node->averaged = GL_TRUE;
					average[0] += facetnorms[3 * triangles[node->index].findex + 0];
					average[1] += facetnorms[3 * triangles[node->index].findex + 1];
					average[2] += facetnorms[3 * triangles[node->index].findex + 2];
					avg = 1;			/* we averaged at least one normal! */
				}
				else {
					node->averaged = GL_FALSE;
				}
				node = node->next;
			}

			if (avg) {
				/* normalize the averaged normal */
				glmNormalize(average);

				/* add the normal to the vertex normals list */
				normals[3 * numnormals + 0] = average[0];
				normals[3 * numnormals + 1] = average[1];
				normals[3 * numnormals + 2] = average[2];
				avg = numnormals;
				numnormals++;
			}

			/* set the normal of this vertex in each triangle it is in */
			node = members[i];
			while (node) {
				if (node->averaged) {
					/* if this node was averaged, use the average normal */
					if (triangles[node->index].vindices[0] == i + 1)
						triangles[node->index].nindices[0] = avg + 1;
					else if (triangles[node->index].vindices[1] == i + 1)
						triangles[node->index].nindices[1] = avg + 1;
					else if (triangles[node->index].vindices[2] == i + 1)
						triangles[node->index].nindices[2] = avg + 1;
				}
				else {
					/* if this node wasn't averaged, use the facet normal */
					normals[3 * numnormals + 0] = facetnorms[3 * triangles[node->index].findex + 0];
					normals[3 * numnormals + 1] = facetnorms[3 * triangles[node->index].findex + 1];
					normals[3 * numnormals + 2] = facetnorms[3 * triangles[node->index].findex + 2];
					if (triangles[node->index].vindices[0] == i + 1)
						triangles[node->index].nindices[0] = numnormals + 1;
					else if (triangles[node->index].vindices[1] == i + 1)
						triangles[node->index].nindices[1] = numnormals + 1;
					else if (triangles[node->index].vindices[2] == i + 1)
						triangles[node->index].nindices[2] = numnormals + 1;
					numnormals++;
				}
				node = node->next;
			}
		}

		/* free the member information */
		for (int i = 0; i < numvertices; i++) {
			node = members[i];
			while (node) {
				tail = node;
				node = node->next;
				free(tail);
			}
		}
		free(members);
		free(facetnorms);
	}

	m_vertices = (GLfloat*)malloc(sizeof(GLfloat) * 9 * numtriangles);
	m_normals = (GLfloat*)malloc(sizeof(GLfloat) * 9 * numtriangles);
	m_indices = (GLuint *)malloc(sizeof(GLuint) * 3 * numtriangles);
	m_numvertices = 3 * numtriangles;

	for (int i = 0; i < 3 * numtriangles; i++)
	{
		m_indices[i] = i;
	}

	for (int i = 0; i < numtriangles; i++)
	{
		m_vertices[i * 9 + 0] = vertices[3 * (triangles[i].vindices[0] - 1) + 0];
		m_vertices[i * 9 + 1] = vertices[3 * (triangles[i].vindices[0] - 1) + 1];
		m_vertices[i * 9 + 2] = vertices[3 * (triangles[i].vindices[0] - 1) + 2];

		m_vertices[i * 9 + 3] = vertices[3 * (triangles[i].vindices[1] - 1) + 0];
		m_vertices[i * 9 + 4] = vertices[3 * (triangles[i].vindices[1] - 1) + 1];
		m_vertices[i * 9 + 5] = vertices[3 * (triangles[i].vindices[1] - 1) + 2];

		m_vertices[i * 9 + 6] = vertices[3 * (triangles[i].vindices[2] - 1) + 0];
		m_vertices[i * 9 + 7] = vertices[3 * (triangles[i].vindices[2] - 1) + 1];
		m_vertices[i * 9 + 8] = vertices[3 * (triangles[i].vindices[2] - 1) + 2];

		if (numnormals > 0)
		{
			m_normals[i * 9 + 0] = normals[3 * (triangles[i].nindices[0] - 1) + 0];
			m_normals[i * 9 + 1] = normals[3 * (triangles[i].nindices[0] - 1) + 1];
			m_normals[i * 9 + 2] = normals[3 * (triangles[i].nindices[0] - 1) + 2];

			m_normals[i * 9 + 3] = normals[3 * (triangles[i].nindices[1] - 1) + 0];
			m_normals[i * 9 + 4] = normals[3 * (triangles[i].nindices[1] - 1) + 1];
			m_normals[i * 9 + 5] = normals[3 * (triangles[i].nindices[1] - 1) + 2];

			m_normals[i * 9 + 6] = normals[3 * (triangles[i].nindices[2] - 1) + 0];
			m_normals[i * 9 + 7] = normals[3 * (triangles[i].nindices[2] - 1) + 1];
			m_normals[i * 9 + 8] = normals[3 * (triangles[i].nindices[2] - 1) + 2];
		}
	}

	free(vertices);
	free(normals);
	free(triangles);

	VertexBuffer* buffer = new VertexBuffer();
	buffer->setData(m_numvertices, m_vertices, m_normals, 0, m_indices);
	free(m_vertices);
	free(m_normals);
	free(m_indices);
	
	return buffer;
}

