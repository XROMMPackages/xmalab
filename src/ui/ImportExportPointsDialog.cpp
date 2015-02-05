#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include "ui/ImportExportPointsDialog.h"
#include "ui_ImportExportPointsDialog.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"
#include "ui/State.h"
#include "ui/PlotWindow.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"

#include <QFileDialog>

#include <fstream>

using namespace xma;

ImportExportPointsDialog::ImportExportPointsDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::ImportExportPointsDialog){



	diag->setupUi(this);

	switchGroups();

	if (Project::getInstance()->getTrials().size() > 1)
	{
		diag->radioButtonTrial->show();
		for (int i = 0; i < Project::getInstance()->getTrials().size(); i++)
		{
			if (i != State::getInstance()->getActiveTrial())
			{
				diag->comboBoxTrial->addItem(Project::getInstance()->getTrials()[i]->getName());
			}
		}
	}
	else
	{
		diag->radioButtonTrial->hide();
	}
	diag->gridLayout_4->setSizeConstraint(QLayout::SetFixedSize);
}

ImportExportPointsDialog::~ImportExportPointsDialog(){
	delete diag;
}

void ImportExportPointsDialog::switchGroups()
{
	if (diag->radioButtonImport->isChecked()) {
		diag->groupBoxImport->show();
	}
	else
	{
		diag->groupBoxImport->hide();
	}
	if (diag->radioButtonExport->isChecked()) {
		diag->groupBoxExport->show();
	}
	else
	{
		diag->groupBoxExport->hide();
	}
	if (diag->radioButtonTrial->isChecked()) {
		diag->groupBoxTrial->show();
	}
	else
	{
		diag->groupBoxTrial->hide();
	}
}

void ImportExportPointsDialog::on_radioButtonExport_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_radioButtonImport_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_radioButtonTrial_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_toolButtonMarkers_clicked(){
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open marker description"), Settings::getLastUsedDirectory(), ("Txt files (*.txt)"));
	if (fileName.isNull() == false)
	{
		Settings::setLastUsedDirectory(fileName);
		diag->lineEditMarkers->setText(fileName);
	}
}

void ImportExportPointsDialog::on_toolButtonRigidBodies_clicked(){
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open rigid body description"), Settings::getLastUsedDirectory(), ("Txt files (*.txt)"));
	if (fileName.isNull() == false)
	{
		Settings::setLastUsedDirectory(fileName);
		diag->lineEditRigidBodies->setText(fileName);
	}
}

void ImportExportPointsDialog::on_pushButtonCancel_clicked()
{
	this->close();
}

void ImportExportPointsDialog::on_pushButtonOK_clicked()
{
	if (diag->radioButtonImport->isChecked()) {
		importData();
	}
	if (diag->radioButtonExport->isChecked()) {
		exportData();
	}
	if (diag->radioButtonTrial->isChecked()) {
		copyFromTrial();
	}
	this->close();
}

bool ImportExportPointsDialog::importData()
{
	if (!diag->lineEditMarkers->text().isEmpty() || !diag->lineEditRigidBodies->text().isEmpty())
	{
		if (!diag->lineEditMarkers->text().isEmpty())
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->loadMarkers(diag->lineEditMarkers->text());
		}
		if (!diag->lineEditRigidBodies->text().isEmpty())
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->loadRigidBodies(diag->lineEditRigidBodies->text());
		}
		PlotWindow::getInstance()->updateMarkers(false);
		return true;
	}
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("You need to select a file for markers and/or rigid bodies.");
		return false;
	}
}

bool ImportExportPointsDialog::exportData()
{
	if (diag->checkBoxMarkers->isChecked() || diag->checkBoxRigidBodies->isChecked()){
		if (diag->checkBoxMarkers->isChecked())
		{
			QString fileNameMarkers = QFileDialog::getSaveFileName(this,
				tr("Save marker description as"), Settings::getLastUsedDirectory() , tr("Txt files (*.txt)"));
			if (fileNameMarkers.isNull() == false)
			{
				Settings::setLastUsedDirectory(fileNameMarkers);
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveMarkers(fileNameMarkers);
			}
		}
		if (diag->checkBoxRigidBodies->isChecked())
		{
			QString fileNameRigidBodies = QFileDialog::getSaveFileName(this,
				tr("Save rigid body description as"), Settings::getLastUsedDirectory(), tr("Txt files (*.txt)"));
			if (fileNameRigidBodies.isNull() == false)
			{
				Settings::setLastUsedDirectory(fileNameRigidBodies);
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveRigidBodies(fileNameRigidBodies);
			}
		}
		
		return true;
	}
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("You need to at least check one checkbox to export the data.");
		return false;
	}
}

bool ImportExportPointsDialog::copyFromTrial()
{
	for (int idx = 0; idx < Project::getInstance()->getTrials().size(); idx++)
	{
		if (diag->comboBoxTrial->currentText() == Project::getInstance()->getTrials()[idx]->getName())
		{
			for (int i = 0; i < Project::getInstance()->getTrials()[idx]->getMarkers().size(); i++)
			{
				if (i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();

				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setDescription(
					Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getDescription());
			}

			for (int i = 0; i < Project::getInstance()->getTrials()[idx]->getRigidBodies().size(); i++)
			{
				if (i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size())
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addRigidBody();

				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->setDescription(
					Project::getInstance()->getTrials()[idx]->getRigidBodies()[i]->getDescription());

				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->clearPointIdx();

				for (int j = 0; j < Project::getInstance()->getTrials()[idx]->getRigidBodies().size(); j++){
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->addPointIdx(
						Project::getInstance()->getTrials()[idx]->getRigidBodies()[i]->getPointsIdx()[j]);
				}
			}
			PlotWindow::getInstance()->updateMarkers(false);
			return true;
		}
	}
	return false;
}





