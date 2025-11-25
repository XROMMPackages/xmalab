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
///\file LitColorShader.h
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#ifndef LITCOLORSHADER_H_
#define LITCOLORSHADER_H_

#include "gl/Shader.h"
#include <QMatrix4x4>

namespace xma
{
    class GLSphere;

    /**
     * Shader for lit geometry (spheres, meshes) with Phong-style lighting.
     * Uses MVP, model matrix, normal matrix, and light uniforms.
     */
    class LitColorShader : public Shader
    {
    public:
        LitColorShader();
        ~LitColorShader() override;

        void bind();
        void release();

        void setProjectionMatrix(const QMatrix4x4& projection);
        void setViewMatrix(const QMatrix4x4& view);
        void setModelMatrix(const QMatrix4x4& model);
        
        void setColor(float r, float g, float b, float a = 1.0f);
        void setColor(const QVector4D& color);
        
        void setLightPosition(const QVector3D& pos);
        void setAmbientColor(const QVector3D& ambient);
        void setDiffuseColor(const QVector3D& diffuse);

        // Draw a sphere at position with given radius
        void drawSphere(const QVector3D& position, float radius);

    private:
        void initShader();

        int m_projectionLocation;
        int m_viewLocation;
        int m_modelLocation;
        int m_normalMatrixLocation;
        int m_colorLocation;
        int m_lightPosLocation;
        int m_ambientLocation;
        int m_diffuseLocation;

        QMatrix4x4 m_projection;
        QMatrix4x4 m_view;
        QMatrix4x4 m_model;
    };
}

#endif // LITCOLORSHADER_H_
