#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/Shortcuts.h"
#include "ui/MainWindow.h"
#include "ui/SequenceNavigationFrame.h"
#include "ui/PointsDockWidget.h"
#include "ui/WizardDockWidget.h"
#include <QtGui/QShortcut>
#include <QKeyEvent>

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

void Shortcuts::installEventFilterToChildren(QObject* object)
{
	QObjectList list = object->children();
	for (int i = 0; i < list.size(); i++)
	{
		installEventFilterToChildren(list.at(i));
	}

	object->installEventFilter(this);
}

bool Shortcuts::checkShortcut(QObject* target, QEvent* event)
{
	return eventFilter(target, event);
}

bool Shortcuts::eventFilter(QObject *target, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *_keyEvent = static_cast<QKeyEvent*>(event);
		if (_keyEvent->key() == Qt::Key_Left)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonPrev_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Right)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonNext_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Up)
		{
			PointsDockWidget::getInstance()->selectPrevPoint();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Down)
		{
			PointsDockWidget::getInstance()->selectNextPoint();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Escape)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonStop_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_1)
		{
			WizardDockWidget::getInstance()->trackSelectedPointBackward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_2)
		{
			WizardDockWidget::getInstance()->trackSelectedPointForward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_3 && !_keyEvent->modifiers().testFlag(Qt::ShiftModifier))
		{
			WizardDockWidget::getInstance()->goToLastTrackedFrame();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_3 && _keyEvent->modifiers().testFlag(Qt::ShiftModifier))
		{
			WizardDockWidget::getInstance()->goToFirstTrackedFrame();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Q)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonPrev_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_W)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonNext_clicked();
			return true;
		}
	}
	return false;
}

Shortcuts* Shortcuts::getInstance()
{
	if(!instance) 
	{
		instance = new Shortcuts();
	}
	return instance;
}

