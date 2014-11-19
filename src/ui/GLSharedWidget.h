#ifndef GLSHAREDWIDGET_H
#define GLSHAREDWIDGET_H

#include <QGLWidget>

class GLSharedWidget: public QGLWidget
{
    Q_OBJECT

public:
    ~GLSharedWidget();
	static GLSharedWidget* getInstance();
	const QGLContext* getQGLContext();
	void makeGLCurrent();

public slots:
	
protected:
	GLSharedWidget(QWidget *parent = NULL);
	void initializeGL();
	void paintEvent ( QPaintEvent * event ){}
	void resizeEvent ( QResizeEvent * event ){}

private:
	static GLSharedWidget* instance;
};



#endif /* GLSHAREDWIDGET_H */
