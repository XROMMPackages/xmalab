#include "ui/State.h"

State* State::instance = NULL;

State::State(){
	workspace = CALIBRATION;
	display = ALL_CAMERAS_1ROW_SCALED;
	activeFrame = -1;
	activeCamera = -1;
	undistortion = NOTUNDISTORTED;
	undistortionVisImage = DISTORTEDUNDISTIMAGE;
	undistortionVisPoints = NOUNDISTPOINTS;
	undistortionMouseMode = UNDISTNOMOUSEMODE;
	calibrationVisImage = DISTORTEDCALIBIMAGE;
	calibrationVisPoints = NOCALIBPOINTS;
}

State::~State(){
	instance = NULL;
}

State* State::getInstance()
{
	if(!instance) 
	{
		instance = new State();
	}
	return instance;
}


void State::changeWorkspace(work_state newWorkspace){
	if(workspace != newWorkspace){
		workspace = newWorkspace;
		emit workspaceChanged(newWorkspace);
	}
}

void State::changeDisplay(ui_state newDisplay){
	if(display != newDisplay){
		display = newDisplay;
		emit displayChanged(newDisplay);
	}
}

void State::changeActiveCamera(int newActiveCamera){
	if(activeCamera != newActiveCamera){
		activeCamera = newActiveCamera;
		emit activeCameraChanged(newActiveCamera);
	}
}

void State::changeActiveFrame(int newActiveFrame){
	if(activeFrame != newActiveFrame){
		activeFrame = newActiveFrame;
		emit activeFrameChanged(newActiveFrame);
	}
}

void State::changeUndistortion(undistortion_state newUndistortion){
	if(undistortion != newUndistortion){
		undistortion = newUndistortion;
		emit undistortionChanged(newUndistortion);
	}
}

void State::changeUndistortionVisImage(undistortionVisImage_state newUndistortionVisImage){
	if(undistortionVisImage != newUndistortionVisImage){
		undistortionVisImage = newUndistortionVisImage;
		emit undistortionVisImageChanged(newUndistortionVisImage);
	}
}

void State::changeUndistortionVisPoints(undistortionVisPoints_state newUndistortionVisPoints){
	if(undistortionVisPoints != newUndistortionVisPoints){
		undistortionVisPoints = newUndistortionVisPoints;
		emit undistortionVisPointsChanged(newUndistortionVisPoints);
	}
}

void State::changeUndistortionMouseMode(undistortionMouseMode_state newUndistortionMouseMode){
	if(undistortionMouseMode != newUndistortionMouseMode){
		undistortionMouseMode = newUndistortionMouseMode;
		emit undistortionMouseModeChanged(newUndistortionMouseMode);
	}
}

void State::changeCalibrationVisImage(calibrationVisImage_state newCalibrationVisImage){
	if(calibrationVisImage != newCalibrationVisImage){
		calibrationVisImage = newCalibrationVisImage;
		emit calibrationVisImageChanged(newCalibrationVisImage);
	}
}

void State::changeCalibrationVisPoints(calibrationVisPoints_state newCalibrationVisPoints){
	if(calibrationVisPoints != newCalibrationVisPoints){
		calibrationVisPoints = newCalibrationVisPoints;
		emit calibrationVisPointsChanged(newCalibrationVisPoints);
	}
}