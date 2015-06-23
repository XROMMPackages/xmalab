#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MarkerTreeWidgetButton.h"
#include "ui/RigidBodyDialog.h"
#include "ui/MarkerDialog.h"
#include "ui/State.h"
#include "ui/MainWindow.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/RigidBody.h"

#include <QToolButton>
#include <QGridLayout>

using namespace xma;

MarkerTreeWidgetButton::MarkerTreeWidgetButton(QWidget * parent, int type, int idx) : QWidget(parent), m_type(type), m_idx(idx)
{
	init();
}

MarkerTreeWidgetButton::~MarkerTreeWidgetButton()
{
	delete settingsButton;
}

void MarkerTreeWidgetButton::init(){
	settingsButton = new QToolButton();
	setButtonIcon();

	settingsButton->setAutoRaise(true);
	settingsButton->connect(settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setVerticalSpacing(0);
	layout->setHorizontalSpacing(0);

	layout->addWidget(settingsButton, 0, 0);
	setLayout(layout);
}


void  MarkerTreeWidgetButton::setButtonIcon()
{
	QIcon icon;
	if (m_type == 1){
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx]->isReferencesSet()){
			icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d.png"), QSize(), QIcon::Normal, QIcon::Off);
		}
		else
		{
			icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_notSet.png"), QSize(), QIcon::Normal, QIcon::Off);
		}
	}
	else if (m_type == 2)
	{
		icon.addFile(QString::fromUtf8(":/images/resource-files/icons/settings.png"), QSize(), QIcon::Normal, QIcon::Off);
	}
	settingsButton->setIcon(icon);
}

void MarkerTreeWidgetButton::settingsButtonClicked()
{
	if (m_type == 1){
		RigidBodyDialog * dialog = new RigidBodyDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx], MainWindow::getInstance());

		dialog->exec();

		setButtonIcon();
	}
	else if (m_type == 2)
	{
		MarkerDialog * dialog = new MarkerDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[m_idx], MainWindow::getInstance());

		dialog->exec();
	}
}