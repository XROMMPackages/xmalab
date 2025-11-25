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
///\file GLFunctions.h
///\author XMALab Team
///\date 2024
///
/// Provides a common base for OpenGL 4.1 Core Profile functionality.
/// Replaces GLEW with Qt's QOpenGLFunctions_4_1_Core for cross-platform compatibility.

#ifndef GLFUNCTIONS_H_
#define GLFUNCTIONS_H_

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLContext>

namespace xma
{
	/// Mixin class providing access to OpenGL 4.1 Core Profile functions.
	/// Classes that need OpenGL access should inherit from this and call initGLFunctions()
	/// during their initializeGL() or when they have a valid GL context.
	class GLFunctions : protected QOpenGLFunctions_4_1_Core
	{
	protected:
		GLFunctions() : m_glInitialized(false) {}
		virtual ~GLFunctions() = default;

		/// Initialize OpenGL functions. Must be called with a valid GL context current.
		/// Returns true on success, false if no context is current.
		bool initGLFunctions() 
		{
			if (!QOpenGLContext::currentContext()) {
				return false;
			}
			if (!m_glInitialized) {
				m_glInitialized = initializeOpenGLFunctions();
			}
			return m_glInitialized;
		}

		/// Check if GL functions have been initialized
		bool glFunctionsInitialized() const { return m_glInitialized; }

		/// Check if there is a valid OpenGL context
		static bool hasValidContext() {
			return QOpenGLContext::currentContext() != nullptr;
		}

	private:
		bool m_glInitialized;
	};

	/// Singleton accessor for GL functions that can be used from non-widget classes.
	/// Must be called from a thread with a valid GL context.
	class GLFunctionsAccessor : public GLFunctions
	{
	public:
		static GLFunctionsAccessor* instance() {
			static GLFunctionsAccessor s_instance;
			return &s_instance;
		}

		/// Get initialized GL functions. Returns nullptr if no context.
		static QOpenGLFunctions_4_1_Core* gl() {
			auto* inst = instance();
			if (inst->initGLFunctions()) {
				return inst;
			}
			return nullptr;
		}

	private:
		GLFunctionsAccessor() = default;
	};
}

// Convenience macro for accessing GL functions
#define GL() (xma::GLFunctionsAccessor::gl())

#endif /* GLFUNCTIONS_H_ */
