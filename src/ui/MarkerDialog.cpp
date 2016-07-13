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
///\file MarkerDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MarkerDialog.h"
#include "ui_MarkerDialog.h"
#include "ui/State.h"

#include "core/Marker.h"

using namespace xma;

MarkerDialog::MarkerDialog(Marker* marker, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::MarkerDialog), m_marker(marker)
{
	diag->setupUi(this);

	diag->spinBox_MarkerRadius->setValue(m_marker->getSizeOverride());
	diag->spinBox_ThresholdOffset->setValue(m_marker->getThresholdOffset());
	diag->spinBox_Penalty->setValue(m_marker->getMaxPenalty());
	diag->comboBox_Method->setCurrentIndex(m_marker->getMethod());
	diag->comboBox_Interpolation->setCurrentIndex(m_marker->getInterpolation());
}


MarkerDialog::~MarkerDialog()
{
	delete diag;
}

bool MarkerDialog::isComplete()
{
	m_marker->setMaxPenalty(diag->spinBox_Penalty->value());
	m_marker->setMethod(diag->comboBox_Method->currentIndex());
	m_marker->setInterpolation(diag->comboBox_Interpolation->currentIndex());
	m_marker->setSizeOverride(diag->spinBox_MarkerRadius->value());
	m_marker->setThresholdOffset(diag->spinBox_ThresholdOffset->value());

	return true;
}

void MarkerDialog::on_pushButton_OK_clicked()
{
	if (isComplete()) this->accept();
}

void MarkerDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

