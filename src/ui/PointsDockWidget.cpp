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
///\file PointsDockWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/PointsDockWidget.h"
#include "ui_PointsDockWidget.h"
#include "ui/MainWindow.h"
#include "ui/ImportExportPointsDialog.h"
#include "ui/State.h"
#include "ui/ConfirmationDialog.h"
#include "ui/WizardDockWidget.h"
#include "ui/PlotWindow.h"
#include "ui/MarkerTreeWidgetButton.h"
#include "ui/Shortcuts.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/Settings.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QToolButton>


using namespace xma;

PointsDockWidget* PointsDockWidget::instance = NULL;

PointsDockWidget::PointsDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::PointsDockWidget)
{
	dock->setupUi(this);

	qApp->installEventFilter(this);

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));

	Shortcuts::getInstance()->installEventFilterToChildren(this);
	dock->checkBoxDrawMarkerIds->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"));
	dock->checkBoxShowMarkerStatus->setChecked(Settings::getInstance()->getBoolSetting("ShowMarkerStates"));
}

PointsDockWidget::~PointsDockWidget()
{
	delete dock;
}

PointsDockWidget* PointsDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new PointsDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::LeftDockWidgetArea, instance);
	}
	return instance;
}

void PointsDockWidget::reset()
{
	int status_columns = (Settings::getInstance()->getBoolSetting("ShowMarkerStates")) ? Project::getInstance()->getCameras().size() : 0;
	dock->treeWidgetPoints->reset(status_columns);
}
void PointsDockWidget::addPointToList(int idx)
{
	QTreeWidgetItem* qtreewidgetitem = new QTreeWidgetItem(MARKER);
	qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDropEnabled);
	qtreewidgetitem->setText(0, QString::number(idx));
	dock->treeWidgetPoints->addTopLevelItem(qtreewidgetitem);
}

void PointsDockWidget::updateColor()
{
	if (!this->isVisible() || State::getInstance()->getActiveFrameTrial() < 0 || !(Settings::getInstance()->getBoolSetting("ShowMarkerStates")))
		return;

	std::vector<Marker *> markers = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers();
	int topCount = dock->treeWidgetPoints->topLevelItemCount();
	for (int i = 0; i < topCount; i++)
	{
		QTreeWidgetItem *item = dock->treeWidgetPoints->topLevelItem(i);
		if (item->type() == MARKER)
		{
			int idx = item->text(0).toInt() - 1;
			for (int c = 0; c < Project::getInstance()->getCameras().size(); c++){
				item->setBackgroundColor(2 + c, markers[idx]->getStatusColor(c, State::getInstance()->getActiveFrameTrial()));
			}
		}
		else
		{
			int childCount = item->childCount();
			for (int j = 0; j < childCount; j++) {
				QTreeWidgetItem *child = item->child(j);
				if (child->type() == MARKER)
				{
					int idx = child->text(0).toInt() - 1;
					for (int c = 0; c < Project::getInstance()->getCameras().size(); c++){
						child->setBackgroundColor(2 + c, markers[idx]->getStatusColor(c, State::getInstance()->getActiveFrameTrial()));			
					}
				}
			}
		}
	}
}

void PointsDockWidget::reloadListFromObject()
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		dock->treeWidgetPoints->clear();
		QTreeWidgetItem* qtreewidgetitem;
		int status_columns = (Settings::getInstance()->getBoolSetting("ShowMarkerStates")) ? Project::getInstance()->getCameras().size() : 0;
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
		{
			qtreewidgetitem = new QTreeWidgetItem(MARKER);
			dock->treeWidgetPoints->addTopLevelItem(qtreewidgetitem);
			qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDropEnabled);
			qtreewidgetitem->setText(0, QString::number(i + 1));
			qtreewidgetitem->setText(1, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[i]->getDescription());
			dock->treeWidgetPoints->setItemWidget(qtreewidgetitem, status_columns + 2, new MarkerTreeWidgetButton(dock->treeWidgetPoints, 2, i));
		}

		//Setup Rigid Body
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size(); i++)
		{
			qtreewidgetitem = new QTreeWidgetItem(RIGID_BODY);
			qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDragEnabled);
			dock->treeWidgetPoints->insertTopLevelItem(i, qtreewidgetitem);
			qtreewidgetitem->setText(0, "RB" + QString::number(i + 1));
			qtreewidgetitem->setText(1, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getDescription());
			dock->treeWidgetPoints->setItemWidget(qtreewidgetitem, status_columns + 2, new MarkerTreeWidgetButton(dock->treeWidgetPoints, 1, i));

			for (unsigned int k = 0; k < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getPointsIdx().size(); k++)
			{
				QList<QTreeWidgetItem *> items = dock->treeWidgetPoints->findItems(QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getPointsIdx()[k] + 1), Qt::MatchExactly | Qt::MatchRecursive, 0);
				if (items.size() > 0)
				{
					int ind = dock->treeWidgetPoints->indexOfTopLevelItem(items.at(0));
					QTreeWidgetItem* qtreewidgetitem2 = dock->treeWidgetPoints->takeTopLevelItem(ind);
					dock->treeWidgetPoints->removeItemWidget(qtreewidgetitem2, status_columns + 2);
					qtreewidgetitem->addChild(qtreewidgetitem2);
					dock->treeWidgetPoints->setItemWidget(qtreewidgetitem2, status_columns + 2, new MarkerTreeWidgetButton(dock->treeWidgetPoints, 2, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getPointsIdx()[k]));
				}
			}
			if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->isExpanded())dock->treeWidgetPoints->expandItem(qtreewidgetitem);
		}

		if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() >= 0 &&
			!selectPoint(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() + 1))
		{
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(-1);
		}
		if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveRBIdx() >= 0 &&
			!selectBody(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveRBIdx() + 1))
		{
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(-1);
		}
	}
	updateColor();
	WizardDockWidget::getInstance()->updateDialog();
}

