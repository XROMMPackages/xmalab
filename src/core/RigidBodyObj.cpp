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
///\file RigidBodyObj.cpp
///\author Benjamin Knorlein
///\date 07/23/2016
#include <GL/glew.h>


/// The contents on this file are based on the GLM Loader by Nate Robins
#include "gl/GLMLoader.h"
#include "core/RigidBodyObj.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
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

