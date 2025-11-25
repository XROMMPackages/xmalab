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
///\file MeshShader.h
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#ifndef MESHSHADER_H_
#define MESHSHADER_H_

#include "gl/Shader.h"
#include <QMatrix4x4>

namespace xma
{
    /**
     * Shader for rendering meshes with lighting (used by VertexBuffer/RigidBody).
     * Supports two-sided lighting for better mesh visibility.
     */
    class MeshShader : public Shader
    {
    public:
        MeshShader();
        ~MeshShader() override;

        void bind();
        void release();

        void setProjectionMatrix(const QMatrix4x4& projection);
        void setViewMatrix(const QMatrix4x4& view);
        void setModelMatrix(const QMatrix4x4& model);
        
        void setColor(float r, float g, float b, float a = 1.0f);
        void setColor(const QVector4D& color);
        
        // Light 1 (front light)
        void setLight1Position(const QVector3D& pos);
        void setLight1Ambient(const QVector3D& ambient);
        void setLight1Diffuse(const QVector3D& diffuse);
        
        // Light 2 (back light)
        void setLight2Position(const QVector3D& pos);
        void setLight2Ambient(const QVector3D& ambient);
        void setLight2Diffuse(const QVector3D& diffuse);

    private:
        void initShader();

        int m_projectionLocation;
        int m_viewLocation;
        int m_modelLocation;
        int m_normalMatrixLocation;
        int m_colorLocation;
        
        int m_light1PosLocation;
        int m_light1AmbientLocation;
        int m_light1DiffuseLocation;
        
        int m_light2PosLocation;
        int m_light2AmbientLocation;
        int m_light2DiffuseLocation;
    };
}

#endif // MESHSHADER_H_
