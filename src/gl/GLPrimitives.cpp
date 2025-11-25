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
///\file GLPrimitives.cpp
///\author XMALab Team
///\date 2024

#include "gl/GLPrimitives.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <cmath>

using namespace xma;

// ============================================================================
// GLPrimitiveShader
// ============================================================================

static const char* primitiveVertexShader = R"(
#version 410 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_mvp;
uniform mat4 u_model;

out vec3 v_normal;
out vec3 v_worldPos;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = mat3(u_model) * a_normal;
    v_worldPos = (u_model * vec4(a_position, 1.0)).xyz;
}
)";

static const char* primitiveFragmentShader = R"(
#version 410 core
in vec3 v_normal;
in vec3 v_worldPos;

uniform vec4 u_color;
uniform bool u_useLighting;
uniform vec3 u_lightPos;

out vec4 fragColor;

void main()
{
    if (u_useLighting) {
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_worldPos);
        
        float ambient = 0.3;
        float diffuse = max(dot(normal, lightDir), 0.0) * 0.7;
        
        // Add some back-lighting
        float backLight = max(dot(normal, -lightDir), 0.0) * 0.2;
        
        float lighting = ambient + diffuse + backLight;
        fragColor = vec4(u_color.rgb * lighting, u_color.a);
    } else {
        fragColor = u_color;
    }
}
)";

GLPrimitiveShader* GLPrimitiveShader::instance()
{
	static GLPrimitiveShader s_instance;
	return &s_instance;
}

GLPrimitiveShader::GLPrimitiveShader()
	: m_initialized(false)
	, m_mvpLoc(-1)
	, m_modelLoc(-1)
	, m_colorLoc(-1)
	, m_useLightingLoc(-1)
	, m_lightPosLoc(-1)
{
}

bool GLPrimitiveShader::initialize()
{
	if (m_initialized) return true;
	if (!QOpenGLContext::currentContext()) return false;

	if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, primitiveVertexShader)) {
		qWarning() << "GLPrimitiveShader: Failed to compile vertex shader:" << m_program.log();
		return false;
	}
	if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, primitiveFragmentShader)) {
		qWarning() << "GLPrimitiveShader: Failed to compile fragment shader:" << m_program.log();
		return false;
	}
	if (!m_program.link()) {
		qWarning() << "GLPrimitiveShader: Failed to link program:" << m_program.log();
		return false;
	}

	m_mvpLoc = m_program.uniformLocation("u_mvp");
	m_modelLoc = m_program.uniformLocation("u_model");
	m_colorLoc = m_program.uniformLocation("u_color");
	m_useLightingLoc = m_program.uniformLocation("u_useLighting");
	m_lightPosLoc = m_program.uniformLocation("u_lightPos");

	m_initialized = true;
	return true;
}

void GLPrimitiveShader::bind()
{
	if (!m_initialized) initialize();
	m_program.bind();
}

void GLPrimitiveShader::release()
{
	m_program.release();
}

void GLPrimitiveShader::setMVP(const QMatrix4x4& mvp)
{
	m_program.setUniformValue(m_mvpLoc, mvp);
}

void GLPrimitiveShader::setModelMatrix(const QMatrix4x4& model)
{
	m_program.setUniformValue(m_modelLoc, model);
}

void GLPrimitiveShader::setColor(const QVector4D& color)
{
	m_program.setUniformValue(m_colorLoc, color);
}

void GLPrimitiveShader::setColor(float r, float g, float b, float a)
{
	setColor(QVector4D(r, g, b, a));
}

void GLPrimitiveShader::setUseLighting(bool use)
{
	m_program.setUniformValue(m_useLightingLoc, use);
}

void GLPrimitiveShader::setLightPosition(const QVector3D& pos)
{
	m_program.setUniformValue(m_lightPosLoc, pos);
}

// ============================================================================
// GLTextureShader
// ============================================================================

static const char* textureVertexShader = R"(
#version 410 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;

uniform mat4 u_mvp;

