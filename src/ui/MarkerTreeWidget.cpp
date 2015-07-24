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

MarkerTreeWidget::MarkerTreeWidget(QWidget * parent):QTreeWidget(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
    setDragEnabled(true);
	setAcceptDrops(true);
	setDragDropMode(QAbstractItemView::InternalMove);
    connect(this,
                SIGNAL(customContextMenuRequested(const QPoint&)),
                SLOT(onCustomContextMenuRequested(const QPoint&)));	

	connect(this,
                SIGNAL(itemExpanded ( QTreeWidgetItem * )),this,
                SLOT(onItemExpanded ( QTreeWidgetItem * )));	

	connect(this,
                SIGNAL(itemCollapsed ( QTreeWidgetItem * )),this,
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


	headerItem()->setText(2, "");
	//headerItem()->setText(3, "");

	header()->setResizeMode(0, QHeaderView::ResizeToContents);
	header()->setResizeMode(1, QHeaderView::Stretch);
	header()->setResizeMode(2, QHeaderView::ResizeToContents);

	//setColumnWidth(2, 25);
} 

void MarkerTreeWidget::onCustomContextMenuRequested(const QPoint& pos) {

    item_contextMenu = itemAt(pos);
 
    if (item_contextMenu) {
        // Note: We must map the point to global from the viewport to
        // account for the header.
        showContextMenu(item_contextMenu, viewport()->mapToGlobal(pos));
    }
}
 
void MarkerTreeWidget::showContextMenu(QTreeWidgetItem* item_contextMenu, const QPoint& globalPos) {
    QMenu menu;

    switch (item_contextMenu->type()) {
        case MARKER:
			menu.addAction(action_ChangeDescription);
            menu.addAction(action_CreateRigidBody);
            menu.addAction(action_AddToRigidBody);
			menu.addAction(action_ResetPoints);
			menu.addAction(action_DeletePoints);
			menu.addAction(action_ChangeDetectionMethod);
			menu.addAction(action_RefinePointsPolynomialFit);
			break;
 
        case RIGID_BODY:
            menu.addAction(action_ChangeDescription);
            break;
    }
 
    menu.exec(globalPos);
}

void MarkerTreeWidget::action_ChangeDescription_triggered(){
	QString description = item_contextMenu->text(1);
	bool ok;
	description = QInputDialog::getText(this, tr("Enter description"),
                                          tr("Description:"), QLineEdit::Normal,
                                          description, &ok);
	if (ok){
		item_contextMenu->setText(1,description);

		if(item_contextMenu->type() == MARKER){
			int idx = item_contextMenu->text(0).toInt()-1;
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->setDescription(description);
		}else if(item_contextMenu->type() == RIGID_BODY){
			QString text = item_contextMenu->text(0);
			text.remove(0,2);
			int idx = text.toInt()-1;
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setDescription(description);
		}
	}
}
void MarkerTreeWidget::action_DeletePoints_triggered(){
	QList<QTreeWidgetItem * > items = this->selectedItems();
	std::vector<int> pointsToDelete;
	if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete the selected Points?")) return;
	for (int i = 0; i < items.size(); i++){
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
	methodnames << "default Xray marker" << "Blobdetection" << "white marker"  << "corner detection" << "default + polyomial fitting" << "white marker + polynomial fitting" << "no detection";

	QString methodName = QInputDialog::getItem(this, tr("Choose method"),
		tr("Method:"), methodnames, 0, false, &ok);

	if (ok && !methodName.isEmpty())
	{
		int method = methodnames.indexOf(methodName);
		QList<QTreeWidgetItem * > items = this->selectedItems();
		for (int i = 0; i < items.size(); i++){
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

		int frame = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveFrame();
		int start = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame() - 1;
		int end = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame() - 1;

		int method = methodnames.indexOf(methodName);
		markerStatus max_marker_status;

		if (method == 0)
		{
			max_marker_status = SET;
		}
		else if(method == 1)
		{
			max_marker_status = TRACKED;
		}
		else
		{
			max_marker_status = MANUAL_REFINED;
		}

		QList<QTreeWidgetItem * > items = this->selectedItems();
		
		ProgressDialog::getInstance()->showProgressbar(start, frame, "Refining center");
		for (int i = start; i <= end; i++){
			ProgressDialog::getInstance()->setProgress(i);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveFrame(i);
			xma::State::getInstance()->changeActiveFrameTrial(i);

			for (int c = 0; c < Project::getInstance()->getCameras().size(); c++){
				for (int it = 0; it < items.size(); it++){
					if (items.at(it)->type() == MARKER)
					{
						if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i] > 0 &&
							Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i] <= max_marker_status){
							
							cv::Point2d pt(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getPoints2D()[c][i]);
							double size = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getSize();
							int marker_method = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getMethod();

							MarkerDetection::refinePointPolynomialFit(pt, size, (marker_method != 5 && marker_method != 2 ), c, xma::State::getInstance()->getActiveTrial());

							Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->setPoint(c, i, pt.x, pt.y, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(it)->text(0).toInt() - 1]->getStatus2D()[c][i]);
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

	/*

	if (ok && !methodName.isEmpty())
	{
		int method = methodnames.indexOf(methodName);
		QList<QTreeWidgetItem * > items = this->selectedItems();
		for (int i = 0; i < items.size(); i++){
			if (items.at(i)->type() == MARKER)
			{
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[items.at(i)->text(0).toInt() - 1]->setMethod(method);
			}
		}
	}*/
}

void MarkerTreeWidget::action_ResetPoints_triggered(){
	QList<QTreeWidgetItem * > items = this->selectedItems();
	if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset the data for the selected Points?")) return;
	for (int i = 0; i < items.size(); i++){
		if (items.at(i)->type() == MARKER)
		{
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[(items.at(i)->text(0).toInt() - 1)]->resetMultipleFrames(-1, 
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame() - 1 , Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame() - 1);
		}
	}
	MainWindow::getInstance()->redrawGL();
}


void MarkerTreeWidget::action_CreateRigidBody_triggered(){
	QList<QTreeWidgetItem * > items = this->selectedItems();
	bool NodeCreated = false;
	QTreeWidgetItem *qtreewidgetitem;

	for (int i = 0; i < items.size(); i++){
		if(!items.at(i)->parent() && items.at(i)->type() == MARKER){
			if(!NodeCreated){
				qtreewidgetitem = new QTreeWidgetItem(RIGID_BODY);
				qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDragEnabled);
				insertTopLevelItem(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size(), qtreewidgetitem);
				qtreewidgetitem->setText(0, "RB" + QString::number(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size() + 1));
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->addRigidBody();
				NodeCreated = true;
			}
			
			int ind = indexOfTopLevelItem(items.at(i));
			QTreeWidgetItem *qtreewidgetitem2 =  takeTopLevelItem(ind);
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size() - 1]->addPointIdx(qtreewidgetitem2->text(0).toInt() - 1);
			qtreewidgetitem->addChild(qtreewidgetitem2);
		}
	}
	PointsDockWidget::getInstance()->reloadListFromObject();
	PlotWindow::getInstance()->updateMarkers(true);
}

void MarkerTreeWidget::action_AddToRigidBody_triggered(){
	QString description = "RB";
	bool ok;
	description = QInputDialog::getText(this, tr("Enter ID"),
                                          tr("ID:"), QLineEdit::Normal,
                                          description, &ok);
	if (ok){
		QList<QTreeWidgetItem *>  itemRigidBody = findItems( description,Qt::MatchExactly,0);
		if(itemRigidBody.size()>0 && itemRigidBody.at(0)->type() == RIGID_BODY){	
			QString text = itemRigidBody.at(0)->text(0);
			text.remove(0,2);
			int idx = text.toInt()-1;
			QList<QTreeWidgetItem * > items = this->selectedItems();
			QTreeWidgetItem *qtreewidgetitem = itemRigidBody.at(0);

			for (int i = 0; i < items.size(); i++){
				if(!items.at(i)->parent() && items.at(i)->type() == MARKER){
					int ind = indexOfTopLevelItem(items.at(i));
					QTreeWidgetItem *qtreewidgetitem2 =  takeTopLevelItem(ind);
					qtreewidgetitem->addChild(qtreewidgetitem2);
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->addPointIdx(items.at(i)->text(0).toInt() - 1);
				}else if(items.at(i)->parent() && items.at(i)->type() == MARKER){
					//remove from previous RB
					QString text_parent = items.at(i)->parent()->text(0);
					text_parent.remove(0,2);
					int idx_parent = text_parent.toInt()-1;
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx_parent]->removePointIdx(items.at(i)->text(0).toInt() - 1);

					//Set To new Parent
					int ind = items.at(i)->parent()->indexOfChild(items.at(i));
					QTreeWidgetItem *qtreewidgetitem2 =  items.at(i)->parent()->takeChild(ind);
					qtreewidgetitem->addChild(qtreewidgetitem2);
					
					//Add to RB
					Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->addPointIdx(items.at(i)->text(0).toInt() - 1);
				}
			}
		}
	}
}



void MarkerTreeWidget::dropEvent ( QDropEvent * event ){
	
	QTreeWidgetItem * dropped = itemAt( event->pos() );
	QList <QTreeWidgetItem *> dragged = selectedItems();

	for(int i = 0 ; i < dragged.size(); i ++){
		int item_idx = dragged[i]->text(0).toInt() - 1;

		if(!dropped){
			if(dragged[i]->parent()){
				//fprintf(stderr, "\Item %s dragged from %s to Noparent",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str());

				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0,2);
				int parent_from_idx = text_parent_from.toInt()-1;
				
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);
			}else{
				//fprintf(stderr, "\Item %s dragged from Noparent to Noparent",dragged[i]->text(1).toStdString().c_str());
			}
		}
		else if(dropped->parent()){
			if(dragged[i]->parent()){
				
				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0,2);
				int parent_from_idx = text_parent_from.toInt()-1;
				
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);

				QString text_parent_to = dropped->parent()->text(0);
				text_parent_to.remove(0,2);
				int parent_to_idx = text_parent_to.toInt()-1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);

			}else{
				//fprintf(stderr, "\Item %s dragged from Noparent to %s",dragged[i]->text(1).toStdString().c_str(), dropped->parent()->text(1).toStdString().c_str());
				QString text_parent_to = dropped->parent()->text(0);
				text_parent_to.remove(0,2);
				int parent_to_idx = text_parent_to.toInt()-1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
		}else if(dropped->type() == RIGID_BODY){
			if(dragged[i]->parent()){
				//fprintf(stderr, "\Item %s dragged from %s to %s",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str(),dropped->text(1).toStdString().c_str());
			
				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0,2);
				int parent_from_idx = text_parent_from.toInt()-1;

				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);

				QString text_parent_to = dropped->text(0);
				text_parent_to.remove(0,2);
				int parent_to_idx = text_parent_to.toInt()-1;
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);

			}else{
				//fprintf(stderr, "\Item %s dragged from Noparent to %s",dragged[i]->text(1).toStdString().c_str(),dropped->text(1).toStdString().c_str());
				
				QString text_parent_to = dropped->text(0);
				text_parent_to.remove(0,2);
				int parent_to_idx = text_parent_to.toInt()-1;
				
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_to_idx]->addPointIdx(item_idx);
			}
		}else{
			if(dragged[i]->parent()){
				//fprintf(stderr, "\Item %s dragged from %s to Noparent",dragged[i]->text(1).toStdString().c_str(),dragged[i]->parent()->text(1).toStdString().c_str());

				QString text_parent_from = dragged[i]->parent()->text(0);
				text_parent_from.remove(0,2);
				int parent_from_idx = text_parent_from.toInt()-1;
				
				Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[parent_from_idx]->removePointIdx(item_idx);

			}else{
				//fprintf(stderr, "\Item %s dragged from Noparent to Noparent",dragged[i]->text(1).toStdString().c_str());
			}
		}
	}
    QTreeWidget::dropEvent(event);
}

void MarkerTreeWidget::onItemCollapsed ( QTreeWidgetItem * item ){
	QString text = item->text(0);
	text.remove(0,2);
	int idx = text.toInt()-1;
	Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setExpanded(false);
}

void MarkerTreeWidget::onItemExpanded ( QTreeWidgetItem * item ){
	QString text = item->text(0);
	text.remove(0,2);
	int idx = text.toInt()-1;	
	Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->setExpanded(true);
}