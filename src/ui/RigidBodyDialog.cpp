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
///\file RigidBodyDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/RigidBodyDialog.h"
#include "ui_RigidBodyDialog.h"
#include "ui/State.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"

#include "core/Settings.h"
#include "core/RigidBody.h"
#include "core/Marker.h"
#include "core/Trial.h"

#include <QLabel>
#include <QComboBox>
#include <QColorDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <core/HelperFunctions.h>

using namespace xma;

RigidBodyDialog::RigidBodyDialog(RigidBody* body, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::RigidBodyDialog), m_body(body)
{
	diag->setupUi(this);

	updateIcon();

	this->setWindowTitle("RB : " + m_body->getDescription());

	updateColorButton();
	diag->checkBox_Draw->setChecked(m_body->getVisible());

	QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);

	QSizePolicy sizePolicy2(QSizePolicy::Maximum, QSizePolicy::Fixed);
	sizePolicy2.setHorizontalStretch(0);
	sizePolicy2.setVerticalStretch(0);

	for (unsigned int i = 0; i < m_body->getPointsIdx().size(); i++)
	{
		label_RefPoint.push_back(new QLabel(diag->groupBox));
		sizePolicy1.setHeightForWidth(label_RefPoint[i]->sizePolicy().hasHeightForWidth());
		label_RefPoint[i]->setSizePolicy(sizePolicy1);

		label_DescPoint.push_back(new QLabel(diag->groupBox));
		sizePolicy1.setHeightForWidth(label_DescPoint[i]->sizePolicy().hasHeightForWidth());
		label_DescPoint[i]->setSizePolicy(sizePolicy1);

		comboBox.push_back(new QComboBox(diag->groupBox));
		sizePolicy2.setHeightForWidth(comboBox[i]->sizePolicy().hasHeightForWidth());
		comboBox[i]->setSizePolicy(sizePolicy2);

		label_RefPoint[i]->setText(m_body->getReferenceNames()[i]);
		for (unsigned int j = 0; j < m_body->getPointsIdx().size(); j++)
		{
			comboBox[i]->addItem(QString::number(m_body->getPointsIdx()[j] + 1));
		}
		comboBox[i]->setCurrentIndex(i);

		connect(comboBox[i], SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));

		label_DescPoint[i]->setText(m_body->getMarker(comboBox[i]->currentText().toInt() - 1)->getDescription());

		diag->gridLayout->addWidget(label_RefPoint[i], i, 0, 1, 1);
		diag->gridLayout->addWidget(comboBox[i], i, 1, 1, 1);
		diag->gridLayout->addWidget(label_DescPoint[i], i, 2, 1, 1);
	}

	if (m_body->getOverrideCutoffFrequency())
	{
		diag->checkBoxCutoffOverride->setChecked(true);
		diag->doubleSpinBoxCutoff->setEnabled(true);
		diag->doubleSpinBoxCutoff->setValue(m_body->getCutoffFrequency());
	}
	else
	{
		diag->checkBoxCutoffOverride->setChecked(false);
		diag->doubleSpinBoxCutoff->setEnabled(false);
		diag->doubleSpinBoxCutoff->setValue(m_body->getTrial()->getCutoffFrequency());
	}

	if (!m_body->allReferenceMarkerReferencesSet())diag->pushButton_Reset->show();

	if (m_body->hasMeshModel())
	{
		QFileInfo info(m_body->getMeshModelname());
		diag->label_Mesh->setText(info.fileName());
		diag->checkbox_DrawMesh->setChecked(m_body->getDrawMeshModel());
	}
	else
	{
		diag->checkbox_DrawMesh->setEnabled(false);
		diag->doubleSpinBoxMeshScale->setEnabled(false);
	}
	diag->doubleSpinBoxMeshScale->setValue(m_body->getMeshScale());

	reloadDummyPoints();

	if (m_body->getTrial()->getIsDefault())
	{
		diag->pushButton_setFromFrame->setEnabled(false);
		diag->checkBox_Optimized->setEnabled(false);
		diag->pushButtonUpdate->setEnabled(false);
	}
	else if (m_body->getTrial()->getIsCopyFromDefault())
	{	
		diag->groupBox->setEnabled(false);
		diag->groupBox_2->setEnabled(false);
		diag->groupBox_3->setEnabled(false);
		diag->groupBox_4->setEnabled(false);
		diag->pushButton_setFromFile->setEnabled(false);
		diag->pushButton_setFromFrame->setEnabled(false);
		diag->pushButton_Reset->setEnabled(false);
		diag->checkBox_Optimized->setEnabled(false);
		diag->pushButtonUpdate->setEnabled(false);
	}
}


