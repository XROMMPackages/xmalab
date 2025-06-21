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
///\file CameraSelector.h
///\author Benjamin Knorlein
///\date 05/30/2017

#ifndef CAMERASELECTOR_H_
#define CAMERASELECTOR_H_

#include <QDialog>
#include <QString>

namespace Ui
{
	class CameraSelector;
}

class QCheckBox;

namespace xma
{
	class CameraSelector : public QDialog
	{
		Q_OBJECT

	private:
		Ui::CameraSelector* diag;
		std::vector<QCheckBox * > boxes;
	public:
		CameraSelector(QWidget* parent = 0);
		virtual ~CameraSelector();

	public slots:

	void on_pushButton_OK_clicked();
	void on_pushButton_Cancel_clicked();

	};
}

#endif /* CAMERASELECTOR_H_ */

