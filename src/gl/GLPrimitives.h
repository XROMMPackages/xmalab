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
///\file GLPrimitives.h
///\author XMALab Team
///\date 2024
///
/// Modern OpenGL 4.1 Core Profile primitives using VAO/VBO.
/// Replaces deprecated immediate mode (glBegin/glEnd) and gluSphere.

#ifndef GLPRIMITIVES_H_
#define GLPRIMITIVES_H_

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <vector>
#include <memory>

namespace xma
{
	/// Shader program for rendering colored primitives with optional lighting
	class GLPrimitiveShader
	{
	public:
		static GLPrimitiveShader* instance();
		
		void bind();
		void release();
		
		void setMVP(const QMatrix4x4& mvp);
		void setModelMatrix(const QMatrix4x4& model);
		void setColor(const QVector4D& color);
		void setColor(float r, float g, float b, float a = 1.0f);
		void setUseLighting(bool use);
		void setLightPosition(const QVector3D& pos);
		
		QOpenGLShaderProgram* program() { return &m_program; }
		
	private:
		GLPrimitiveShader();
		bool initialize();
		
		QOpenGLShaderProgram m_program;
		bool m_initialized;
		
		int m_mvpLoc;
		int m_modelLoc;
		int m_colorLoc;
		int m_useLightingLoc;
		int m_lightPosLoc;
	};

	/// Shader for textured quads
	class GLTextureShader
	{
	public:
		static GLTextureShader* instance();
		
		void bind();
		void release();
		
		void setMVP(const QMatrix4x4& mvp);
		void setTexture(int unit);
		void setColor(const QVector4D& color);
		
		QOpenGLShaderProgram* program() { return &m_program; }
		
	private:
		GLTextureShader();
		bool initialize();
		
		QOpenGLShaderProgram m_program;
		bool m_initialized;
		
		int m_mvpLoc;
		int m_texLoc;
		int m_colorLoc;
	};

	/// A sphere primitive using an icosphere mesh
	class GLSphere
	{
	public:
		static GLSphere* instance();
		
		/// Draw a sphere at the given position with given radius and color
		void draw(const QMatrix4x4& mvp, const QVector3D& position, float radius, 
		          const QVector4D& color, bool useLighting = true);
		
		/// Draw a sphere with a full model matrix
		void draw(const QMatrix4x4& mvp, const QMatrix4x4& model, const QVector4D& color, 
		          bool useLighting = true);
		
	private:
		GLSphere();
		void initialize();
		void generateIcosphere(int subdivisions);
		void subdivide(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, int depth);
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		QOpenGLBuffer m_ibo;
		
		std::vector<float> m_vertices;  // interleaved position + normal
		std::vector<unsigned int> m_indices;
		
		bool m_initialized;
		int m_indexCount;
	};

	/// A simple quad for 2D rendering (images, overlays)
	class GLQuad
	{
	public:
		static GLQuad* instance();
		
		/// Draw a textured quad with the given dimensions
		void drawTextured(const QMatrix4x4& mvp, float x, float y, float width, float height,
		                  bool flipY = false);
		
		/// Draw a colored quad
		void drawColored(const QMatrix4x4& mvp, float x, float y, float width, float height,
		                 const QVector4D& color);
		
		/// Draw a full-screen quad (for post-processing)
		void drawFullscreen(const QMatrix4x4& mvp);
		
	private:
		GLQuad();
		void initialize();
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		
		bool m_initialized;
	};

	/// Lines for drawing axes, connections, etc.
	class GLLines
	{
	public:
		GLLines();
		~GLLines();
		
		/// Set line data (pairs of vertices)
		void setData(const std::vector<float>& vertices);
		
		/// Draw all lines
		void draw(const QMatrix4x4& mvp, const QVector4D& color, float lineWidth = 1.0f);
		
		/// Draw a single line segment
		static void drawLine(const QMatrix4x4& mvp, 
		                     const QVector3D& start, const QVector3D& end,
		                     const QVector4D& color, float lineWidth = 1.0f);
		
		/// Draw coordinate axes
		static void drawAxes(const QMatrix4x4& mvp, float length, float lineWidth = 2.0f);
		
	private:
		void initialize();
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		
		bool m_initialized;
		int m_vertexCount;
	};

	/// Points for drawing markers
	class GLPoints
	{
	public:
		GLPoints();
		~GLPoints();
		
		/// Set point data
		void setData(const std::vector<float>& vertices);
		
		/// Draw all points
		void draw(const QMatrix4x4& mvp, const QVector4D& color, float pointSize = 5.0f);
		
	private:
		void initialize();
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		
		bool m_initialized;
		int m_vertexCount;
	};

	/// Line loop for drawing rectangles, polygons
	class GLLineLoop
	{
	public:
		GLLineLoop();
		~GLLineLoop();
		
		/// Set vertices for the loop
		void setData(const std::vector<float>& vertices);
		
		/// Draw the line loop
		void draw(const QMatrix4x4& mvp, const QVector4D& color, float lineWidth = 1.0f);
		
	private:
		void initialize();
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		
		bool m_initialized;
		int m_vertexCount;
	};

	/// Line renderer for dynamic line drawing (replaces immediate mode)
	class GLLineRenderer
	{
	public:
		GLLineRenderer();
		~GLLineRenderer();
		
		/// Add a line segment to the batch
		void addLine(const QVector3D& start, const QVector3D& end);
		void addLine(float x1, float y1, float z1, float x2, float y2, float z2);
		
		/// Render all accumulated lines (must have shader already bound with MVP set)
		void render();
		
		/// Clear accumulated lines
		void clear();
		
		/// Set line width for next render
		void setLineWidth(float width) { m_lineWidth = width; }
		
	private:
		void initialize();
		void updateBuffer();
		
		QOpenGLVertexArrayObject m_vao;
		QOpenGLBuffer m_vbo;
		
		std::vector<float> m_vertices;
		bool m_initialized;
		float m_lineWidth;
	};

	/// Main primitives manager singleton
	class GLPrimitives
	{
	public:
		static GLPrimitives* getInstance();
		
		GLSphere* getSphere() { return GLSphere::instance(); }
		GLLineRenderer* getLineRenderer() { return &m_lineRenderer; }
		GLQuad* getQuad() { return GLQuad::instance(); }
		
	private:
		GLPrimitives();
		~GLPrimitives();
		
		GLLineRenderer m_lineRenderer;
	};
}

#endif /* GLPRIMITIVES_H_ */