void RigidBodyDialog::updateIcon()
{
	QIcon icon;
	if (m_body->isReferencesSet() == 2)
	{
		if (m_body->getHasOptimizedCoordinates())
		{
			icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_optimized.png"), QSize(), QIcon::Normal, QIcon::Off);
		}
		else{
			icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d.png"), QSize(), QIcon::Normal, QIcon::Off);
		}
	}
	else if (m_body->isReferencesSet() == 1)
	{
		if (m_body->getHasOptimizedCoordinates())
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
	this->setWindowIcon(icon);

	diag->checkBox_Optimized->setChecked(m_body->getHasOptimizedCoordinates());

	diag->pushButtonUpdate->setEnabled(diag->checkBox_Optimized->isChecked());
}

void RigidBodyDialog::updateLabels()
{
	for (unsigned int i = 0; i < comboBox.size(); i++)
	{
		label_RefPoint[i]->setText(m_body->getReferenceNames()[i]);
		label_DescPoint[i]->setText(m_body->getMarker(comboBox[i]->currentText().toInt() - 1)->getDescription());
	}
}

RigidBodyDialog::~RigidBodyDialog()
{
	delete diag;

	for (unsigned int i = 0; i < comboBox.size(); i++)
	{
		delete label_RefPoint[i];
		delete comboBox[i];
		delete label_DescPoint[i];
	}
	label_RefPoint.clear();
	comboBox.clear();
	label_DescPoint.clear();
}

bool RigidBodyDialog::isComplete()
{
	bool valid_Selection = true;
	for (unsigned int i = 0; i < comboBox.size(); i++)
	{
		for (unsigned int j = 0; j < comboBox.size(); j++)
		{
			if (i != j && comboBox[i]->currentText() == comboBox[j]->currentText()) valid_Selection = false;
		}
	}

	if (!valid_Selection)
	{
		ErrorDialog::getInstance()->showErrorDialog("Each Marker can only be used once. Please correct your selection.");
		return false;
	}

	setRigidBodyIdxByDialog();

	return true;
}


bool RigidBodyDialog::setRigidBodyIdxByDialog()
{
	bool valid_Selection = true;
	for (unsigned int i = 0; i < comboBox.size(); i++)
	{
		for (unsigned int j = 0; j < comboBox.size(); j++)
		{
			if (i != j && comboBox[i]->currentText() == comboBox[j]->currentText()) valid_Selection = false;
		}
	}

	if (!valid_Selection)
	{
		ErrorDialog::getInstance()->showErrorDialog("Each Marker can only be used once. Please correct your selection.");
		return false;
	}

	for (unsigned int j = 0; j < m_body->getPointsIdx().size(); j++)
	{
		m_body->setPointIdx(j, comboBox[j]->currentText().toInt() - 1);
	}

	return true;
}

void RigidBodyDialog::updateColorButton()
{
	QPixmap pix(16, 16);
	pix.fill(m_body->getColor());
	diag->toolButton_Color->setIcon(pix);
}

void RigidBodyDialog::reloadDummyPoints()
{
	for (unsigned int i = 0; i < label_Dummy.size(); i++)
	{
		diag->gridLayout_5->removeWidget(label_Dummy[i]);
		delete label_Dummy[i];
	}
	label_Dummy.clear();

	for (unsigned int i = 0; i < m_body->getDummyNames().size(); i++)
	{
		QLabel* label = new QLabel(m_body->getDummyNames()[i]);
		diag->gridLayout_5->addWidget(label, i + 1, 0, 1, 2);
		label_Dummy.push_back(label);
	}
}

void RigidBodyDialog::on_pushButton_setFromFile_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open rigid body references"), Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));

	if (fileName.isNull() == false)
	{
		if (m_body->isReferencesSet())
		{
			if (!ConfirmationDialog::getInstance()->showConfirmationDialog("References are already set by using a file. Are you sure you want to reset the references?"))
			{
				return;
			}
			m_body->setReferencesSet(0);
		}

		setRigidBodyIdxByDialog();
		updateLabels();

		m_body->computeCoordinateSystemAverage();

		int result = m_body->setReferenceFromFile(fileName);

		if (result == -1)
		{
			ErrorDialog::getInstance()->showErrorDialog("Marker of the rigid body and the references supplied in the file are different or there has been a problem loading the file.");
			return;
		}
		else if (result == -2)
		{
			ErrorDialog::getInstance()->showErrorDialog("The data in the marker could not be fitted. Please check the used correspondances.");
			return;
		}

		updateIcon();
		updateLabels();
	}
}