out vec2 v_texcoord;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
}
)";

static const char* textureFragmentShader = R"(
#version 410 core
in vec2 v_texcoord;

uniform sampler2D u_texture;
uniform vec4 u_color;

out vec4 fragColor;

void main()
{
    fragColor = texture(u_texture, v_texcoord) * u_color;
}
)";

GLTextureShader* GLTextureShader::instance()
{
	static GLTextureShader s_instance;
	return &s_instance;
}

GLTextureShader::GLTextureShader()
	: m_initialized(false)
	, m_mvpLoc(-1)
	, m_texLoc(-1)
	, m_colorLoc(-1)
{
}

bool GLTextureShader::initialize()
{
	if (m_initialized) return true;
	if (!QOpenGLContext::currentContext()) return false;

	if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexShader)) {
		qWarning() << "GLTextureShader: Failed to compile vertex shader:" << m_program.log();
		return false;
	}
	if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentShader)) {
		qWarning() << "GLTextureShader: Failed to compile fragment shader:" << m_program.log();
		return false;
	}
	if (!m_program.link()) {
		qWarning() << "GLTextureShader: Failed to link program:" << m_program.log();
		return false;
	}

	m_mvpLoc = m_program.uniformLocation("u_mvp");
	m_texLoc = m_program.uniformLocation("u_texture");
	m_colorLoc = m_program.uniformLocation("u_color");

	m_initialized = true;
	return true;
}

void GLTextureShader::bind()
{
	if (!m_initialized) initialize();
	m_program.bind();
}

void GLTextureShader::release()
{
	m_program.release();
}

void GLTextureShader::setMVP(const QMatrix4x4& mvp)
{
	m_program.setUniformValue(m_mvpLoc, mvp);
}

void GLTextureShader::setTexture(int unit)
{
	m_program.setUniformValue(m_texLoc, unit);
}

void GLTextureShader::setColor(const QVector4D& color)
{
	m_program.setUniformValue(m_colorLoc, color);
}

// ============================================================================
// GLSphere
// ============================================================================

GLSphere* GLSphere::instance()
{
	static GLSphere s_instance;
	return &s_instance;
}

GLSphere::GLSphere()
	: m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_ibo(QOpenGLBuffer::IndexBuffer)
	, m_initialized(false)
	, m_indexCount(0)
{
}

void GLSphere::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	generateIcosphere(3);  // 3 subdivisions gives ~1280 triangles, smooth enough

	m_vao.create();
	m_vao.bind();

	m_vbo.create();
	m_vbo.bind();
	m_vbo.allocate(m_vertices.data(), static_cast<int>(m_vertices.size() * sizeof(float)));

	m_ibo.create();
	m_ibo.bind();
	m_ibo.allocate(m_indices.data(), static_cast<int>(m_indices.size() * sizeof(unsigned int)));

	// Position attribute (location = 0)
	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);

	// Normal attribute (location = 1)
	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
	                         reinterpret_cast<void*>(3 * sizeof(float)));

	m_vao.release();
	m_indexCount = static_cast<int>(m_indices.size());
	m_initialized = true;
}

