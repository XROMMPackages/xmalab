//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file DetectionSettings.h
///\author Benjamin Knorlein
///\date 11/8/2016

#ifndef DETECTIONSETTINGS_H_
#define DETECTIONSETTINGS_H_

#include <QDialog>
#include <opencv2/opencv.hpp>

namespace Ui
{
	class DetectionSettings;
}

namespace xma
{
	class Marker;

	class DetectionSettings : public QDialog
	{
		Q_OBJECT

	public:
		virtual ~DetectionSettings() override;

		static DetectionSettings* getInstance();
		void setMarker(Marker * marker);
		void update(int camera, cv::Point2d center);
	protected:
		void closeEvent(QCloseEvent * e) override;
		void showEvent(QShowEvent * event) override;
	private:
		QPixmap getPixmap(cv::Mat inMat);

		Ui::DetectionSettings* diag;
		static DetectionSettings* instance;
		explicit DetectionSettings(QWidget* parent = nullptr);
		Marker* m_marker;
		std::vector <cv::Mat> m_images;
		cv::Point2d m_lastCenter;
		int m_lastCam;
		bool updating;
	public slots:
		void on_comboBox_currentIndexChanged(int index);
		void on_comboBox_Method_currentIndexChanged(int index);
		void activePointChanged(int idx);
		void on_spinBox_ThresholdOffset_valueChanged(int value);
		void on_checkBoxCrosshair_clicked(bool state);
	};
}


#endif /* DETECTIONSETTINGS_H_ */