void RigidBodyDialog::on_pushButton_setFromFrame_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Frame Number"),
		tr(""), m_body->getTrial()->getActiveFrame() + 1, m_body->getTrial()->getStartFrame(), m_body->getTrial()->getEndFrame(), 1, &ok) - 1;
	if (ok)
	{
		bool could_set = m_body->setReferenceFromFrame(idx);

		if (!could_set)
		{
			ErrorDialog::getInstance()->showErrorDialog("Cannot set references. All markers of the rigid body have to be detected in the selected frame!");
			return;
		}
		updateIcon();
		updateLabels();
	}
}

void RigidBodyDialog::on_pushButtonExport_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this,tr("Rigid Body references as"), 
		Settings::getInstance()->getLastUsedDirectory() + OS_SEP + m_body->getDescription(), tr("CSV (*.csv)"));

	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		m_body->saveOptimized(fileName, true);

	}
}

void RigidBodyDialog::on_pushButton_OK_clicked()
{
	if (isComplete()) this->accept();
}

void RigidBodyDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

void RigidBodyDialog::currentIndexChanged(int idx)
{
	updateLabels();
}

void RigidBodyDialog::on_checkBox_Draw_clicked()
{
	m_body->setVisible(diag->checkBox_Draw->isChecked());
}

void RigidBodyDialog::on_toolButton_Color_clicked()
{
	QColorDialog* cdiag = new QColorDialog();
	cdiag->setCurrentColor(m_body->getColor());
	cdiag->exec();

	m_body->setColor(cdiag->currentColor());

	updateColorButton();

	delete cdiag;
}

void RigidBodyDialog::on_checkBoxCutoffOverride_clicked()
{
	if (diag->checkBoxCutoffOverride->isChecked())
	{
		m_body->setOverrideCutoffFrequency(true);
		diag->doubleSpinBoxCutoff->setEnabled(true);
		diag->doubleSpinBoxCutoff->setValue(m_body->getCutoffFrequency());
	}
	else
	{
		m_body->setOverrideCutoffFrequency(false);
		diag->doubleSpinBoxCutoff->setEnabled(false);
		diag->doubleSpinBoxCutoff->setValue(m_body->getTrial()->getCutoffFrequency());
	}
}

void RigidBodyDialog::on_doubleSpinBoxCutoff_valueChanged(double value)
{
	if (diag->checkBoxCutoffOverride->isChecked())
	{
		m_body->setCutoffFrequency(value);
	}
}