void GLSphere::generateIcosphere(int subdivisions)
{
	// Start with icosahedron
	const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

	std::vector<QVector3D> verts = {
		QVector3D(-1, t, 0).normalized(),
		QVector3D(1, t, 0).normalized(),
		QVector3D(-1, -t, 0).normalized(),
		QVector3D(1, -t, 0).normalized(),
		QVector3D(0, -1, t).normalized(),
		QVector3D(0, 1, t).normalized(),
		QVector3D(0, -1, -t).normalized(),
		QVector3D(0, 1, -t).normalized(),
		QVector3D(t, 0, -1).normalized(),
		QVector3D(t, 0, 1).normalized(),
		QVector3D(-t, 0, -1).normalized(),
		QVector3D(-t, 0, 1).normalized()
	};

	std::vector<std::array<int, 3>> faces = {
		{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
		{1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
		{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
		{4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
	};

	// Subdivide
	for (int i = 0; i < subdivisions; ++i) {
		std::vector<std::array<int, 3>> newFaces;
		std::map<std::pair<int, int>, int> midpointCache;

		auto getMidpoint = [&](int i1, int i2) -> int {
			auto key = std::minmax(i1, i2);
			auto it = midpointCache.find(key);
			if (it != midpointCache.end()) return it->second;

			QVector3D mid = ((verts[i1] + verts[i2]) / 2.0f).normalized();
			int idx = static_cast<int>(verts.size());
			verts.push_back(mid);
			midpointCache[key] = idx;
			return idx;
		};

		for (const auto& face : faces) {
			int a = getMidpoint(face[0], face[1]);
			int b = getMidpoint(face[1], face[2]);
			int c = getMidpoint(face[2], face[0]);

			newFaces.push_back({face[0], a, c});
			newFaces.push_back({face[1], b, a});
			newFaces.push_back({face[2], c, b});
			newFaces.push_back({a, b, c});
		}
		faces = std::move(newFaces);
	}

	// Convert to interleaved vertex data (position + normal, which are the same for unit sphere)
	m_vertices.clear();
	m_vertices.reserve(verts.size() * 6);
	for (const auto& v : verts) {
		m_vertices.push_back(v.x());
		m_vertices.push_back(v.y());
		m_vertices.push_back(v.z());
		m_vertices.push_back(v.x());  // normal = position for unit sphere
		m_vertices.push_back(v.y());
		m_vertices.push_back(v.z());
	}

	m_indices.clear();
	m_indices.reserve(faces.size() * 3);
	for (const auto& face : faces) {
		m_indices.push_back(static_cast<unsigned int>(face[0]));
		m_indices.push_back(static_cast<unsigned int>(face[1]));
		m_indices.push_back(static_cast<unsigned int>(face[2]));
	}
}

void GLSphere::draw(const QMatrix4x4& mvp, const QVector3D& position, float radius,
                    const QVector4D& color, bool useLighting)
{
	QMatrix4x4 model;
	model.translate(position);
	model.scale(radius);
	draw(mvp, model, color, useLighting);
}

void GLSphere::draw(const QMatrix4x4& mvp, const QMatrix4x4& model, const QVector4D& color,
                    bool useLighting)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	GLPrimitiveShader* shader = GLPrimitiveShader::instance();
	shader->bind();
	shader->setMVP(mvp * model);
	shader->setModelMatrix(model);
	shader->setColor(color);
	shader->setUseLighting(useLighting);
	shader->setLightPosition(QVector3D(0, 100, 100));

	m_vao.bind();
	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	m_vao.release();

	shader->release();
}

// ============================================================================
// GLQuad
// ============================================================================

GLQuad* GLQuad::instance()
{
	static GLQuad s_instance;
	return &s_instance;
}

GLQuad::GLQuad()
	: m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_initialized(false)
{
}

void GLQuad::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	// Quad vertices: position (3) + texcoord (2) = 5 floats per vertex
	// Two triangles forming a quad
	float vertices[] = {
		// Position         // TexCoord
		0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,   0.0f, 1.0f
	};

	m_vao.create();
	m_vao.bind();

	m_vbo.create();
	m_vbo.bind();
	m_vbo.allocate(vertices, sizeof(vertices));

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	// Position attribute
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
	// TexCoord attribute
	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
	                         reinterpret_cast<void*>(3 * sizeof(float)));

	m_vao.release();
	m_initialized = true;
}

void GLQuad::drawTextured(const QMatrix4x4& mvp, float x, float y, float width, float height,
                          bool flipY)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	QMatrix4x4 transform;
	transform.translate(x, y, 0);
	transform.scale(width, height, 1);

	GLTextureShader* shader = GLTextureShader::instance();
	shader->bind();
	shader->setMVP(mvp * transform);
	shader->setTexture(0);
	shader->setColor(QVector4D(1, 1, 1, 1));

	m_vao.bind();
	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glDrawArrays(GL_TRIANGLES, 0, 6);
	m_vao.release();

	shader->release();
}

