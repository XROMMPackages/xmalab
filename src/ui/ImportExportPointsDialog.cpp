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
///\file ImportExportPointsDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "ui/ImportExportPointsDialog.h"
#include "ui_ImportExportPointsDialog.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"
#include "ui/State.h"
#include "ui/PlotWindow.h"
#include "ui/ProjectFileIO.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"

#include <QFileDialog>

#include <fstream>
#include <QtGui/QInputDialog>

using namespace xma;

ImportExportPointsDialog::ImportExportPointsDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::ImportExportPointsDialog)
{
	diag->setupUi(this);

	switchGroups();

	if (Project::getInstance()->getTrials().size() > 1)
	{
		diag->radioButtonTrial->show();
		for (unsigned int i = 0; i < Project::getInstance()->getTrials().size(); i++)
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

ImportExportPointsDialog::~ImportExportPointsDialog()
{
	delete diag;
}

void ImportExportPointsDialog::switchGroups()
{
	if (diag->radioButtonImportXMA->isChecked())
	{
		diag->groupBoxImportXMA->show();
	}
	else
	{
		diag->groupBoxImportXMA->hide();
	}
	if (diag->radioButtonImportCSV->isChecked())
	{
		diag->groupBoxImportCSV->show();
	}
	else
	{
		diag->groupBoxImportCSV->hide();
	}

	if (diag->radioButtonTrial->isChecked())
	{
		diag->groupBoxTrial->show();
	}
	else
	{
		diag->groupBoxTrial->hide();
	}
}

void ImportExportPointsDialog::on_radioButtonImportCSV_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_radioButtonImportXMA_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_radioButtonTrial_clicked(bool checked)
{
	switchGroups();
}

void ImportExportPointsDialog::on_toolButtonMarkersCSV_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Open marker csv file"), Settings::getInstance()->getLastUsedDirectory(), ("CSV files (*.csv)"));
	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		diag->lineEditMarkersCSV->setText(fileName);
	}
}

void ImportExportPointsDialog::on_toolButtonMarkersXMA_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Open marker csv file"), Settings::getInstance()->getLastUsedDirectory(), ("Dataset (*.xma  *.zip)"));
	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		diag->lineEditMarkersXMA->setText(fileName);
	}
}


void ImportExportPointsDialog::on_pushButtonCancel_clicked()
{
	this->close();
}

void ImportExportPointsDialog::on_pushButtonOK_clicked()
{
	if (diag->radioButtonImportXMA->isChecked())
	{
		importXMA();
	}
	if (diag->radioButtonImportCSV->isChecked())
	{
		importCSV();
	}
	if (diag->radioButtonTrial->isChecked())
	{
		copyFromTrial();
	}
	this->close();
}

bool ImportExportPointsDialog::importCSV()
{
	if (!diag->lineEditMarkersCSV->text().isEmpty())
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->loadMarkersFromCSV(diag->lineEditMarkersCSV->text());

		PlotWindow::getInstance()->updateMarkers(false);
		return true;
	}
	return false;
}

bool ImportExportPointsDialog::importXMA()
{
	bool ok = true;
	if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() > 0 || Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() > 0)
	{
		ok = ConfirmationDialog::getInstance()->showConfirmationDialog("All markers and Rigid bodies will be deleted before import. Are you sure you want to proceed?");
	}
	if (ok)
	{
		if (!diag->lineEditMarkersXMA->text().isEmpty())
		{
			QStringList trialnames = ProjectFileIO::getInstance()->readTrials(diag->lineEditMarkersXMA->text());

			bool ok;
			QString item = QInputDialog::getItem(this, tr("Choose trial to import"),
			                                     tr("Trial:"), trialnames, 0, false, &ok);

			if (ok && !item.isEmpty())
			{
				ProjectFileIO::getInstance()->loadMarker(diag->lineEditMarkersXMA->text(), item, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]);

				PlotWindow::getInstance()->updateMarkers(false);
			}
			return true;
		}
	}
	return false;
}


bool ImportExportPointsDialog::copyFromTrial()
{
	bool ok = true;
	if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() > 0 || Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() > 0)
	{
		ok = ConfirmationDialog::getInstance()->showConfirmationDialog("All markers and Rigid bodies will be deleted before import. Are you sure you want to proceed?");
	}
	if (ok)
	{
		for (unsigned int idx = 0; idx < Project::getInstance()->getTrials().size(); idx++)
		{
			if (diag->comboBoxTrial->currentText() == Project::getInstance()->getTrials()[idx]->getName())
			{
				for (unsigned int i = 0; i < Project::getInstance()->getTrials()[idx]->getMarkers().size(); i++)
				{
					if (i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setDescription(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getDescription());

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setMaxPenalty(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getMaxPenalty());

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setMethod(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getMethod());

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setInterpolation(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getInterpolation());

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setSizeOverride(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getSizeOverride());

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->setThresholdOffset(
						Project::getInstance()->getTrials()[idx]->getMarkers()[i]->getThresholdOffset());
				}

				for (unsigned int i = 0; i < Project::getInstance()->getTrials()[idx]->getRigidBodies().size(); i++)
				{
					if (i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size())
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addRigidBody();

					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->copyData(
						Project::getInstance()->getTrials()[idx]->getRigidBodies()[i]);
				}
				PlotWindow::getInstance()->updateMarkers(false);
				return true;
			}
		}
	}
	return false;
}

