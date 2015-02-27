/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef DETAILVIEWDOCKWIDGET_H_
#define DETAILVIEWDOCKWIDGET_H_

#include <QDockWidget>
#include "ui/State.h"

namespace Ui {
	class DetailViewDockWidget;
}

namespace xma{
	class CameraViewDetailWidget;

	class DetailViewDockWidget : public QDockWidget{

		Q_OBJECT

	public:
		virtual ~DetailViewDockWidget();
		static DetailViewDockWidget* getInstance();

		void draw();
		void setup();
		void clear();
		void centerViews();
	protected:
		void closeEvent(QCloseEvent *event) override;

	private:
		static DetailViewDockWidget* instance;
		DetailViewDockWidget(QWidget *parent = 0);

		std::vector <CameraViewDetailWidget * > cameraViews;

		Ui::DetailViewDockWidget *dock;

		public slots:

	};
}


#endif /* DETAILVIEWDOCKWIDGET_H_ */

