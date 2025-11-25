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

/// The contents on this file are based on the GLM Loader by Nate Robins
#include "gl/GLMLoader.h"
#include "core/RigidBodyObj.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "gl/ShaderManager.h"
#include "gl/MeshShader.h"
#include <iostream>
#include <QMatrix4x4>

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

void RigidBodyObj::render(int frame, const QMatrix4x4& projection, const QMatrix4x4& view)
{
	if (m_body->getPoseComputed().size() <= (unsigned int)frame)
		return;

	if (m_body->getPoseComputed()[frame] || (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && m_body->getPoseFiltered()[frame])){
		bool filtered = Settings::getInstance()->getBoolSetting("TrialDrawFiltered");
		
		// Build the model transformation matrix
		double m[16];
		cv::Mat rotationMat;
		cv::Rodrigues(m_body->getRotationVector(filtered)[frame], rotationMat);
		for (unsigned int y = 0; y < 3; y++)
		{
			m[y * 4] = rotationMat.at<double>(y, 0);
			m[y * 4 + 1] = rotationMat.at<double>(y, 1);
			m[y * 4 + 2] = rotationMat.at<double>(y, 2);
			m[y * 4 + 3] = 0.0;
		}
		m[3] = 0.0; m[7] = 0.0; m[11] = 0.0;
		m[12] = m[0] * -m_body->getTranslationVector(filtered)[frame][0] + m[4] * -m_body->getTranslationVector(filtered)[frame][1] + m[8] * -m_body->getTranslationVector(filtered)[frame][2];
		m[13] = m[1] * -m_body->getTranslationVector(filtered)[frame][0] + m[5] * -m_body->getTranslationVector(filtered)[frame][1] + m[9] * -m_body->getTranslationVector(filtered)[frame][2];
		m[14] = m[2] * -m_body->getTranslationVector(filtered)[frame][0] + m[6] * -m_body->getTranslationVector(filtered)[frame][1] + m[10] * -m_body->getTranslationVector(filtered)[frame][2];
		m[15] = 1.0;
		
		// Create model matrix from the array (column-major)
		QMatrix4x4 modelMatrix(
			static_cast<float>(m[0]), static_cast<float>(m[4]), static_cast<float>(m[8]), static_cast<float>(m[12]),
			static_cast<float>(m[1]), static_cast<float>(m[5]), static_cast<float>(m[9]), static_cast<float>(m[13]),
			static_cast<float>(m[2]), static_cast<float>(m[6]), static_cast<float>(m[10]), static_cast<float>(m[14]),
			static_cast<float>(m[3]), static_cast<float>(m[7]), static_cast<float>(m[11]), static_cast<float>(m[15])
		);
		
		// Apply scale
		float scale = static_cast<float>(m_body->getMeshScale());
		modelMatrix.scale(scale, scale, scale);
		
		// Get the mesh shader and set up matrices
		MeshShader* meshShader = ShaderManager::getInstance()->getMeshShader();
		meshShader->bind();
		meshShader->setProjectionMatrix(projection);
		meshShader->setViewMatrix(view);
		meshShader->setModelMatrix(modelMatrix);
		
		// Set color from rigid body
		QColor bodyColor = m_body->getColor();
		meshShader->setColor(bodyColor.redF(), bodyColor.greenF(), bodyColor.blueF(), 1.0f);
		
		if (m_vbo) m_vbo->render();
		
		meshShader->release();
	}
}

