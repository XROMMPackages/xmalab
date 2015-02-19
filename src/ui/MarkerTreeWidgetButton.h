#ifndef MARKERTREEWIDGETBUTTON_H
#define MARKERTREEWIDGETBUTTON_H

#include <QWidget>
#include "ui/MarkerTreeWidgetButton.h" 

class QToolButton;

namespace xma{
	class MarkerTreeWidgetButton : public QWidget
	{
		Q_OBJECT

	public:
		MarkerTreeWidgetButton(QWidget * parent, int type, int idx);
		~MarkerTreeWidgetButton();

	private:
		void init();
		void setButtonIcon();
		QToolButton* settingsButton;

		int m_type;
		int m_idx;

	protected:

		public slots :
			void settingsButtonClicked();
	};
}

#endif 