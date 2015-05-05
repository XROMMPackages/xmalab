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

using namespace xma;

RigidBodyDialog::RigidBodyDialog(RigidBody * body, QWidget *parent) :
												QDialog(parent),
												diag(new Ui::RigidBodyDialog), m_body(body){

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

	for (int i = 0; i < m_body->getPointsIdx().size(); i++)
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
		for (int j = 0; j < m_body->getPointsIdx().size(); j++)
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

	if (m_body->getOverrideCutoffFrequency()){
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

	reloadDummyPoints();
}


void  RigidBodyDialog::updateIcon()
{
	QIcon icon;
	if (m_body->isReferencesSet()){
		icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d.png"), QSize(), QIcon::Normal, QIcon::Off);
	}
	else
	{
		icon.addFile(QString::fromUtf8(":/images/resource-files/icons/shape_3d_notSet.png"), QSize(), QIcon::Normal, QIcon::Off);
	}
	this->setWindowIcon(icon);
}

void RigidBodyDialog::updateLabels()
{
	for (int i = 0; i <comboBox.size(); i++)
	{
		label_RefPoint[i]->setText(m_body->getReferenceNames()[i]);
		label_DescPoint[i]->setText(m_body->getMarker(comboBox[i]->currentText().toInt() - 1)->getDescription());
	}
}

RigidBodyDialog::~RigidBodyDialog(){
	delete diag;

	for (int i = 0; i < comboBox.size(); i++){
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
	for (int i = 0; i < comboBox.size(); i++)
	{
		for (int j = 0; j < comboBox.size(); j++)
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
	for (int i = 0; i < comboBox.size(); i++)
	{
		for (int j = 0; j < comboBox.size(); j++)
		{
			if (i != j && comboBox[i]->currentText() == comboBox[j]->currentText()) valid_Selection = false;
		}
	}

	if (!valid_Selection)
	{
		ErrorDialog::getInstance()->showErrorDialog("Each Marker can only be used once. Please correct your selection.");
		return false;
	}

	for (int j = 0; j < m_body->getPointsIdx().size(); j++)
	{
		m_body->setPointIdx(j, comboBox[j]->currentText().toInt() - 1);
	}

	return true;
}

void RigidBodyDialog::updateColorButton()
{
	QPixmap pix(16,16);
	pix.fill(m_body->getColor());
	diag->toolButton_Color->setIcon(pix);
}

void RigidBodyDialog::reloadDummyPoints()
{
	for (int i = 0; i < label_Dummy.size(); i++)
	{
		diag->gridLayout_5->removeWidget(label_Dummy[i]);
		delete label_Dummy[i];
	}
	label_Dummy.clear();

	for (int i = 0; i < m_body->getDummyNames().size(); i++)
	{
		QLabel * label = new QLabel(m_body->getDummyNames()[i]);
		diag->gridLayout_5->addWidget(label , i + 1, 0,1,2);
		label_Dummy.push_back(label);
	}
}

void RigidBodyDialog::on_pushButton_setFromFile_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open rigid body references"), Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));

	if (fileName.isNull() == false){

		if (m_body->isReferencesSet())
		{
			if (!ConfirmationDialog::getInstance()->showConfirmationDialog("References are already set by using a file. Are you sure you want to reset the references?"))
			{
				return;
			}
			m_body->setReferencesSet(false);
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




void RigidBodyDialog::on_pushButton_OK_clicked(){
	if (isComplete()) this->accept();
}

void RigidBodyDialog::on_pushButton_Cancel_clicked(){
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
	QColorDialog * cdiag = new QColorDialog();
	cdiag->setCurrentColor(m_body->getColor());
	cdiag->exec();

	m_body->setColor(cdiag->currentColor());

	updateColorButton();

	delete cdiag;
}

void RigidBodyDialog::on_checkBoxCutoffOverride_clicked()
{
	if (diag->checkBoxCutoffOverride->isChecked()){
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
	if (diag->checkBoxCutoffOverride->isChecked()){
		m_body->setCutoffFrequency(value);
	}
}

void RigidBodyDialog::on_pushButton_AddDummy_clicked()
{
	bool ok;
	QString name = QInputDialog::getText(this, tr("QInputDialog::getText()"),
		tr("Enter a name for the virtual point:"), QLineEdit::Normal, "Virtual " + QString::number(m_body->getDummyNames().size()+1), &ok);
	QString filenameCoords;
	QString filenameRef;

	if (ok && !name.isEmpty())
	{
		filenameRef = QFileDialog::getOpenFileName(this, tr("Open CT coordinate file"), Settings::getInstance()->getLastUsedDirectory(), ("CSV Files (*.csv)"));
		if (!filenameRef.isEmpty())
		{
			Settings::getInstance()->setLastUsedDirectory(filenameRef);
		}
		else
		{
			return;
		}
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
	else
	{
		return;
	}

	m_body->addDummyPoint(name, filenameRef, filenameCoords);
	
	reloadDummyPoints();

	m_body->recomputeTransformations();
}

void RigidBodyDialog::on_pushButton_DeleteDummy_clicked()
{
	m_body->clearAllDummyPoints();

	reloadDummyPoints();

	m_body->recomputeTransformations();
}