void RigidBodyDialog::on_pushButton_AddDummy_clicked()
{
	bool ok;
	QString name = QInputDialog::getText(this, tr("QInputDialog::getText()"),
	                                     tr("Enter a name for the virtual point:"), QLineEdit::Normal, "Virtual " + QString::number(m_body->getDummyNames().size() + 1), &ok);
	QString filenameCoords;
	QString filenameRef;
	QString filenameRef2;
	int markerID = -1;

	if (ok && !name.isEmpty())
	{
		int id = -1;
		for (unsigned int i = 0; i < m_body->getTrial()->getRigidBodies().size(); i++)
		{
			if (m_body == m_body->getTrial()->getRigidBodies()[i]) id = i + 1;
		}

		filenameRef = QFileDialog::getOpenFileName(this, "Open CT coordinate file. (Point in the reference of RB" + QString::number(id) + ")" , Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));
		if (!filenameRef.isEmpty())
		{
			Settings::getInstance()->setLastUsedDirectory(filenameRef);
		}
		else
		{
			return;
		}

		if (ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to animate the virtual point by using a Rigid Body? Click No if you want to import a csv of tracked data instead.", true))
		{
			QStringList trialnames;
			for (unsigned int i = 0; i < m_body->getTrial()->getRigidBodies().size(); i++)
			{
				trialnames << QString::number(i + 1);
			}

			QString item = QInputDialog::getItem(this, tr("Choose Rigid Body"),
			                                     tr("RB:"), trialnames, 0, false, &ok);

			if (ok && !item.isEmpty())
			{
				markerID = item.toInt() - 1;
				name = name + " RB" + QString::number(markerID + 1);
			}

			if (markerID != -1)
			{
				if (ConfirmationDialog::getInstance()->showConfirmationDialog("Do need to use different CT coordinates for the animated point (Point in the reference of RB" + QString::number(markerID + 1) + ") ?"" Click No if both rigid bodies use the same point.", true))
				{
					filenameRef2 = QFileDialog::getOpenFileName(this, "Open CT coordinate file. (Point in the reference of RB" + QString::number(markerID + 1) + ")", Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));
					if (!filenameRef2.isEmpty())
					{
						Settings::getInstance()->setLastUsedDirectory(filenameRef2);
					}
				} 
				else
				{
					filenameRef2 = filenameRef;
				}
			}
		}

		if (markerID == -1)
		{
			filenameCoords = QFileDialog::getOpenFileName(this, tr("Open tracked virtual data file"), Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));
			if (!filenameCoords.isEmpty())
			{
				Settings::getInstance()->setLastUsedDirectory(filenameCoords);
			}
			else
			{
				return;
			}
		} 
	}
	else
	{
		return;
	}

	m_body->addDummyPoint(name, filenameRef, filenameRef2, markerID, filenameCoords);

	reloadDummyPoints();

	m_body->recomputeTransformations();
}

void RigidBodyDialog::on_pushButton_DeleteDummy_clicked()
{
	m_body->clearAllDummyPoints();

	reloadDummyPoints();

	m_body->recomputeTransformations();
}

void RigidBodyDialog::on_pushButton_Reset_clicked()
{
	m_body->resetReferences();
	updateIcon();
	updateLabels();
}

void RigidBodyDialog::on_checkBox_Optimized_clicked()
{
	m_body->setOptimized(diag->checkBox_Optimized->isChecked());
	updateIcon();
	m_body->recomputeTransformations();
}

void RigidBodyDialog::on_pushButtonUpdate_clicked()
{
	m_body->recomputeTransformations();
}

void RigidBodyDialog::on_checkbox_DrawMesh_clicked()
{
	m_body->setDrawMeshModel(diag->checkbox_DrawMesh->isChecked());
}

void RigidBodyDialog::on_toolButton_Mesh_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open meshfile file"), Settings::getInstance()->getLastUsedDirectory(), ("OBJ Files (*.obj)"));
	if (!filename.isEmpty())
	{
		if (!m_body->addMeshModel(filename)){
			ErrorDialog::getInstance()->showErrorDialog("Could not load OBJ-file");
			diag->checkbox_DrawMesh->setEnabled(false);
			diag->doubleSpinBoxMeshScale->setEnabled(false);
		}else{
			QFileInfo info(filename);
			diag->label_Mesh->setText(info.fileName());
			diag->checkbox_DrawMesh->setEnabled(true);
			diag->doubleSpinBoxMeshScale->setEnabled(true);
		}
	}
}

void RigidBodyDialog::on_doubleSpinBoxMeshScale_valueChanged(double value)
{
	m_body->setMeshScale(value);
}
