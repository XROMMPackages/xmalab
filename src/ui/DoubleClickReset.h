#ifndef DOUBLECLICKRESET_H_
#define DOUBLECLICKRESET_H_

#include <QObject>
#include <QEvent>
#include <QWidget>
#include <functional>

namespace xma
{
	// Runs a callback when the watched widget is double-clicked, e.g. to reset a
	// scale/bias control to its default. Owned by the widget it watches.
	class DoubleClickReset : public QObject
	{
	public:
		static void install(QWidget* widget, std::function<void()> onReset)
		{
			widget->installEventFilter(new DoubleClickReset(widget, std::move(onReset)));
			widget->setToolTip(QObject::tr("Double-click to reset"));
		}

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override
		{
			if (event->type() == QEvent::MouseButtonDblClick)
			{
				onReset();
				return true;
			}
			return QObject::eventFilter(watched, event);
		}

	private:
		DoubleClickReset(QWidget* parent, std::function<void()> callback)
			: QObject(parent), onReset(std::move(callback))
		{
		}

		std::function<void()> onReset;
	};
}

#endif // DOUBLECLICKRESET_H_
