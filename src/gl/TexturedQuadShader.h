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
///\file TexturedQuadShader.h
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#ifndef TEXTUREDQUADSHADER_H_
#define TEXTUREDQUADSHADER_H_

#include "gl/Shader.h"
#include <QMatrix4x4>

namespace xma
{
    /**
     * Shader for textured quads.
     * Uses MVP matrix and texture sampler.
     */
    class TexturedQuadShader : public Shader
    {
    public:
        TexturedQuadShader();
        ~TexturedQuadShader() override;

        void bind();
        void release();

        void setMVP(const QMatrix4x4& mvp);
        void setTexture(GLuint textureId, int unit = 0);
        
        // Scale and bias for brightness/contrast adjustments
        void setScaleBias(float scale, float bias);

        // Draw a textured quad with specified corners
        // Coordinates are in the order: bottom-left, bottom-right, top-right, top-left
        void drawQuad(float x1, float y1, float x2, float y2);
        
        // Draw with full texture coordinates
        void drawQuadWithTexCoords(float x1, float y1, float x2, float y2,
                                   float tx1, float ty1, float tx2, float ty2);

    private:
        void initShader();
        void initQuadVAO();

        GLuint m_vao;
        GLuint m_vbo;
        bool m_vaoInitialized;

        int m_mvpLocation;
        int m_textureLocation;
        int m_scaleLocation;
        int m_biasLocation;
    };
}

#endif // TEXTUREDQUADSHADER_H_