void GLQuad::drawColored(const QMatrix4x4& mvp, float x, float y, float width, float height,
                         const QVector4D& color)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	QMatrix4x4 transform;
	transform.translate(x, y, 0);
	transform.scale(width, height, 1);

	GLPrimitiveShader* shader = GLPrimitiveShader::instance();
	shader->bind();
	shader->setMVP(mvp * transform);
	shader->setModelMatrix(transform);
	shader->setColor(color);
	shader->setUseLighting(false);

	m_vao.bind();
	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glDrawArrays(GL_TRIANGLES, 0, 6);
	m_vao.release();

	shader->release();
}

void GLQuad::drawFullscreen(const QMatrix4x4& mvp)
{
	drawTextured(mvp, -1, -1, 2, 2, false);
}

// ============================================================================
// GLLines
// ============================================================================

GLLines::GLLines()
	: m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_initialized(false)
	, m_vertexCount(0)
{
}

GLLines::~GLLines()
{
	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();
}

void GLLines::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	m_vao.create();
	m_vbo.create();
	m_initialized = true;
}

void GLLines::setData(const std::vector<float>& vertices)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	m_vao.bind();
	m_vbo.bind();
	m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	m_vao.release();
	m_vertexCount = static_cast<int>(vertices.size() / 3);
}

void GLLines::draw(const QMatrix4x4& mvp, const QVector4D& color, float lineWidth)
{
	if (!m_initialized || m_vertexCount == 0) return;

	GLPrimitiveShader* shader = GLPrimitiveShader::instance();
	shader->bind();
	shader->setMVP(mvp);
	shader->setModelMatrix(QMatrix4x4());
	shader->setColor(color);
	shader->setUseLighting(false);

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glLineWidth(lineWidth);

	m_vao.bind();
	f->glDrawArrays(GL_LINES, 0, m_vertexCount);
	m_vao.release();

	shader->release();
}

void GLLines::drawLine(const QMatrix4x4& mvp, const QVector3D& start, const QVector3D& end,
                       const QVector4D& color, float lineWidth)
{
	static GLLines s_tempLines;
	std::vector<float> verts = {
		start.x(), start.y(), start.z(),
		end.x(), end.y(), end.z()
	};
	s_tempLines.setData(verts);
	s_tempLines.draw(mvp, color, lineWidth);
}

void GLLines::drawAxes(const QMatrix4x4& mvp, float length, float lineWidth)
{
	// X axis - red
	drawLine(mvp, QVector3D(0, 0, 0), QVector3D(length, 0, 0), QVector4D(1, 0, 0, 1), lineWidth);
	// Y axis - green
	drawLine(mvp, QVector3D(0, 0, 0), QVector3D(0, length, 0), QVector4D(0, 1, 0, 1), lineWidth);
	// Z axis - blue
	drawLine(mvp, QVector3D(0, 0, 0), QVector3D(0, 0, length), QVector4D(0, 0, 1, 1), lineWidth);
}

// ============================================================================
// GLPoints
// ============================================================================

GLPoints::GLPoints()
	: m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_initialized(false)
	, m_vertexCount(0)
{
}

GLPoints::~GLPoints()
{
	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();
}

void GLPoints::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	m_vao.create();
	m_vbo.create();
	m_initialized = true;
}

void GLPoints::setData(const std::vector<float>& vertices)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	m_vao.bind();
	m_vbo.bind();
	m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	m_vao.release();
	m_vertexCount = static_cast<int>(vertices.size() / 3);
}

void GLPoints::draw(const QMatrix4x4& mvp, const QVector4D& color, float pointSize)
{
	if (!m_initialized || m_vertexCount == 0) return;

	GLPrimitiveShader* shader = GLPrimitiveShader::instance();
	shader->bind();
	shader->setMVP(mvp);
	shader->setModelMatrix(QMatrix4x4());
	shader->setColor(color);
	shader->setUseLighting(false);

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	// Note: glPointSize may be clamped. For larger points, use geometry shader or sprites.
	// For now we set it and hope for the best.
	
	m_vao.bind();
	f->glDrawArrays(GL_POINTS, 0, m_vertexCount);
	m_vao.release();

	shader->release();
}

