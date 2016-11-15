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
///\file MarkerTreeWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MarkerTreeWidget.h"
#include "ui/State.h"
#include "ui/MainWindow.h"
#include "ui/PointsDockWidget.h"
#include "ui/PlotWindow.h"
#include "ui/ConfirmationDialog.h"
#include "ui/ProgressDialog.h"
#include "ui/FromToDialog.h"
#include "ui/State.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"

#include <QMenu>
#include <QMouseEvent>
#include <QInputDialog>
#include <QHeaderView>
#include <QApplication>
#include <processing/MarkerDetection.h>

using namespace xma;

MarkerTreeWidget::MarkerTreeWidget(QWidget* parent): QTreeWidget(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDragDropMode(QAbstractItemView::InternalMove);
	connect(this,
	        SIGNAL(customContextMenuRequested(const QPoint&)),
	        SLOT(onCustomContextMenuRequested(const QPoint&)));

	connect(this,
	        SIGNAL(itemExpanded ( QTreeWidgetItem * )), this,
	        SLOT(onItemExpanded ( QTreeWidgetItem * )));

	connect(this,
	        SIGNAL(itemCollapsed ( QTreeWidgetItem * )), this,
	        SLOT(onItemCollapsed ( QTreeWidgetItem * )));

	action_ChangeDescription = new QAction(tr("&Change Description"), this);
	connect(action_ChangeDescription, SIGNAL(triggered()), this, SLOT(action_ChangeDescription_triggered()));

	action_CreateRigidBody = new QAction(tr("&Create Rigid Body from selected"), this);
	connect(action_CreateRigidBody, SIGNAL(triggered()), this, SLOT(action_CreateRigidBody_triggered()));

	action_AddToRigidBody = new QAction(tr("&Add selected to Rigid Body"), this);
	connect(action_AddToRigidBody, SIGNAL(triggered()), this, SLOT(action_AddToRigidBody_triggered()));

	action_DeletePoints = new QAction(tr("&Delete selected points"), this);
	connect(action_DeletePoints, SIGNAL(triggered()), this, SLOT(action_DeletePoints_triggered()));

	action_ResetPoints = new QAction(tr("&Delete Data from selected points"), this);
	connect(action_ResetPoints, SIGNAL(triggered()), this, SLOT(action_ResetPoints_triggered()));

	action_ChangeDetectionMethod = new QAction(tr("&Change Detectionmethod of selected Points"), this);
	connect(action_ChangeDetectionMethod, SIGNAL(triggered()), this, SLOT(action_ChangeDetectionMethod_triggered()));

	action_RefinePointsPolynomialFit = new QAction(tr("&Refine center of selected Points by polynomial fit"), this);
	connect(action_RefinePointsPolynomialFit, SIGNAL(triggered()), this, SLOT(action_RefinePointsPolynomialFit_triggered()));

	action_ChangePoint = new QAction(tr("&Change tracked data with data from another point"), this);
	connect(action_ChangePoint, SIGNAL(triggered()), this, SLOT(action_ChangePoint_triggered()));
	
	action_Save3D = new QAction(tr("&Save the 3D data of selected points"), this);
	connect(action_Save3D, SIGNAL(triggered()), this, SLOT(action_Save3D_triggered()));
	
	action_SaveTransformations = new QAction(tr("&Save the transformation of selected rigid bodies"), this);
	connect(action_SaveTransformations, SIGNAL(triggered()), this, SLOT(action_SaveTransformations_triggered()));

	header()->setResizeMode(0, QHeaderView::ResizeToContents);
	headerItem()->setText(0, "");

	for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		headerItem()->setText(2 + i, "");
		header()->setResizeMode(2 + i, QHeaderView::ResizeToContents);
	}
	
	headerItem()->setText(Project::getInstance()->getCameras().size() + 2, "");
	header()->setResizeMode(Project::getInstance()->getCameras().size() + 2, QHeaderView::ResizeToContents);

	headerItem()->setText(1, "");
	header()->setResizeMode(1, QHeaderView::Stretch);
	statusSlots = Project::getInstance()->getCameras().size();
}

void MarkerTreeWidget::reset(int markerStateColumns)
{
	this->setColumnCount(3 + markerStateColumns);

	header()->setResizeMode(0, QHeaderView::ResizeToContents);
	headerItem()->setText(0, "");
	statusSlots = markerStateColumns;
	for (int i = 0; i < markerStateColumns; i++)
	{
		headerItem()->setText(2 + i, "");
		header()->setResizeMode(2 + i, QHeaderView::ResizeToContents);
	}

	headerItem()->setText(markerStateColumns + 2, "");
	header()->setResizeMode(markerStateColumns + 2, QHeaderView::ResizeToContents);

	headerItem()->setText(1, "");
	header()->setResizeMode(1, QHeaderView::Stretch);
}

