//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file MarkerTreeWidgetButton.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

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

MarkerTreeWidgetButton::MarkerTreeWidgetButton(QWidget* parent, int type, int idx) : QWidget(parent), m_type(type), m_idx(idx)
{
	init();
}

MarkerTreeWidgetButton::~MarkerTreeWidgetButton()
{
	delete settingsButton;
}

void MarkerTreeWidgetButton::init()
{
	settingsButton = new QToolButton();
	setButtonIcon();

	settingsButton->setAutoRaise(true);
	settingsButton->connect(settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
	QGridLayout* layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setVerticalSpacing(0);
	layout->setHorizontalSpacing(0);

	layout->addWidget(settingsButton, 0, 0);
	setLayout(layout);
}


void MarkerTreeWidgetButton::setButtonIcon()
{
	QIcon icon;
	if (m_type == 1)
	{
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx]->isReferencesSet() == 2)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx]->getHasOptimizedCoordinates())
			{
				icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_optimized.png"), QSize(), QIcon::Normal, QIcon::Off);
			}
			else{
				icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d.png"), QSize(), QIcon::Normal, QIcon::Off);
			}
		}
		else if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx]->isReferencesSet() == 1)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx]->getHasOptimizedCoordinates())
			{
				icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_setMarker_optimized.png"), QSize(), QIcon::Normal, QIcon::Off);
			}
			else{
				icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_setMarker.png"), QSize(), QIcon::Normal, QIcon::Off);
			}
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
	if (m_type == 1)
	{
		RigidBodyDialog* dialog = new RigidBodyDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[m_idx], MainWindow::getInstance());

		dialog->exec();
		MainWindow::getInstance()->redrawGL();
		setButtonIcon();
		delete dialog;
	}
	else if (m_type == 2)
	{
		MarkerDialog* dialog = new MarkerDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[m_idx], MainWindow::getInstance());

		dialog->exec();

		MainWindow::getInstance()->redrawGL();

		delete dialog;
	}
}

