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
///\file PointImportExportDialog.cpp
///\author Benjamin Knorlein
///\date 11/2groupBoxRecentFiles0/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/PointImportExportDialog.h"
#include "ui_PointImportExportDialog.h"
#include "core/Settings.h"

using namespace xma;

PointImportExportDialog::PointImportExportDialog(ImportExportType type, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::PointImportExportDialog),
	m_type(type)
{
	diag->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	switch (m_type)
	{
	case IMPORT2D:
		this->setWindowTitle("Import 2D points");
		diag->frame_File->hide();
		diag->frame_Filtered->hide();

		diag->radioButton_Count0->setChecked(Settings::getInstance()->getBoolSetting("Import2DCount0"));
		diag->radioButton_Count1->setChecked(Settings::getInstance()->getBoolSetting("Import2DCount1"));
		diag->radioButton_YDown->setChecked(Settings::getInstance()->getBoolSetting("Import2DYDown"));
		diag->radioButton_YUp->setChecked(Settings::getInstance()->getBoolSetting("Import2DYUp"));
		diag->radioButton_NoHeader->setChecked(Settings::getInstance()->getBoolSetting("Import2DNoHeader"));
		diag->radioButton_Header->setChecked(Settings::getInstance()->getBoolSetting("Import2DHeader"));
		diag->radioButton_Distorted->setChecked(Settings::getInstance()->getBoolSetting("Import2DDistorted"));
		diag->radioButton_Undistorted->setChecked(Settings::getInstance()->getBoolSetting("Import2DUndistorted"));
		diag->radioButton_NoCols->setChecked(Settings::getInstance()->getBoolSetting("Import2DNoCols"));
		diag->radioButton_OffsetCols->setChecked(Settings::getInstance()->getBoolSetting("Import2DOffsetCols"));
		diag->radioButton_OffsetCols->setText("cam offset columns(Matlab xromm tools)");
		break;
	case EXPORT2D:
		this->setWindowTitle("Export 2D points");
		diag->frame_Filtered->hide();

		diag->radioButton_Multi->setChecked(Settings::getInstance()->getBoolSetting("Export2DMulti"));
		diag->radioButton_Single->setChecked(Settings::getInstance()->getBoolSetting("Export2DSingle"));
		diag->radioButton_Count0->setChecked(Settings::getInstance()->getBoolSetting("Export2DCount0"));
		diag->radioButton_Count1->setChecked(Settings::getInstance()->getBoolSetting("Export2DCount1"));
		diag->radioButton_YDown->setChecked(Settings::getInstance()->getBoolSetting("Export2DYDown"));
		diag->radioButton_YUp->setChecked(Settings::getInstance()->getBoolSetting("Export2DYUp"));
		diag->radioButton_NoHeader->setChecked(Settings::getInstance()->getBoolSetting("Export2DNoHeader"));
		diag->radioButton_Header->setChecked(Settings::getInstance()->getBoolSetting("Export2DHeader"));
		diag->radioButton_Distorted->setChecked(Settings::getInstance()->getBoolSetting("Export2DDistorted"));
		diag->radioButton_Undistorted->setChecked(Settings::getInstance()->getBoolSetting("Export2DUndistorted"));
		diag->radioButton_NoCols->setChecked(Settings::getInstance()->getBoolSetting("Export2DNoCols"));
		diag->radioButton_OffsetCols->setChecked(Settings::getInstance()->getBoolSetting("Export2DOffsetCols"));
		diag->radioButton_OffsetCols->setText("cam offset columns(Matlab xromm tools)");
		break;
	case EXPORT3D:
		this->setWindowTitle("Export 3D points");

		diag->frame_YAxis->hide();
		diag->frame_Start->hide();
		diag->frame_Distorted->hide();
		//diag->frame_Columns->hide();
		diag->frame_Filtered->hide();

		diag->radioButton_Multi->setChecked(Settings::getInstance()->getBoolSetting("Export3DMulti"));
		diag->radioButton_Single->setChecked(Settings::getInstance()->getBoolSetting("Export3DSingle"));
		diag->radioButton_NoHeader->setChecked(Settings::getInstance()->getBoolSetting("Export3DNoHeader"));
		diag->radioButton_Header->setChecked(Settings::getInstance()->getBoolSetting("Export3DHeader"));
		diag->radioButton_NoCols->setChecked(Settings::getInstance()->getBoolSetting("Export3DNoCols"));
		diag->radioButton_OffsetCols->setChecked(Settings::getInstance()->getBoolSetting("Export3DOffsetCols"));
		diag->radioButton_OffsetCols->setText("Frame Number (allows saving of subsequence)");
		break;

	case EXPORTTRANS:
		diag->frame_YAxis->hide();
		diag->frame_Start->hide();
		diag->frame_Distorted->hide();
		//diag->frame_Columns->hide();

		diag->radioButton_Multi->setChecked(Settings::getInstance()->getBoolSetting("ExportTransMulti"));
		diag->radioButton_Single->setChecked(Settings::getInstance()->getBoolSetting("ExportTransSingle"));
		diag->radioButton_NoHeader->setChecked(Settings::getInstance()->getBoolSetting("ExportTransNoHeader"));
		diag->radioButton_Header->setChecked(Settings::getInstance()->getBoolSetting("ExportTransHeader"));
		diag->radioButton_Unfiltered->setChecked(Settings::getInstance()->getBoolSetting("ExportTransUnfiltered"));
		diag->radioButton_Filtered->setChecked(Settings::getInstance()->getBoolSetting("ExportTransFiltered"));
		diag->radioButton_NoCols->setChecked(Settings::getInstance()->getBoolSetting("ExportTransNoCols"));
		diag->radioButton_OffsetCols->setChecked(Settings::getInstance()->getBoolSetting("ExportTransOffsetCols"));
		diag->radioButton_OffsetCols->setText("Frame Number (allows saving of subsequence)");
		break;
	}

	diag->gridLayout_5->setSizeConstraint(QLayout::SetFixedSize);
}

