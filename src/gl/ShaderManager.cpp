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
///\file ShaderManager.cpp
///\author OpenGL 4.1 Core Profile migration
///\date 2024

#include "gl/ShaderManager.h"
#include "gl/SimpleColorShader.h"
#include "gl/LitColorShader.h"
#include "gl/TexturedQuadShader.h"
#include "gl/MeshShader.h"

using namespace xma;

ShaderManager* ShaderManager::instance = nullptr;

ShaderManager::ShaderManager()
    : m_simpleColorShader(nullptr)
    , m_litColorShader(nullptr)
    , m_texturedQuadShader(nullptr)
    , m_meshShader(nullptr)
    , m_initialized(false)
{
}

ShaderManager::~ShaderManager()
{
    delete m_simpleColorShader;
    delete m_litColorShader;
    delete m_texturedQuadShader;
    delete m_meshShader;
}

ShaderManager* ShaderManager::getInstance()
{
    if (!instance) {
        instance = new ShaderManager();
    }
    return instance;
}

void ShaderManager::initializeShaders()
{
    if (m_initialized) return;
    
    m_simpleColorShader = new SimpleColorShader();
    m_litColorShader = new LitColorShader();
    m_texturedQuadShader = new TexturedQuadShader();
    m_meshShader = new MeshShader();
    
    m_initialized = true;
}

SimpleColorShader* ShaderManager::getSimpleColorShader()
{
    if (!m_initialized) initializeShaders();
    return m_simpleColorShader;
}

LitColorShader* ShaderManager::getLitColorShader()
{
    if (!m_initialized) initializeShaders();
    return m_litColorShader;
}

TexturedQuadShader* ShaderManager::getTexturedQuadShader()
{
    if (!m_initialized) initializeShaders();
    return m_texturedQuadShader;
}

MeshShader* ShaderManager::getMeshShader()
{
    if (!m_initialized) initializeShaders();
    return m_meshShader;
}
