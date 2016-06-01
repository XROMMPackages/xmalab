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
///\file Image.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef IMAGE_H_
#define IMAGE_H_

#include <QString>
#include <opencv/cv.h>

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

namespace xma
{
	class Image
	{
	public:
		Image(QString imageFileName);
		Image(Image* _image);
		Image(cv::Mat& _image);
		virtual ~Image();

		void loadTexture();
		void bindTexture();
		void deleteTexture();

		int getWidth()
		{
			return width;
		}

		int getHeight()
		{
			return height;
		}

		void save(QString filename);
		void getImage(cv::Mat& image);
		void getSubImage(cv::Mat& _image, int size, int off_x, int off_y);
		void getSubImage(cv::Mat& _image, int size, double x, double y);
		void setImage(cv::Mat& image, bool _color = false);
		void setImage(QString imageFileName);

	private:
		cv::Mat image;
		int height, width;
		bool color;

		bool textureLoaded;
		bool image_reset;
		GLuint texture;
	};
}

#endif /* IMAGE_H_ */