// ============================================================================
// GLLineLoop
// ============================================================================

GLLineLoop::GLLineLoop()
	: m_vbo(QOpenGLBuffer::VertexBuffer)
	, m_initialized(false)
	, m_vertexCount(0)
{
}

GLLineLoop::~GLLineLoop()
{
	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();
}

void GLLineLoop::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	m_vao.create();
	m_vbo.create();
	m_initialized = true;
}

void GLLineLoop::setData(const std::vector<float>& vertices)
{
	if (!m_initialized) initialize();
	if (!m_initialized) return;

	m_vao.bind();
	m_vbo.bind();
	m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	m_vao.release();
	m_vertexCount = static_cast<int>(vertices.size() / 3);
}

void GLLineLoop::draw(const QMatrix4x4& mvp, const QVector4D& color, float lineWidth)
{
	if (!m_initialized || m_vertexCount == 0) return;

	GLPrimitiveShader* shader = GLPrimitiveShader::instance();
	shader->bind();
	shader->setMVP(mvp);
	shader->setModelMatrix(QMatrix4x4());
	shader->setColor(color);
	shader->setUseLighting(false);

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glLineWidth(lineWidth);

	m_vao.bind();
	f->glDrawArrays(GL_LINE_LOOP, 0, m_vertexCount);
	m_vao.release();

	shader->release();
}

// ============================================================================
// GLLineRenderer
// ============================================================================

GLLineRenderer::GLLineRenderer()
	: m_initialized(false)
	, m_lineWidth(1.0f)
{
}

GLLineRenderer::~GLLineRenderer()
{
	if (m_vao.isCreated()) m_vao.destroy();
	if (m_vbo.isCreated()) m_vbo.destroy();
}

void GLLineRenderer::initialize()
{
	if (m_initialized) return;
	if (!QOpenGLContext::currentContext()) return;

	m_vao.create();
	m_vbo.create();
	m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	m_initialized = true;
}

void GLLineRenderer::addLine(const QVector3D& start, const QVector3D& end)
{
	m_vertices.push_back(start.x());
	m_vertices.push_back(start.y());
	m_vertices.push_back(start.z());
	m_vertices.push_back(end.x());
	m_vertices.push_back(end.y());
	m_vertices.push_back(end.z());
}

void GLLineRenderer::addLine(float x1, float y1, float z1, float x2, float y2, float z2)
{
	m_vertices.push_back(x1);
	m_vertices.push_back(y1);
	m_vertices.push_back(z1);
	m_vertices.push_back(x2);
	m_vertices.push_back(y2);
	m_vertices.push_back(z2);
}

void GLLineRenderer::updateBuffer()
{
	if (!m_initialized) initialize();
	if (!m_initialized || m_vertices.empty()) return;

	m_vao.bind();
	m_vbo.bind();
	m_vbo.allocate(m_vertices.data(), static_cast<int>(m_vertices.size() * sizeof(float)));

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	m_vao.release();
}

void GLLineRenderer::render()
{
	if (m_vertices.empty()) return;

	updateBuffer();

	QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
	f->glLineWidth(m_lineWidth);

	m_vao.bind();
	f->glDrawArrays(GL_LINES, 0, static_cast<int>(m_vertices.size() / 3));
	m_vao.release();
}

void GLLineRenderer::clear()
{
	m_vertices.clear();
}

// ============================================================================
// GLPrimitives Singleton
// ============================================================================

GLPrimitives* GLPrimitives::getInstance()
{
	static GLPrimitives s_instance;
	return &s_instance;
}

GLPrimitives::GLPrimitives()
{
}

GLPrimitives::~GLPrimitives()
{
}