void MarkerTreeWidget::onCustomContextMenuRequested(const QPoint& pos)
{
	item_contextMenu = itemAt(pos);

	if (item_contextMenu)
	{
		// Note: We must map the point to global from the viewport to
		// account for the header.
		showContextMenu(item_contextMenu, viewport()->mapToGlobal(pos));
	}
}

void MarkerTreeWidget::showContextMenu(QTreeWidgetItem* item_contextMenu, const QPoint& globalPos)
{
	QMenu menu;

	switch (item_contextMenu->type())
	{
	case MARKER:
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault())menu.addAction(action_ChangeDescription);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault()) menu.addAction(action_CreateRigidBody);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault())menu.addAction(action_AddToRigidBody);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsDefault())menu.addAction(action_ResetPoints);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault())menu.addAction(action_DeletePoints);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault())menu.addAction(action_ChangeDetectionMethod);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsDefault())menu.addAction(action_RefinePointsPolynomialFit);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsDefault())menu.addAction(action_ChangePoint);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsDefault())menu.addAction(action_Save3D);

		break;

	case RIGID_BODY:
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsCopyFromDefault())menu.addAction(action_ChangeDescription);
		if (!Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getIsDefault())menu.addAction(action_SaveTransformations);
		break;
	}

	menu.exec(globalPos);
}

void MarkerTreeWidget::action_ChangeDescription_triggered()
{
	QString description = item_contextMenu->text(1);
	bool ok;
	description = QInputDialog::getText(this, tr("Enter description"),
	                                    tr("Description:"), QLineEdit::Normal,
	                                    description, &ok);
	if (ok)
	{
		item_contextMenu->setText(1, description);

		if (item_contextMenu->type() == MARKER)
		{
			int idx = item_contextMenu->text(0).toInt() - 1;
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->setDescription(description);
		}
		else if (item_contextMenu->type() == RIGID_BODY)
		{
			QString text = item_contextMenu->text(0);
			text.remove(0, 2);
			int idx = text.toInt() - 1;
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setDescription(description);
		}
	}
}

void MarkerTreeWidget::action_DeletePoints_triggered()
{
	QList<QTreeWidgetItem *> items = this->selectedItems();
	std::vector<int> pointsToDelete;
	if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete the selected Points?")) return;
	for (int i = 0; i < items.size(); i++)
	{
		if (items.at(i)->type() == MARKER)
		{
			if (items.at(i)->parent())
			{
				QString text_parent = items.at(i)->parent()->text(0);
				text_parent.remove(0, 2);
				int idx_parent = text_parent.toInt() - 1;
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx_parent]->removePointIdx(items.at(i)->text(0).toInt() - 1);
			}
			pointsToDelete.push_back(items.at(i)->text(0).toInt() - 1);
		}
	}
	std::sort(pointsToDelete.begin(), pointsToDelete.end());
	for (int i = pointsToDelete.size() - 1; i >= 0; i--)
	{
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->removeMarker(pointsToDelete[i]);
	}
	pointsToDelete.clear();
	PointsDockWidget::getInstance()->reloadListFromObject();
	PlotWindow::getInstance()->updateMarkers(true);
	MainWindow::getInstance()->redrawGL();
}

void MarkerTreeWidget::action_ChangeDetectionMethod_triggered()
{
	bool ok;
	QStringList methodnames;
	methodnames << "default Xray marker" << "Blobdetection" << "white marker" << "corner detection" << "no detection" << "xray marker (center of mass)" << "white Blobdetection";

	QString methodName = QInputDialog::getItem(this, tr("Choose method"),
	                                           tr("Method:"), methodnames, 0, false, &ok);

	if (ok && !methodName.isEmpty())
	{
		int method = methodnames.indexOf(methodName);
		QList<QTreeWidgetItem *> items = this->selectedItems();
		for (int i = 0; i < items.size(); i++)
		{
			if (items.at(i)->type() == MARKER)
			{
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(i)->text(0).toInt() - 1]->setMethod(method);
			}
		}
	}
}

