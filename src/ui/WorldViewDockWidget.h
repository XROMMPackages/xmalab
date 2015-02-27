#ifndef WORLDVIEWDOCKWIDGET_H_
#define WORLDVIEWDOCKWIDGET_H_

#include "ui/WorldViewDockGLWidget.h"
#include <QGridLayout>
#include <QDockWidget>

namespace xma{
	class WorldViewDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		WorldViewDockWidget(QWidget *parent);
		WorldViewDockGLWidget *openGL;

		void setSharedGLContext(const QGLContext * sharedContext);
		void draw();

	private:
		QGridLayout *layout;
	protected:
		void resizeEvent(QResizeEvent * event) override;
		void closeEvent(QCloseEvent *event) override;
	};
}


#endif /* PROGRESSDIALOG_H_ */
