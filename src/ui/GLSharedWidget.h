#ifndef GLSHAREDWIDGET_H
#define GLSHAREDWIDGET_H

#include <QGLWidget>

namespace xma{
	class GLSharedWidget : public QGLWidget
	{
		Q_OBJECT

	public:
		virtual ~GLSharedWidget();
		static GLSharedWidget* getInstance();
		const QGLContext* getQGLContext();
		void makeGLCurrent();

		public slots:

		bool getHasBlendExt();
		bool getHasBlendSubtract();

	protected:
		GLSharedWidget(QWidget *parent = NULL);
		void initializeGL();
		void paintEvent(QPaintEvent * event){}
		void resizeEvent(QResizeEvent * event){}

	private:
		static GLSharedWidget* instance;

		bool hasBlendSubtract;
		bool hasBlendExt;
	};
}


#endif /* GLSHAREDWIDGET_H */