PointImportExportDialog::~PointImportExportDialog()
{
	delete diag;
}

void PointImportExportDialog::on_radioButton_Single_toggled(bool value)
{
	switch (m_type)
	{
	case EXPORT2D:
		Settings::getInstance()->set("Export2DSingle", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DSingle", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransSingle", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Multi_toggled(bool value)
{
	switch (m_type)
	{
	case EXPORT2D:
		Settings::getInstance()->set("Export2DMulti", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DMulti", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransMulti", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Count0_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DCount0", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DCount0", value);
		break;
    default:
        break;
    }
}

void PointImportExportDialog::on_radioButton_Count1_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DCount1", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DCount1", value);
            break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Header_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DHeader", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DHeader", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DHeader", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransHeader", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_NoHeader_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DNoHeader", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DNoHeader", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DNoHeader", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransNoHeader", value);
		break;
    default:
        break;	}
}

void PointImportExportDialog::on_radioButton_YDown_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DYDown", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DYDown", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_YUp_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DYUp", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DYUp", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Distorted_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DDistorted", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DDistorted", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Undistorted_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DUndistorted", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DUndistorted", value);
		break;
    default:
        break;	}
}

void PointImportExportDialog::on_radioButton_NoCols_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DNoCols", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DNoCols", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DNoCols", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransNoCols", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_OffsetCols_toggled(bool value)
{
	switch (m_type)
	{
	case IMPORT2D:
		Settings::getInstance()->set("Import2DOffsetCols", value);
		break;
	case EXPORT2D:
		Settings::getInstance()->set("Export2DOffsetCols", value);
		break;
	case EXPORT3D:
		Settings::getInstance()->set("Export3DOffsetCols", value);
		break;
	case EXPORTTRANS:
		Settings::getInstance()->set("ExportTransOffsetCols", value);
		break;
    default:
        break;
	}
}

void PointImportExportDialog::on_radioButton_Unfiltered_toggled(bool value)
{
	Settings::getInstance()->set("ExportTransUnfiltered", value);
}

void PointImportExportDialog::on_radioButton_Filtered_toggled(bool value)
{
	Settings::getInstance()->set("ExportTransFiltered", value);
}


void PointImportExportDialog::on_pushButton_OK_clicked()
{
	this->accept();
}

void PointImportExportDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