void MarkerTreeWidget::action_RefinePointsPolynomialFit_triggered()
{
	QStringList methodnames;
	methodnames << "set from Mainwindow + tracked" << "only tracked" << "All (including set from detailwindow)";
	bool ok;

	QString methodName = QInputDialog::getItem(this, tr("Which points do you want to refine?"),
	                                           tr("Points :"), methodnames, 0, false, &ok);


	if (ok && !methodName.isEmpty())
	{
		FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame()
		                                        , Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame()
		                                        , Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages()
		                                        , false, this);

		bool ok2 = fromTo->exec();
		if (ok2)
		{
			int frame = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveFrame();
			int start = fromTo->getFrom() - 1;
			int end = fromTo->getTo() - 1;

			int method = methodnames.indexOf(methodName);
			markerStatus max_marker_status;

			if (method == 0)
			{
				max_marker_status = SET_AND_OPTIMIZED;
			}
			else if (method == 1)
			{
				max_marker_status = TRACKED_AND_OPTIMIZED;
			}
			else
			{
				max_marker_status = MANUAL_AND_OPTIMIZED;
			}

			QList<QTreeWidgetItem *> items = this->selectedItems();

			ProgressDialog::getInstance()->showProgressbar(start, end, "Refining center");
			for (int i = start; i <= end; i++)
			{
				ProgressDialog::getInstance()->setProgress(i);
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveFrame(i);
				xma::State::getInstance()->changeActiveFrameTrial(i);

				for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
				{
					for (int it = 0; it < items.size(); it++)
					{
						if (items.at(it)->type() == MARKER)
						{
							if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i] >= TRACKED &&
								Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i] <= max_marker_status)
							{
								cv::Point2d pt(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getPoints2D()[c][i]);
								double size = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getSize();
								int marker_method = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getMethod();

								if (size == -1)
								{
									size = 5;
								}

								bool success = MarkerDetection::refinePointPolynomialFit(pt, size, marker_method != 2 && marker_method != 6, c, xma::State::getInstance()->getActiveTrial());

								if (success){
									markerStatus status = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i];
									if (status == TRACKED) status = TRACKED_AND_OPTIMIZED;
									else if (status == SET) status = SET_AND_OPTIMIZED;
									else if (status == MANUAL) status = MANUAL_AND_OPTIMIZED;

									Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->setPoint(c, i, pt.x, pt.y, status);
									Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->setSize(c, i, size);
								}
							}
						}
					}
				}


				MainWindow::getInstance()->redrawGL();
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
			ProgressDialog::getInstance()->closeProgressbar();
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveFrame(frame);
			xma::State::getInstance()->changeActiveFrameTrial(frame);
		}
		delete fromTo;
	}
}

void MarkerTreeWidget::action_ChangePoint_triggered()
{
	if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers().size() < 2) 
		return;
	
	int idx = item_contextMenu->text(0).toInt() - 1;
	bool ok;
	QStringList idx2Names;
	for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
	{
		if (i != idx)
		{
			idx2Names << QString::number(i + 1);
		}
	}

	QString idx2string = QInputDialog::getItem(this, tr("Choose marker with which to exchange tracked data"),
		tr("Marker:"), idx2Names, 0, false, &ok);

	if (ok)
	{
		int idx2 = idx2string.toInt() - 1;

		bool ok2;
		QStringList cameraNames;
		cameraNames << "All";
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
			cameraNames << QString::number(i+1);
			
		QString cameraString = QInputDialog::getItem(this, tr("Which camera"),
			tr("Camera:"), cameraNames, 0, false, &ok2);

		if (ok2)
		{
			
			FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame()
				, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame()
				, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages()
				, false, this);

			bool ok3 = fromTo->exec();
			if (ok3)
			{
				int startFrame = fromTo->getFrom() - 1;
				int endFrame = fromTo->getTo() - 1;
				int startCamera = (cameraString == "All") ? 0 : cameraString.toInt() - 1;
				int endCamera = (cameraString == "All") ? Project::getInstance()->getCameras().size() - 1 : cameraString.toInt() - 1;
				std::cerr << startCamera << " " << endCamera;
				std::vector<RigidBody * > bodiesToUpdate;
				for (std::vector<RigidBody *>::const_iterator it = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().begin();
					it != Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().end(); ++it)
				{
					if ((std::find((*it)->getPointsIdx().begin(),
						(*it)->getPointsIdx().end(), idx)
						!= (*it)->getPointsIdx().end())
						||
						(std::find((*it)->getPointsIdx().begin(),
						(*it)->getPointsIdx().end(), idx2)
						!= (*it)->getPointsIdx().end()))
					{
						bodiesToUpdate.push_back((*it));
					}
				}
				for (int c = startCamera; c <= endCamera; c++)
				{
					for (int f = startFrame; f <= endFrame; f++)
					{
						double tmpx = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->getPoints2D()[c][f].x;
						double tmpy = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->getPoints2D()[c][f].y;
						markerStatus tmpStatus = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->getStatus2D()[c][f];
						
						Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->setPoint(c, f,
							Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getPoints2D()[c][f].x,
							Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getPoints2D()[c][f].y,
							Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus2D()[c][f]);
						
						Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->setPoint(c, f,tmpx,tmpy,tmpStatus);
					}
				}

				for (std::vector<RigidBody*>::iterator it = bodiesToUpdate.begin(); it != bodiesToUpdate.end(); ++it)
				{
					(*it)->recomputeTransformations();
					(*it)->filterTransformations();
				}
				bodiesToUpdate.clear();
				MainWindow::getInstance()->redrawGL();
			}
			delete fromTo;
		}
	}
}

