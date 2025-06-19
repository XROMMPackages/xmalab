//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file State.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/State.h"

using namespace xma;

State* State::instance = NULL;

State::State()
{
	workspace = CALIBRATION;
	display = ALL_CAMERAS_1ROW_SCALED;
	activeFrameCalibration = -1;
	activeCamera = -1;
	activeFrameTrial = -1;
	activeTrial = -1;
	undistortion = NOTUNDISTORTED;
	undistortionVisImage = DISTORTEDUNDISTIMAGE;
	undistortionVisPoints = NOUNDISTPOINTS;
	undistortionMouseMode = UNDISTNOMOUSEMODE;
	calibrationVisImage = DISTORTEDCALIBIMAGE;
	calibrationVisPoints = NOCALIBPOINTS;
	calibrationVisText = NOCALIBTEXT;
	disableDraw = false;
	loading = false;
}

State::~State()
{
	instance = NULL;
}

State* State::getInstance()
{
	if (!instance)
	{
		instance = new State();
	}
	return instance;
}


void State::changeWorkspace(work_state newWorkspace, bool force)
{
	if (force || workspace != newWorkspace)
	{
		workspace = newWorkspace;
		emit workspaceChanged(newWorkspace);
	}
}

void State::changeDisplay(ui_state newDisplay, bool force)
{
	if (force || display != newDisplay)
	{
		display = newDisplay;
		emit displayChanged(newDisplay);
	}
}

void State::changeActiveCamera(int newActiveCamera, bool force)
{
	if (force || activeCamera != newActiveCamera)
	{
		activeCamera = newActiveCamera;
		emit activeCameraChanged(newActiveCamera);
	}
}

void State::changeActiveTrial(int newActiveTrial, bool force)
{
	if (force || activeTrial != newActiveTrial)
	{
		activeTrial = newActiveTrial;
		emit activeTrialChanged(newActiveTrial);
	}
}

void State::changeActiveFrameTrial(int newActiveFrameTrial, bool force)
{
	if (force || activeFrameTrial != newActiveFrameTrial)
	{
		activeFrameTrial = newActiveFrameTrial;
		emit activeFrameTrialChanged(newActiveFrameTrial);
	}
}

void State::changeActiveFrameCalibration(int newActiveFrame, bool force)
{
	if (force || activeFrameCalibration != newActiveFrame)
	{
		activeFrameCalibration = newActiveFrame;
		emit activeFrameCalibrationChanged(newActiveFrame);
	}
}

void State::changeUndistortion(undistortion_state newUndistortion, bool force)
{
	if (force || undistortion != newUndistortion)
	{
		undistortion = newUndistortion;
		emit undistortionChanged(newUndistortion);
	}
}

void State::changeUndistortionVisImage(undistortionVisImage_state newUndistortionVisImage, bool force)
{
	if (force || undistortionVisImage != newUndistortionVisImage)
	{
		undistortionVisImage = newUndistortionVisImage;
		emit undistortionVisImageChanged(newUndistortionVisImage);
	}
}

void State::changeUndistortionVisPoints(undistortionVisPoints_state newUndistortionVisPoints, bool force)
{
	if (force || undistortionVisPoints != newUndistortionVisPoints)
	{
		undistortionVisPoints = newUndistortionVisPoints;
		emit undistortionVisPointsChanged(newUndistortionVisPoints);
	}
}

void State::changeUndistortionMouseMode(undistortionMouseMode_state newUndistortionMouseMode, bool force)
{
	if (force || undistortionMouseMode != newUndistortionMouseMode)
	{
		undistortionMouseMode = newUndistortionMouseMode;
		emit undistortionMouseModeChanged(newUndistortionMouseMode);
	}
}

void State::changeCalibrationVisImage(calibrationVisImage_state newCalibrationVisImage, bool force)
{
	if (force || calibrationVisImage != newCalibrationVisImage)
	{
		calibrationVisImage = newCalibrationVisImage;
		emit calibrationVisImageChanged(newCalibrationVisImage);
	}
}

void State::changeCalibrationVisPoints(calibrationVisPoints_state newCalibrationVisPoints, bool force)
{
	if (force || calibrationVisPoints != newCalibrationVisPoints)
	{
		calibrationVisPoints = newCalibrationVisPoints;
		emit calibrationVisPointsChanged(newCalibrationVisPoints);
	}
}

void State::changeCalibrationVisText(calibrationVisText_state newCalibrationVisText, bool force)
{
	if (force || calibrationVisText != newCalibrationVisText)
	{
		calibrationVisText = newCalibrationVisText;
		emit calibrationVisTextChanged(newCalibrationVisText);
	}
}

bool State::isLoading()
{
	return loading;
}

void State::setLoading(bool value)
{
	loading = value;
}

