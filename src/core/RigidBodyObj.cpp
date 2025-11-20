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
///\file RigidBodyObj.cpp
///\author Benjamin Knorlein
///\date 07/23/2016
#include "gl/MeshShader.h"
#include "gl/VertexBuffer.h"
#include "core/RigidBodyObj.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include <iostream>
#include <QMatrix4x4>

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

RigidBodyObj::RigidBodyObj(QString filename, RigidBody* body) : m_filename(filename), m_body(body), m_vbo(NULL)
{
	m_vbo = GLMLoader::load(m_filename);
}

QString RigidBodyObj::getFilename()
{
	return m_filename;
}

RigidBodyObj::~RigidBodyObj()
{
	if (m_vbo)
	{
		delete m_vbo;
		m_vbo = NULL;
	}
}

bool RigidBodyObj::vboSet()
{
	return m_vbo != NULL;
}

void RigidBodyObj::render(int frame)
{
	if (m_body->getPoseComputed().size() <= frame)
		return;

	if (m_body->getPoseComputed()[frame] || (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && m_body->getPoseFiltered()[frame])){
		bool filtered = Settings::getInstance()->getBoolSetting("TrialDrawFiltered");
		
		glPushMatrix();

		glColor4f(m_body->getColor().redF(), m_body->getColor().greenF(), m_body->getColor().blueF(), 1.0);

		double m[16];
		//inversere Rotation = transposed rotation
		//and opengl requires transposed, so we set R

		cv::Mat rotationMat;
		cv::Rodrigues(m_body->getRotationVector(filtered)[frame], rotationMat);
		for (unsigned int y = 0; y < 3; y++)
		{
			m[y * 4] = rotationMat.at<double>(y, 0);
			m[y * 4 + 1] = rotationMat.at<double>(y, 1);
			m[y * 4 + 2] = rotationMat.at<double>(y, 2);
			m[y * 4 + 3] = 0.0;
		}
		//inverse translation = translation rotated with inverse rotation/transposed rotation
		//R-1 * -t = R^tr * -t
		m[12] = m[0] * -m_body->getTranslationVector(filtered)[frame][0] + m[4] * -m_body->getTranslationVector(filtered)[frame][1] + m[8] * -m_body->getTranslationVector(filtered)[frame][2];
		m[13] = m[1] * -m_body->getTranslationVector(filtered)[frame][0] + m[5] * -m_body->getTranslationVector(filtered)[frame][1] + m[9] * -m_body->getTranslationVector(filtered)[frame][2];
		m[14] = m[2] * -m_body->getTranslationVector(filtered)[frame][0] + m[6] * -m_body->getTranslationVector(filtered)[frame][1] + m[10] * -m_body->getTranslationVector(filtered)[frame][2];
		m[15] = 1.0;
		
		glMultMatrixd(m);
		glEnable(GL_NORMALIZE);
		glScaled(m_body->getMeshScale(), m_body->getMeshScale(), m_body->getMeshScale());
		if (m_vbo) m_vbo->render();

		glDisable(GL_NORMALIZE);
		glPopMatrix();
	}
}

void RigidBodyObj::renderGL(int frame, MeshShader* shader, const QMatrix4x4& vp, bool filtered)
{
	if (m_body->getPoseComputed().size() <= frame)
		return;

	if (m_body->getPoseComputed()[frame] || (filtered && m_body->getPoseFiltered()[frame])) {
		
		// Compute Model Matrix (Replicating legacy logic: M = (T*R)^-1 = R^T * T^-1)
		QMatrix4x4 m;
		
		cv::Vec3d rot = m_body->getRotationVector(filtered)[frame];
		cv::Mat rotationMat;
		cv::Rodrigues(rot, rotationMat);

		// Fill m with R^T (which is R^-1)
		// OpenGL is column-major. QMatrix4x4(row, col).
		// Legacy m[y*4 + x] is column x, row y.
		// Legacy m[0] = R(0,0). m[1] = R(0,1).
		// So m's first column is R's first row.
		// So m is R^T.
		for (int r = 0; r < 3; ++r) {
			for (int c = 0; c < 3; ++c) {
				m(c, r) = rotationMat.at<double>(r, c); 
			}
		}
		
		cv::Vec3d trans = m_body->getTranslationVector(filtered)[frame];
		double tx = trans[0];
		double ty = trans[1];
		double tz = trans[2];
		
		// Translation part: R^T * -t
		m(0, 3) = m(0, 0) * -tx + m(0, 1) * -ty + m(0, 2) * -tz;
		m(1, 3) = m(1, 0) * -tx + m(1, 1) * -ty + m(1, 2) * -tz;
		m(2, 3) = m(2, 0) * -tx + m(2, 1) * -ty + m(2, 2) * -tz;
		m(3, 3) = 1.0;
		
		// Scale
		double s = m_body->getMeshScale();
		QMatrix4x4 scaleMat;
		scaleMat.scale(s);
		
		// Final MVP = VP * M * S
		QMatrix4x4 mvp = vp * m * scaleMat;
		
		shader->setMVP(mvp);
		shader->setColor(m_body->getColor(), 0.5f); // Semi-transparent as requested
		
		if (m_vbo) {
			if (m_vbo->bindVAO()) {
				glDrawElements(GL_TRIANGLES, m_vbo->getNumVertices(), GL_UNSIGNED_INT, 0);
				m_vbo->releaseVAO();
			}
		}
	}
}

