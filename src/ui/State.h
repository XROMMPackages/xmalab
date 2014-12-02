#ifndef STATE_H
#define STATE_H

#include <QObject>

enum work_state {UNDISTORTION = 0, CALIBRATION = 1};
enum undistortion_state {NOTUNDISTORTED = 0, UNDISTORTED = 1};
enum undistortionMouseMode_state {UNDISTNOMOUSEMODE = 0 , UNDISTTOGGLEOUTLIER = 1, UNDISTSETCENTER = 2};
enum undistortionVisImage_state {DISTORTEDUNDISTIMAGE = 0, UNDISTORTEDUNDISTIMAGE = 1, MAPANGLESCALED = 2, MAPANGLES = 3 , MAPDISTANCE = 4, MAPHORIZONTAL = 5, MAPVERTICAL = 6};
enum undistortionVisPoints_state {NOUNDISTPOINTS = 0 , DETECTEDUNDISTPOINTS = 1, GRIDDISTORTEDUNDISTPOINTS = 2, GRIDUNDISTORTEDUNDISTPOINTS = 3, REFERENCEUNDISTPOINTS = 4};
enum calibrationVisImage_state {DISTORTEDCALIBIMAGE = 0, UNDISTORTEDCALIBIMAGE = 1};
enum calibrationVisPoints_state {NOCALIBPOINTS = 0 , DETECTEDALLCALIBPOINTS = 1, DETCUBEDISTORTEDCALIBPOINTS = 2, DETCUBEUNDISTORTEDCALIBPOINTS = 3, PROJCUBEDISTORTEDCALIBPOINTS = 4, PROJCUBEUNDISTORTEDCALIBPOINTS = 5};
enum calibrationVisText_state {NOCALIBTEXT = 0 , IDCALIBTEXT = 1, ERRORCALIBTEXT = 2, ERRORCOLORCALIBTEXT = 3};

enum ui_state {SINGLE_CAMERA = 0, ALL_CAMERAS_FULL_HEIGHT = 1, ALL_CAMERAS_1ROW_SCALED = 2, ALL_CAMERAS_2ROW_SCALED = 3, ALL_CAMERAS_3ROW_SCALED = 4};

class State : public QObject{
	
	Q_OBJECT

	public:
		static State* getInstance();
		~State();

		void changeWorkspace(work_state newWorkspace);
		void changeDisplay(ui_state newDisplay);
		void changeActiveCamera(int newActiveCamera);
		void changeActiveFrame(int newActiveFrame);
		void changeUndistortion(undistortion_state newUndistortion);
		void changeUndistortionVisImage(undistortionVisImage_state newUndistortionVisImage);
		void changeUndistortionVisPoints(undistortionVisPoints_state newUndistortionVisPoints);
		void changeUndistortionMouseMode(undistortionMouseMode_state newUndistortionMouseMode);
		void changeCalibrationVisImage(calibrationVisImage_state newCalibrationVisImage);
		void changeCalibrationVisPoints(calibrationVisPoints_state newCalibrationVisPoints);
		void changeCalibrationVisText(calibrationVisText_state newCalibrationVisText);

		work_state getWorkspace(){return workspace;}
		ui_state getDisplay(){return display;}
		int getActiveCamera(){return activeCamera;}
		int getActiveFrame(){return activeFrame;}
		undistortion_state getUndistortion(){return undistortion;}
		undistortionVisPoints_state getUndistortionVisPoints(){return undistortionVisPoints;}
		undistortionVisImage_state getUndistortionVisImage(){return undistortionVisImage;}
		undistortionMouseMode_state getUndistortionMouseMode(){return undistortionMouseMode;}
		calibrationVisPoints_state getCalibrationVisPoints(){return calibrationVisPoints;}
		calibrationVisImage_state getCalibrationVisImage(){return calibrationVisImage;}
		calibrationVisText_state getCalibrationVisText(){return calibrationVisText;}
		
	signals:
		void workspaceChanged(work_state workspace);  
		void displayChanged(ui_state display);  
		void activeCameraChanged(int activeCamera);  
		void activeFrameChanged(int activeFrame);  
		void undistortionChanged(undistortion_state undistortion);  
		void undistortionVisImageChanged(undistortionVisImage_state undistortionVisImage);  
		void undistortionVisPointsChanged(undistortionVisPoints_state undistortionVisPoints);  
		void undistortionMouseModeChanged(undistortionMouseMode_state undistortionMouseMode);  
		void calibrationVisImageChanged(calibrationVisImage_state calibrationVisImage);  
		void calibrationVisPointsChanged(calibrationVisPoints_state calibrationVisPoints);  
		void calibrationVisTextChanged(calibrationVisText_state calibrationVisText);  
		
	private :
		work_state workspace;
		ui_state display;
		undistortion_state undistortion;
		undistortionVisImage_state undistortionVisImage;
		undistortionVisPoints_state undistortionVisPoints;
		undistortionMouseMode_state undistortionMouseMode;
		calibrationVisImage_state calibrationVisImage;
		calibrationVisPoints_state calibrationVisPoints;
		calibrationVisText_state calibrationVisText;
		int activeFrame;
		int activeCamera;

		State();
		static State* instance;
};


	

#endif  // STATE_H
