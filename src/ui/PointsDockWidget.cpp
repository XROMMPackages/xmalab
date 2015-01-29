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

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"

#include <QMouseEvent>
#include <QInputDialog>

using namespace xma;

PointsDockWidget* PointsDockWidget::instance = NULL;

PointsDockWidget::PointsDockWidget(QWidget *parent) :
											QDockWidget(parent),
											dock(new Ui::PointsDockWidget){

	dock->setupUi(this);

	qApp->installEventFilter( this );

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
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

		if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() >=0 &&
			!selectPoint(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() + 1)){
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(-1);
		}
		if (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveRBIdx() >= 0 &&
			!selectBody(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getActiveRBIdx() + 1)){
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(-1);
		}
	}

	WizardDockWidget::getInstance()->updateDialog();
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

bool PointsDockWidget::selectBody(int idx){
	QList<QTreeWidgetItem *>  items = dock->treeWidgetPoints->findItems("RB" + QString::number(idx), Qt::MatchExactly | Qt::MatchRecursive, 0);
	dock->treeWidgetPoints->clearSelection();
	if (items.size()>0){
		if (dock->treeWidgetPoints->currentItem() == items.at(0)) on_treeWidgetPoints_currentItemChanged(items.at(0), items.at(0));
		dock->treeWidgetPoints->setCurrentItem(items.at(0));
		return true;
	}
	else{
		return false;
	}
}

void PointsDockWidget::on_treeWidgetPoints_currentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous){
	if(current && current->type() == MARKER){
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(current->text(0).toInt() - 1);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(-1);
	}
	else if (current && current->type() == RIGID_BODY)
	{
		QString text = current->text(0);
		text.remove(0, 2);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveRBIdx(text.toInt() - 1);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(-1);
	}

	WizardDockWidget::getInstance()->updateDialog();
	MainWindow::getInstance()->redrawGL();
}

void PointsDockWidget::on_pushButtonSetNumberMarkers_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Number of Markers to"),
		tr(""), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(), 0, 1000, 1, &ok);
	if (ok){
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() < idx)
		{
			while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() != idx){
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();
			}
		}
		else
		{
			if(ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to delete Points " + QString::number(idx + 1) + " to " + QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()) + ". Are you sure you want to proceed?")){	
				while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() != idx){
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
	if (ok){
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() < idx)
		{
			while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() != idx){
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addRigidBody();
			}
		}
		else
		{
			if (ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to delete Rigid Bodies " + QString::number(idx + 1) + " to " + QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size()) + ". Are you sure you want to proceed?")){
				while (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() != idx){
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->removeRigidBody(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() - 1);
				}
			}
		}
	}
	reloadListFromObject();
}

void PointsDockWidget::on_pushButtonImportExport_clicked()
{
	ImportExportPointsDialog * diag = new ImportExportPointsDialog(this);
	diag->exec();
	delete diag;
	reloadListFromObject();
}

void PointsDockWidget::activeTrialChanged(int activeCamera)
{
	reloadListFromObject();
}
