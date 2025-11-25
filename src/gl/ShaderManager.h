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
///\file ShaderManager.h
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#ifndef SHADERMANAGER_H_
#define SHADERMANAGER_H_

namespace xma
{
    class SimpleColorShader;
    class LitColorShader;
    class TexturedQuadShader;
    class MeshShader;

    /**
     * Singleton manager for shared shader instances.
     * Shaders are created once and shared across all OpenGL widgets.
     */
    class ShaderManager
    {
    public:
        static ShaderManager* getInstance();
        
        // Get shader instances - creates if needed
        SimpleColorShader* getSimpleColorShader();
        LitColorShader* getLitColorShader();
        TexturedQuadShader* getTexturedQuadShader();
        MeshShader* getMeshShader();

        // Initialize all shaders (call after OpenGL context is ready)
        void initializeShaders();
        
        // Check if shaders are initialized
        bool isInitialized() const { return m_initialized; }

    private:
        ShaderManager();
        ~ShaderManager();
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        static ShaderManager* instance;

        SimpleColorShader* m_simpleColorShader;
        LitColorShader* m_litColorShader;
        TexturedQuadShader* m_texturedQuadShader;
        MeshShader* m_meshShader;
        
        bool m_initialized;
    };
}

#endif // SHADERMANAGER_H_