void PointsDockWidget::selectNextPoint()
{
	std::cerr << "Select Next" << std::endl;
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && (int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			int idx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() + 1;
			if (idx >= 0 && idx < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
			{
				selectPoint(idx + 1);
			}
		}
	}
}

void PointsDockWidget::selectPrevPoint()
{
	std::cerr << "Select Prev" << std::endl;
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((Project::getInstance()->getTrials().size() > 0 && (int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0))
		{
			int idx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() - 1;
			if (idx >= 0 && idx < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
			{
				selectPoint(idx + 1);
			}
		}
	}
}


bool PointsDockWidget::selectPoint(int idx)
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		QList<QTreeWidgetItem *> items = dock->treeWidgetPoints->findItems(QString::number(idx), Qt::MatchExactly | Qt::MatchRecursive, 0);
		dock->treeWidgetPoints->clearSelection();
		if (items.size() > 0)
		{
			if (dock->treeWidgetPoints->currentItem() == items.at(0)) on_treeWidgetPoints_currentItemChanged(items.at(0), items.at(0));
			dock->treeWidgetPoints->setCurrentItem(items.at(0));
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool PointsDockWidget::selectBody(int idx)
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		QList<QTreeWidgetItem *> items = dock->treeWidgetPoints->findItems("RB" + QString::number(idx), Qt::MatchExactly | Qt::MatchRecursive, 0);
		dock->treeWidgetPoints->clearSelection();
		if (items.size() > 0)
		{
			if (dock->treeWidgetPoints->currentItem() == items.at(0)) on_treeWidgetPoints_currentItemChanged(items.at(0), items.at(0));
			dock->treeWidgetPoints->setCurrentItem(items.at(0));
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void PointsDockWidget::on_treeWidgetPoints_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		if (current && current->type() == MARKER)
		{
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(current->text(0).toInt() - 1);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(-1);
			emit activePointChanged(current->text(0).toInt() - 1);
			emit activeRigidBodyChanged(-1);

			if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
			{
				
			}
		}
		else if (current && current->type() == RIGID_BODY)
		{
			QString text = current->text(0);
			text.remove(0, 2);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(text.toInt() - 1);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(-1);
			emit activePointChanged(-1);
			emit activeRigidBodyChanged(text.toInt() - 1);
		}

		WizardDockWidget::getInstance()->updateDialog();
		MainWindow::getInstance()->redrawGL();
	}
}

void PointsDockWidget::on_pushButtonSetNumberMarkers_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Number of Markers to"),
	                               tr(""), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(), 0, 1000, 1, &ok);
	if (ok)
	{
		if ((int)Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() < idx)
		{
			while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() != idx)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();
			}
			PlotWindow::getInstance()->updateMarkers(true);
		}
		else
		{
			if (ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to delete Points " + QString::number(idx + 1) + " to " + QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()) + ". Are you sure you want to proceed?"))
			{
				while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() != idx)
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->removeMarker(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() - 1);
				}
			}
		}
	}
	reloadListFromObject();
}

void PointsDockWidget::on_pushButtonSetNumberRigidBodies_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Number of Rigid Bodies to"),
	                               tr(""), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size(), 0, 1000, 1, &ok);
	if (ok)
	{
		if ((int)Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() < idx)
		{
			while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() != idx)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addRigidBody();
			}
		}
		else
		{
			if (ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to delete Rigid Bodies " + QString::number(idx + 1) + " to " + QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size()) + ". Are you sure you want to proceed?"))
			{
				while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() != idx)
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->removeRigidBody(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() - 1);
				}
			}
		}
	}
	reloadListFromObject();
}

void PointsDockWidget::on_pushButtonImportExport_clicked()
{
	ImportExportPointsDialog* diag = new ImportExportPointsDialog(this);
	diag->exec();
	delete diag;
	reloadListFromObject();
}

void PointsDockWidget::on_checkBoxDrawMarkerIds_clicked()
{
	Settings::getInstance()->set("TrialDrawMarkerIds", dock->checkBoxDrawMarkerIds->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void PointsDockWidget::on_checkBoxShowMarkerStatus_clicked()
{
	Settings::getInstance()->set("ShowMarkerStates", dock->checkBoxShowMarkerStatus->isChecked());
	this->reset();
	this->reloadListFromObject();
}

void PointsDockWidget::activeTrialChanged(int activeTrial)
{
	if (activeTrial >= 0)
	{
		reset();
		reloadListFromObject();
	}
}

