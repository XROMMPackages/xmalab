#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/Shortcuts.h"
#include "ui/MainWindow.h"
#include "ui/SequenceNavigationFrame.h"
#include "ui/PointsDockWidget.h"
#include "ui/WizardDockWidget.h"
#include <QtGui/QShortcut>

using namespace xma;

Shortcuts* Shortcuts::instance = NULL;

Shortcuts::Shortcuts(){
}

Shortcuts::~Shortcuts(){
	instance = NULL;
}

void Shortcuts::bindApplicationShortcuts()
{
	QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Left), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonPrev_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Right), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonNext_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Up), PointsDockWidget::getInstance(), SLOT(selectPrevPoint()));
	shortcut->setContext(Qt::ApplicationShortcut); 

	shortcut = new QShortcut(QKeySequence(Qt::Key_Down), PointsDockWidget::getInstance(), SLOT(selectNextPoint()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonStop_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_1), WizardDockWidget::getInstance(), SLOT(trackSelectedPointBackward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_2), WizardDockWidget::getInstance(), SLOT(trackSelectedPointForward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_3), WizardDockWidget::getInstance(), SLOT(goToLastTrackedFrame()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_3), WizardDockWidget::getInstance(), SLOT(goToFirstTrackedFrame()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Q), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonPrev_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_W), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonNext_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);
}

Shortcuts* Shortcuts::getInstance()
{
	if(!instance) 
	{
		instance = new Shortcuts();
	}
	return instance;
}

