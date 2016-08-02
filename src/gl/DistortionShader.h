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
///\file DistortionShader.h
///\author Benjamin Knorlein
///\date 7/28/2016

#ifndef DISTORTIONSHADER_H_
#define DISTORTIONSHADER_H_

#include "gl/Shader.h"
#include "gl/FrameBuffer.h"
#include <vector>
#include <QObject>
#include <QFutureWatcher>

namespace xma
{
	class Camera;
	class VertexBuffer;
	class DistortionShader : public QObject, public Shader, public FrameBuffer {

		Q_OBJECT;

	public :
		DistortionShader(Camera * camera);
		virtual ~DistortionShader();

		void draw(unsigned int texture_id, unsigned depth_id, float transparency);
		void setDistortionMap();
		bool canRender();

	private:
		VertexBuffer * m_vbo;
		Camera * m_camera;
		void intialiseVBO();

		static int nbInstances;
		static bool m_distortionComplete;
		
		bool m_distortionRunning;
		int m_numpoints;
		std::vector<float> m_vertices;
		std::vector <float> m_texcoords;
		std::vector <unsigned int> m_indices;

		QFutureWatcher<void>* m_FutureWatcher;
		bool stopped;
		QMutex loading;

	public slots:
		void loadComplete();
	};
}


#endif /* DISTORTIONSHADER_H_ */

