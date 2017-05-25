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
///\file CameraBox.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CAMERABOX_H_
#define CAMERABOX_H_

#include <QWidget>

namespace Ui
{
	class CameraBox;
}

namespace xma
{
	class CameraBox : public QWidget
	{
		Q_OBJECT

	private:
		QStringList imageFileNames;

	public:
		explicit CameraBox(QWidget* parent = 0);
		virtual ~CameraBox();

		Ui::CameraBox* widget;

		bool isComplete();

		const QStringList& getImageFileNames()
		{
			return imageFileNames;
		}

		const bool hasUndistortion();
		const QString getUndistortionGridFileName();
		const QString getCameraName();
		
		void setCameraName(QString name);
		bool isLightCamera();
		void setIsLightCamera();

		void addCalibrationImage(QString filename);
		void addUndistortionImage(QString filename);

	public slots:

		void on_toolButtonImages_clicked();
		void on_toolButtonUndistortionGrid_clicked();

		void on_radioButtonXRay_clicked();
		void on_radioButtonLightCamera_clicked();

		void on_checkBoxUndistortionGrid_stateChanged(int state);
	};
}

#endif /* CAMERABOX_H_ */