void MarkerTreeWidget::action_Save3D_triggered()
{
	QList<QTreeWidgetItem *> items = this->selectedItems();
	std::vector<int> markers;
	for (int i = 0; i < items.size(); i++)
	{
		if (items.at(i)->type() == MARKER)
		{
			markers.push_back(items.at(i)->text(0).toInt() - 1);
		}
	}
	std::sort(markers.begin(), markers.end());
	MainWindow::getInstance()->save3DPoints(markers);
}

void MarkerTreeWidget::action_SaveTransformations_triggered()
{
	QList<QTreeWidgetItem *> items = this->selectedItems();
	std::vector<int> bodies;
	for (int i = 0; i < items.size(); i++)
	{
		if (items.at(i)->type() == RIGID_BODY)
		{
			bodies.push_back(items.at(i)->text(0).remove(0, 2).toInt() - 1);
		}
	}
	std::cerr << bodies[0] << std::endl;
	std::sort(bodies.begin(), bodies.end());
	MainWindow::getInstance()->saveRigidBodies(bodies);
}

void MarkerTreeWidget::action_ResetPoints_triggered()
{
	QList<QTreeWidgetItem *> items = this->selectedItems();

	FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame()
	                                        , Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame()
	                                        , Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages()
	                                        , false, this);

	bool ok = fromTo->exec();
	if (ok)
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset the data for the selected Points?")) return;
		for (int i = 0; i < items.size(); i++)
		{
			if (items.at(i)->type() == MARKER)
			{
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[(items.at(i)->text(0).toInt() - 1)]->resetMultipleFrames(-1,
				                                                                                                                                                        fromTo->getFrom() - 1, fromTo->getTo() - 1);
			}
		}
		MainWindow::getInstance()->redrawGL();
	}
	delete fromTo;
}

void MarkerTreeWidget::action_CreateRigidBody_triggered()
{
	QList<QTreeWidgetItem *> items = this->selectedItems();
	bool NodeCreated = false;
	QTreeWidgetItem* qtreewidgetitem;

	for (int i = 0; i < items.size(); i++)
	{
		if (!items.at(i)->parent() && items.at(i)->type() == MARKER)
		{
			if (!NodeCreated)
			{
				qtreewidgetitem = new QTreeWidgetItem(RIGID_BODY);
				qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDragEnabled);
				insertTopLevelItem(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size(), qtreewidgetitem);
				qtreewidgetitem->setText(0, "RB" + QString::number(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size() + 1));
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->addRigidBody();
				NodeCreated = true;
			}

			int ind = indexOfTopLevelItem(items.at(i));
			QTreeWidgetItem* qtreewidgetitem2 = takeTopLevelItem(ind);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size() - 1]->addPointIdx(qtreewidgetitem2->text(0).toInt() - 1);
			qtreewidgetitem->addChild(qtreewidgetitem2);
		}
	}
	PointsDockWidget::getInstance()->reloadListFromObject();
	PlotWindow::getInstance()->updateMarkers(true);
}

