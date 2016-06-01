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
///\file NewTrialDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef NEWTRAILDIALOG_H_
#define NEWTRAILDIALOG_H_

#include <QDialog>
#include <QFuture>

namespace Ui
{
	class NewTrialDialog;
}

namespace xma
{
	class CameraBoxTrial;
	class Trial;
	class NewTrialDialog : public QDialog
	{
		Q_OBJECT

	public:
		explicit NewTrialDialog(Trial * trial = NULL , QWidget* parent = 0);
		virtual ~NewTrialDialog();

		Ui::NewTrialDialog* diag;

		const std::vector<CameraBoxTrial *>& getCameras()
		{
			return cameras;
		}

		void setCam(int i, QString filename);
		void setTrialName(QString trialName);
		void setXmlMetadata(const QString& xml_metadata);

		bool createTrial();
		QString trialname;
	private:
		std::vector<CameraBoxTrial *> cameras;
		Trial * m_trial;
		//checks if all inputs are done
		bool isComplete();

		QFutureWatcher<void>* m_FutureWatcher;
		QString xml_metadata;
		QString xmaTrial_filename;
		bool deleteAfterLoad;

	public slots:
		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
		void on_pushButton_LoadXMA_clicked();

		void on_lineEditTrialName_textChanged(QString text);
		void LoadXMAFinished();
	};
}
#endif /* NEWTRAILDIALOG_H_ */

