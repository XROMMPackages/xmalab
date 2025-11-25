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
///\file SimpleColorShader.h
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#ifndef SIMPLECOLORSHADER_H_
#define SIMPLECOLORSHADER_H_

#include "gl/Shader.h"
#include <QMatrix4x4>

namespace xma
{
    /**
     * Simple shader for drawing solid-colored primitives (lines, points, triangles).
     * Uses MVP matrix and a uniform color.
     */
    class SimpleColorShader : public Shader
    {
    public:
        SimpleColorShader();
        ~SimpleColorShader() override;

        void bind();
        void release();

        void setMVP(const QMatrix4x4& mvp);
        void setColor(float r, float g, float b, float a = 1.0f);
        void setColor(const QVector4D& color);

        // Draw a line from start to end
        void drawLine(const QVector3D& start, const QVector3D& end);
        
        // Draw multiple lines (pairs of points)
        void drawLines(const std::vector<QVector3D>& points);
        
        // Draw a line loop
        void drawLineLoop(const std::vector<QVector3D>& points);
        
        // Draw a point
        void drawPoint(const QVector3D& point, float size = 1.0f);

    private:
        void initShader();
        void initLineVAO();

        GLuint m_lineVAO;
        GLuint m_lineVBO;
        bool m_lineVAOInitialized;

        int m_mvpLocation;
        int m_colorLocation;
    };
}

#endif // SIMPLECOLORSHADER_H_