void MarkerTreeWidget::action_AddToRigidBody_triggered()
{
	QString description = "RB";
	bool ok;
	description = QInputDialog::getText(this, tr("Enter ID"),
	                                    tr("ID:"), QLineEdit::Normal,
	                                    description, &ok);
	if (ok)
	{
		QList<QTreeWidgetItem *> itemRigidBody = findItems(description, Qt::MatchExactly, 0);
		if (itemRigidBody.size() > 0 && itemRigidBody.at(0)->type() == RIGID_BODY)
		{
			QString text = itemRigidBody.at(0)->text(0);
			text.remove(0, 2);
			int idx = text.toInt() - 1;
			QList<QTreeWidgetItem *> items = this->selectedItems();
			QTreeWidgetItem* qtreewidgetitem = itemRigidBody.at(0);

			for (int i = 0; i < items.size(); i++)
			{
				if (!items.at(i)->parent() && items.at(i)->type() == MARKER)
				{
					int ind = indexOfTopLevelItem(items.at(i));
					QTreeWidgetItem* qtreewidgetitem2 = takeTopLevelItem(ind);
					qtreewidgetitem->addChild(qtreewidgetitem2);
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->addPointIdx(items.at(i)->text(0).toInt() - 1);
				}
				else if (items.at(i)->parent() && items.at(i)->type() == MARKER)
				{
					//remove from previous RB
					QString text_parent = items.at(i)->parent()->text(0);
					text_parent.remove(0, 2);
					int idx_parent = text_parent.toInt() - 1;
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx_parent]->removePointIdx(items.at(i)->text(0).toInt() - 1);

					//Set To new Parent
					int ind = items.at(i)->parent()->indexOfChild(items.at(i));
					QTreeWidgetItem* qtreewidgetitem2 = items.at(i)->parent()->takeChild(ind);
					qtreewidgetitem->addChild(qtreewidgetitem2);

					//Add to RB
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->addPointIdx(items.at(i)->text(0).toInt() - 1);
				}
			}
		}
	}
	PointsDockWidget::getInstance()->reloadListFromObject();
}

void MarkerTreeWidget::dropEvent(QDropEvent* event)
{
	QTreeWidgetItem* dropped = itemAt(event->pos());
	QList<QTreeWidgetItem *> dragged = selectedItems();

	for (int i = 0; i < dragged.size(); i ++)
	{
		int item_idx = dragged[i]->text(0).toInt() - 1;

		if (!dropped)
		{
			if (dragged[i]->parent())
			{
				//fprintf(stderr, "\Item %s dragged from %s to Noparent",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str());

				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0, 2);
				int parent_from_idx = text_parent_from.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);
			}
			else
			{
				//fprintf(stderr, "\Item %s dragged from Noparent to Noparent",dragged[i]->text(1).toStdString().c_str());
			}
		}
		else if (dropped->parent())
		{
			if (dragged[i]->parent())
			{
				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0, 2);
				int parent_from_idx = text_parent_from.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);

				QString text_parent_to = dropped->parent()->text(0);
				text_parent_to.remove(0, 2);
				int parent_to_idx = text_parent_to.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
			else
			{
				//fprintf(stderr, "\Item %s dragged from Noparent to %s",dragged[i]->text(1).toStdString().c_str(), dropped->parent()->text(1).toStdString().c_str());
				QString text_parent_to = dropped->parent()->text(0);
				text_parent_to.remove(0, 2);
				int parent_to_idx = text_parent_to.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
		}
		else if (dropped->type() == RIGID_BODY)
		{
			if (dragged[i]->parent())
			{
				//fprintf(stderr, "\Item %s dragged from %s to %s",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str(),dropped->text(1).toStdString().c_str());

				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0, 2);
				int parent_from_idx = text_parent_from.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);

				QString text_parent_to = dropped->text(0);
				text_parent_to.remove(0, 2);
				int parent_to_idx = text_parent_to.toInt() - 1;
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
			else
			{
				//fprintf(stderr, "\Item %s dragged from Noparent to %s",dragged[i]->text(1).toStdString().c_str(),dropped->text(1).toStdString().c_str());

				QString text_parent_to = dropped->text(0);
				text_parent_to.remove(0, 2);
				int parent_to_idx = text_parent_to.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
		}
		else
		{
			if (dragged[i]->parent())
			{
				//fprintf(stderr, "\Item %s dragged from %s to Noparent",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str());

				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0, 2);
				int parent_from_idx = text_parent_from.toInt() - 1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);
			}
			else
			{
				//fprintf(stderr, "\Item %s dragged from Noparent to Noparent",dragged[i]->text(1).toStdString().c_str());
			}
		}
	}
	QTreeWidget::dropEvent(event);
	PointsDockWidget::getInstance()->reloadListFromObject();
}

void MarkerTreeWidget::onItemCollapsed(QTreeWidgetItem* item)
{
	QString text = item->text(0);
	text.remove(0, 2);
	int idx = text.toInt() - 1;
	Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setExpanded(false);
}

void MarkerTreeWidget::onItemExpanded(QTreeWidgetItem* item)
{
	QString text = item->text(0);
	text.remove(0, 2);
	int idx = text.toInt() - 1;
	Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setExpanded(true);
}

