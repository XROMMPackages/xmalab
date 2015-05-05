#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QObject>

namespace xma{

	class Shortcuts : public QObject{

		Q_OBJECT

	public:
		static Shortcuts* getInstance();
		virtual ~Shortcuts();

		void bindApplicationShortcuts();

		void installEventFilterToChildren(QObject* object);
		bool checkShortcut(QObject *target, QEvent *event);

	protected:
		bool eventFilter(QObject *target, QEvent *event) override;

	private:
		Shortcuts();
		static Shortcuts* instance;

	};
}

	

#endif  // STATE_H
