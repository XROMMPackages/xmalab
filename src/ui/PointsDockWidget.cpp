#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/PointsDockWidget.h"
#include "ui_PointsDockWidget.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"

#include <QMouseEvent>

using namespace xma;

PointsDockWidget* PointsDockWidget::instance = NULL;

PointsDockWidget::PointsDockWidget(QWidget *parent) :
											QDockWidget(parent),
											dock(new Ui::PointsDockWidget){

	dock->setupUi(this);

	qApp->installEventFilter( this );			
}
	
PointsDockWidget::~PointsDockWidget(){
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

void PointsDockWidget::addPointToList(int idx){
	QTreeWidgetItem *qtreewidgetitem = new QTreeWidgetItem(MARKER);
	qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDropEnabled);
	qtreewidgetitem->setText(0, QString::number(idx));
	dock->treeWidgetPoints->addTopLevelItem(qtreewidgetitem);
}

void PointsDockWidget::reloadListFromObject(){
	if (Project::getInstance()->getTrials().size() > 0){
		dock->treeWidgetPoints->clear();
		QTreeWidgetItem *qtreewidgetitem;

		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers().size(); i++){
			qtreewidgetitem = new QTreeWidgetItem(MARKER);
			qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDropEnabled);
			qtreewidgetitem->setText(0, QString::number(i + 1));

			qtreewidgetitem->setText(1, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[i]->getDescription());

			dock->treeWidgetPoints->addTopLevelItem(qtreewidgetitem);
		}

		//Setup Rigid Body
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies().size(); i++){
			qtreewidgetitem = new QTreeWidgetItem(RIGID_BODY);
			qtreewidgetitem->setFlags(qtreewidgetitem->flags() & ~Qt::ItemIsDragEnabled);
			dock->treeWidgetPoints->insertTopLevelItem(i, qtreewidgetitem);
			qtreewidgetitem->setText(0, "RB" + QString::number(i + 1));
			qtreewidgetitem->setText(1, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getDescription());
			for (unsigned int k = 0; k < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getPointsIdx().size(); k++){
				QList<QTreeWidgetItem *>  items = dock->treeWidgetPoints->findItems(QString::number(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getPointsIdx()[k] + 1), Qt::MatchExactly | Qt::MatchRecursive, 0);
				if (items.size()>0){
					int ind = dock->treeWidgetPoints->indexOfTopLevelItem(items.at(0));
					QTreeWidgetItem *qtreewidgetitem2 = dock->treeWidgetPoints->takeTopLevelItem(ind);
					qtreewidgetitem->addChild(qtreewidgetitem2);
				}
			}
			if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->isExpanded())dock->treeWidgetPoints->expandItem(qtreewidgetitem);
		}

		if (!selectPoint(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() + 1)){
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(-1);
		}
	}
}

bool PointsDockWidget::selectPoint(int idx){
	QList<QTreeWidgetItem *>  items = dock->treeWidgetPoints->findItems(QString::number(idx), Qt::MatchExactly | Qt::MatchRecursive, 0);
	dock->treeWidgetPoints->clearSelection();
	if(items.size()>0){
		if (dock->treeWidgetPoints->currentItem() == items.at(0)) on_treeWidgetPoints_currentItemChanged(items.at(0), items.at(0));
		dock->treeWidgetPoints->setCurrentItem(items.at(0));
			return true;
	}else{
		return false;
	}
}

void PointsDockWidget::on_treeWidgetPoints_currentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous){
	if(current && current->type() == MARKER){
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(current->text(0).toInt() - 1);
	}
	else if (current && current->type() == RIGID_BODY)
	{

	}
	MainWindow::getInstance()->redrawGL();
}
