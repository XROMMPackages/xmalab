#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CineVideo.h"

#include <fstream>      

#include <QtCore/QFileInfo>

#include <opencv/cv.h>

using namespace xma;

//BYTE 8 - bit unsigned integer
//CHAR 8 - bit signed integer
//WORD 16 - bit(2 - byte) unsigned integer
//INT16, SHORT 16 - bit(2 - byte) signed integer
//BOOL 32 - bit(4 - byte) logic value(TRUE = 1, FALSE = 0)
//DWORD, UINT 32 - bit(4 - byte) unsigned integer
//LONG, INT 32 - bit(4 - byte) signed integer
//FLOAT 32 - bit(4 - byte) floating point
//DOUBLE 64 - bit(8 - byte) floating point
//STRING array of chars


#define SHOW(a) std::cout << #a << ": " << (a) << std::endl
#define SHOWNAME(a) std::cout << #a << ": " 
#define SHOWSTRING(a) std::cout << #a << ": " <<  std::string(a) << std::endl
#define SHOWSTRINGONLY(a) std::cout << std::string(a) << " "
#define SHOWVALONLY(a) std::cout << a << " "
#define SHOWEND std::cout << std::endl
#define printHeader false

template<typename T> void readAndAdvance(T &output, char* &ptr)
{
	T * Tptr = (T *) ptr; ptr += sizeof(T);
	output = *Tptr;
}

template<typename T> void readAndAdvance(T *output, char* &ptr, int length)
{
	for (int i = 0; i < length; i++){
		T * Tptr = (T *) ptr; ptr += sizeof(T);
		output[i] = *Tptr;
	}
}

CineVideo::CineVideo(QStringList _filenames) :VideoStream(_filenames){
	loadCineInfo();
}

void CineVideo::loadCineInfo()
{
	//Open File
	if (QFile::exists(filenames.at(0))){
		std::ifstream is(filenames.at(0).toAscii().data(), std::ifstream::binary);

		char * counter;
		if (printHeader)std::cout << "Read Cine File " << filenames.at(0).toAscii().data() << std::endl;
		//Read CINEFILEHEADER
		is.seekg(0);
		char * header = new char[44];
		counter = header;
		is.read(header, 44);

		readAndAdvance(&Type[0], counter, 2);
		if (printHeader) SHOWSTRING(Type);
		readAndAdvance(Headersize, counter);
		if (printHeader) SHOW(Headersize);
		readAndAdvance(Compression, counter);
		if (printHeader) SHOW(Compression);
		readAndAdvance(Version, counter);
		if (printHeader) SHOW(Version);

		readAndAdvance(FirstMovieImage, counter);
		if (printHeader) SHOW(FirstMovieImage);
		readAndAdvance(TotalImageCount, counter);
		if (printHeader) SHOW(TotalImageCount);
		readAndAdvance(FirstImageNo, counter);
		if (printHeader) SHOW(FirstImageNo);
		readAndAdvance(ImageCount, counter);
		if (printHeader) SHOW(ImageCount);

		readAndAdvance(OffImageHeader, counter);
		if (printHeader) SHOW(OffImageHeader);
		readAndAdvance(OffSetup, counter);
		if (printHeader) SHOW(OffSetup);
		readAndAdvance(OffImageOffsets, counter);
		if (printHeader) SHOW(OffImageOffsets);

		readAndAdvance(TriggerTimeFractions, counter);
		if (printHeader) SHOW(TriggerTimeFractions);
		readAndAdvance(TriggerTimeSections, counter);
		if (printHeader) SHOW(TriggerTimeSections);
		delete[] header;

		//Read BITMAPINFOHEADER size
		is.seekg(OffImageHeader);
		DWORD lengthBitmapInfoHeader;
		is.read((char*)&lengthBitmapInfoHeader, sizeof(DWORD));

		//Read BITMAPINFOHEADER 
		is.seekg(OffImageHeader);
		char * bitmapInfoHeader = new char[lengthBitmapInfoHeader];
		counter = bitmapInfoHeader;
		is.read(bitmapInfoHeader, lengthBitmapInfoHeader);

		readAndAdvance(biSize, counter);
		if (printHeader) SHOW(biSize);
		readAndAdvance(biWidth, counter);
		if (printHeader) SHOW(biWidth);
		readAndAdvance(biHeight, counter);
		if (printHeader) SHOW(biHeight);
		readAndAdvance(biPlanes, counter);
		if (printHeader) SHOW(biPlanes);
		readAndAdvance(biBitCount, counter);
		if (printHeader) SHOW(biBitCount);
		readAndAdvance(biCompression, counter);
		if (printHeader) SHOW(biCompression);
		readAndAdvance(biSizeImage, counter);
		if (printHeader) SHOW(biSizeImage);
		readAndAdvance(biXPelsPerMeter, counter);
		if (printHeader) SHOW(biXPelsPerMeter);
		readAndAdvance(biYPelsPerMeter, counter);
		if (printHeader) SHOW(biYPelsPerMeter);
		readAndAdvance(biClrUsed, counter);
		if (printHeader) SHOW(biClrUsed);

		delete[]bitmapInfoHeader;

		is.seekg(OffSetup);
		char * camerasetup = new char[5740];
		counter = camerasetup;
		is.read(camerasetup, 5740);

		readAndAdvance(FrameRate16, counter);
		if (printHeader) SHOW(FrameRate16);
		readAndAdvance(Shutter16, counter);
		if (printHeader) SHOW(Shutter16);
		readAndAdvance(PostTrigger16, counter);
		if (printHeader) SHOW(PostTrigger16);
		readAndAdvance(FrameDelay16, counter);
		if (printHeader) SHOW(FrameDelay16);
		readAndAdvance(AspectRatio, counter);
		if (printHeader) SHOW(AspectRatio);
		readAndAdvance(Contrast16, counter);
		if (printHeader) SHOW(Contrast16);
		readAndAdvance(Bright16, counter);
		if (printHeader) SHOW(Bright16);
		readAndAdvance(Rotate16, counter);
		if (printHeader) SHOW(Rotate16);
		readAndAdvance(TimeAnnotation, counter);
		if (printHeader) SHOW(TimeAnnotation);
		readAndAdvance(TrigCine, counter);
		if (printHeader) SHOW(TrigCine);
		readAndAdvance(TrigFrame, counter);
		if (printHeader) SHOW(TrigFrame);
		readAndAdvance(ShutterOn, counter);
		if (printHeader) SHOW(ShutterOn);
		readAndAdvance(&DescriptionOld[0], counter, MAXLENDESCRIPTION_OLD);
		if (printHeader) SHOWSTRING(DescriptionOld);
		readAndAdvance(&Mark[0], counter, 2);
		if (printHeader) SHOWSTRING(Mark);
		readAndAdvance(Length, counter);
		if (printHeader) SHOW(Length);
		readAndAdvance(Binning, counter);
		if (printHeader) SHOW(Binning);
		readAndAdvance(SigOption, counter);
		if (printHeader) SHOW(SigOption);
		readAndAdvance(BinChannels, counter);
		if (printHeader) SHOW(BinChannels);
		readAndAdvance(SamplesPerImage, counter);
		if (printHeader) SHOW(SamplesPerImage);
		for (int i = 0; i < 8; i++)readAndAdvance(&BinName[i][0], counter, 11);
		if (printHeader){ SHOWNAME(BinName); for (int i = 0; i < 8; i++)SHOWSTRINGONLY(BinName[i]); SHOWEND; }
		readAndAdvance(AnaOption, counter);
		if (printHeader) SHOW(AnaOption);
		readAndAdvance(AnaChannels, counter);
		if (printHeader) SHOW(AnaChannels);
		readAndAdvance(Res6, counter);
		if (printHeader) SHOW(Res6);
		readAndAdvance(AnaBoard, counter);
		if (printHeader) SHOW(AnaBoard);
		readAndAdvance(ChOption, counter, 8);
		if (printHeader){ SHOWNAME(ChOption); for (int i = 0; i < 8; i++) SHOWVALONLY(ChOption[i]); SHOWEND; }
		readAndAdvance(AnaGain, counter, 8);
		if (printHeader){ SHOWNAME(AnaGain); for (int i = 0; i < 8; i++)SHOWVALONLY(AnaGain[i]); SHOWEND; }
		for (int i = 0; i < 8; i++)readAndAdvance(&AnaUnit[i][0], counter, 6);
		if (printHeader){ SHOWNAME(AnaUnit); for (int i = 0; i < 8; i++)SHOWSTRINGONLY(AnaUnit[i]); SHOWEND; }
		for (int i = 0; i < 8; i++)readAndAdvance(&AnaName[i][0], counter, 11);
		if (printHeader){ SHOWNAME(AnaName); for (int i = 0; i < 8; i++)SHOWSTRINGONLY(AnaName[i]); SHOWEND; }
		readAndAdvance(lFirstImage, counter);
		if (printHeader) SHOW(lFirstImage);
		readAndAdvance(dwImageCount, counter);
		if (printHeader) SHOW(dwImageCount);
		readAndAdvance(nQFactor, counter);
		if (printHeader) SHOW(nQFactor);
		readAndAdvance(wCineFileType, counter);
		if (printHeader) SHOW(wCineFileType);
		for (int i = 0; i < 4; i++)readAndAdvance(&szCinePath[i][0], counter, 65);
		if (printHeader){ SHOWNAME(szCinePath); for (int i = 0; i < 4; i++)SHOWSTRINGONLY(szCinePath[i]); SHOWEND; }
		readAndAdvance(bMainsFreq, counter);
		if (printHeader) SHOW(bMainsFreq);
		readAndAdvance(bTimeCode, counter);
		if (printHeader) SHOW(bTimeCode);
		readAndAdvance(bPriority, counter);
		if (printHeader) SHOW(bPriority);
		readAndAdvance(wLeapSecDY, counter);
		if (printHeader) SHOW(wLeapSecDY);
		readAndAdvance(dDelayTC, counter);
		if (printHeader) SHOW(dDelayTC);
		readAndAdvance(dDelayPPS, counter);
		if (printHeader) SHOW(dDelayPPS);
		readAndAdvance(GenBits, counter);
		if (printHeader) SHOW(GenBits);
		readAndAdvance(Res1, counter);
		if (printHeader) SHOW(Res1);
		readAndAdvance(Res2, counter);
		if (printHeader) SHOW(Res2);
		readAndAdvance(Res3, counter);
		if (printHeader) SHOW(Res3);
		readAndAdvance(ImWidth, counter);
		if (printHeader) SHOW(ImWidth);
		readAndAdvance(ImHeight, counter);
		if (printHeader) SHOW(ImHeight);
		readAndAdvance(EDRShutter16, counter);
		if (printHeader) SHOW(EDRShutter16);
		readAndAdvance(Serial, counter);
		if (printHeader) SHOW(Serial);
		readAndAdvance(Saturation, counter);
		if (printHeader) SHOW(Saturation);
		readAndAdvance(Res5, counter);
		if (printHeader) SHOW(Res5);
		readAndAdvance(AutoExposure, counter);
		if (printHeader) SHOW(AutoExposure);
		readAndAdvance(bFlipH, counter);
		if (printHeader) SHOW(bFlipH);
		readAndAdvance(bFlipV, counter);
		if (printHeader) SHOW(bFlipV);
		readAndAdvance(Grid, counter);
		if (printHeader) SHOW(Grid);
		readAndAdvance(FrameRate, counter);
		if (printHeader) SHOW(FrameRate);
		readAndAdvance(Shutter, counter);
		if (printHeader) SHOW(Shutter);
		readAndAdvance(EDRShutter, counter);
		if (printHeader) SHOW(EDRShutter);
		readAndAdvance(PostTrigger, counter);
		if (printHeader) SHOW(PostTrigger);
		readAndAdvance(FrameDelay, counter);
		if (printHeader) SHOW(FrameDelay);
		readAndAdvance(bEnableColor, counter);
		if (printHeader) SHOW(bEnableColor);
		readAndAdvance(CameraVersion, counter);
		if (printHeader) SHOW(CameraVersion);
		readAndAdvance(FirmwareVersion, counter);
		if (printHeader) SHOW(FirmwareVersion);
		readAndAdvance(SoftwareVersion, counter);
		if (printHeader) SHOW(SoftwareVersion);
		readAndAdvance(RecordingTimeZone, counter);
		if (printHeader) SHOW(RecordingTimeZone);
		readAndAdvance(CFA, counter);
		if (printHeader) SHOW(CFA);
		readAndAdvance(Bright, counter);
		if (printHeader) SHOW(Bright);
		readAndAdvance(Contrast, counter);
		if (printHeader) SHOW(Contrast);
		readAndAdvance(Gamma, counter);
		if (printHeader) SHOW(Gamma);
		readAndAdvance(Reserved1, counter);
		if (printHeader) SHOW(Reserved1);
		readAndAdvance(AutoExpLevel, counter);
		if (printHeader) SHOW(AutoExpLevel);
		readAndAdvance(AutoExpSpeed, counter);
		if (printHeader) SHOW(AutoExpSpeed);
		readAndAdvance(AutoExpRect, counter, 4);
		if (printHeader){ SHOWNAME(AutoExpRect); for (int i = 0; i < 4; i++) SHOWVALONLY(AutoExpRect[i]); SHOWEND; }
		readAndAdvance(WBGain1, counter, 2);
		if (printHeader){ SHOWNAME(WBGain1); for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain1[i]);  SHOWEND; }
		readAndAdvance(WBGain2, counter, 2);
		if (printHeader){ SHOWNAME(WBGain2); for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain2[i]);  SHOWEND; }
		readAndAdvance(WBGain3, counter, 2);
		if (printHeader){ SHOWNAME(WBGain3); for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain3[i]);  SHOWEND; }
		readAndAdvance(WBGain4, counter, 2);
		if (printHeader){ SHOWNAME(WBGain4); for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain4[i]);  SHOWEND; }
		readAndAdvance(Rotate, counter);
		if (printHeader) SHOW(Rotate);
		readAndAdvance(WBView, counter, 2);
		if (printHeader){ SHOWNAME(WBView); for (int i = 0; i < 2; i++) SHOWVALONLY(WBView[i]);  SHOWEND; }
		readAndAdvance(RealBPP, counter);
		if (printHeader) SHOW(RealBPP);
		readAndAdvance(Conv8Min, counter);
		if (printHeader) SHOW(Conv8Min);
		readAndAdvance(Conv8Max, counter);
		if (printHeader) SHOW(Conv8Max);
		readAndAdvance(FilterCode, counter);
		if (printHeader) SHOW(FilterCode);
		readAndAdvance(FilterParam, counter);
		if (printHeader) SHOW(FilterParam);
		readAndAdvance(UF_Dim, counter);
		if (printHeader) SHOW(UF_Dim);
		readAndAdvance(UF_Shifts, counter);
		if (printHeader) SHOW(UF_Shifts);
		readAndAdvance(UF_Bias, counter);
		if (printHeader) SHOW(UF_Bias);
		readAndAdvance(UF_Coeff, counter, 25);
		if (printHeader){ SHOWNAME(UF_Coeff); for (int i = 0; i < 25; i++) SHOWVALONLY(UF_Coeff[i]);  SHOWEND; }
		readAndAdvance(BlackCalSVer, counter);
		if (printHeader) SHOW(BlackCalSVer);
		readAndAdvance(WhiteCalSVer, counter);
		if (printHeader) SHOW(WhiteCalSVer);
		readAndAdvance(GrayCalSVer, counter);
		if (printHeader) SHOW(GrayCalSVer);
		readAndAdvance(bStampTime, counter);
		if (printHeader) SHOW(bStampTime);
		readAndAdvance(SoundDest, counter);
		if (printHeader) SHOW(SoundDest);
		readAndAdvance(FRPSteps, counter);
		if (printHeader) SHOW(FRPSteps);
		readAndAdvance(FRPImgNr, counter, 16);
		if (printHeader){ SHOWNAME(FRPImgNr); for (int i = 0; i < 16; i++) SHOWVALONLY(FRPImgNr[i]); SHOWEND; }
		readAndAdvance(FRPRate, counter, 16);
		if (printHeader){ SHOWNAME(FRPRate); for (int i = 0; i < 16; i++) SHOWVALONLY(FRPRate[i]);  SHOWEND; }
		readAndAdvance(FRPExp, counter, 16);
		if (printHeader){ SHOWNAME(FRPExp); for (int i = 0; i < 16; i++) SHOWVALONLY(FRPExp[i]);  SHOWEND; }
		readAndAdvance(MCCnt, counter);
		if (printHeader) SHOW(MCCnt);
		readAndAdvance(MCPercent, counter, 64);
		if (printHeader){ SHOWNAME(MCPercent); for (int i = 0; i < 64; i++) SHOWVALONLY(MCPercent[i]);  SHOWEND; }
		readAndAdvance(CICalib, counter);
		if (printHeader) SHOW(CICalib);
		readAndAdvance(CalibWidth, counter);
		if (printHeader) SHOW(CalibWidth);
		readAndAdvance(CalibHeight, counter);
		if (printHeader) SHOW(CalibHeight);
		readAndAdvance(CalibRate, counter);
		if (printHeader) SHOW(CalibRate);
		readAndAdvance(CalibExp, counter);
		if (printHeader) SHOW(CalibExp);
		readAndAdvance(CalibEDR, counter);
		if (printHeader) SHOW(CalibEDR);
		readAndAdvance(CalibTemp, counter);
		if (printHeader) SHOW(CalibTemp);
		readAndAdvance(HeadSerial, counter, 4);
		if (printHeader){ SHOWNAME(HeadSerial); for (int i = 0; i < 4; i++) SHOWVALONLY(HeadSerial[i]);  SHOWEND; }
		readAndAdvance(RangeCode, counter);
		if (printHeader) SHOW(RangeCode);
		readAndAdvance(RangeSize, counter);
		if (printHeader) SHOW(RangeSize);
		readAndAdvance(Decimation, counter);
		if (printHeader) SHOW(Decimation);
		readAndAdvance(MasterSerial, counter);
		if (printHeader) SHOW(MasterSerial);
		readAndAdvance(Sensor, counter);
		if (printHeader) SHOW(Sensor);
		readAndAdvance(ShutterNs, counter);
		if (printHeader) SHOW(ShutterNs);
		readAndAdvance(EDRShutterNs, counter);
		if (printHeader) SHOW(EDRShutterNs);
		readAndAdvance(FrameDelayNs, counter);
		if (printHeader) SHOW(FrameDelayNs);
		readAndAdvance(ImPosXAcq, counter);
		if (printHeader) SHOW(ImPosXAcq);
		readAndAdvance(ImPosYAcq, counter);
		if (printHeader) SHOW(ImPosYAcq);
		readAndAdvance(ImWidthAcq, counter);
		if (printHeader) SHOW(ImWidthAcq);
		readAndAdvance(ImHeightAcq, counter);
		if (printHeader) SHOW(ImHeightAcq);
		readAndAdvance(&Description[0], counter, 4096);
		if (printHeader) SHOWSTRING(Description);
		delete[] camerasetup;

		//IMAGE POSITIONS
		is.seekg(OffImageOffsets);
		int addressSize = (Version == 0) ? 4 : 8;
		char * imageAddressarray = new char[addressSize * ImageCount];
		is.read(imageAddressarray, addressSize * ImageCount);
		counter = imageAddressarray;

		image_addresses.clear();
		for (int i = 0; i < ImageCount; i++){
			if (Version == 0)
			{
				unsigned long var;
				readAndAdvance(var, counter);
				image_addresses.push_back(var);
			}
			else
			{
				unsigned long long var;
				readAndAdvance(var, counter);
				image_addresses.push_back(var);
			}
		}

		delete[] imageAddressarray;

		nbImages = ImageCount;
		fps = FrameRate; 

		if (ImageCount > 0){
			is.seekg(image_addresses[0]);
			DWORD annotationSize;
			is.read((char*)&annotationSize, sizeof(DWORD));

			is.seekg(image_addresses[0] + annotationSize);
			char * imageData = new char[biSizeImage];
			is.read(imageData, biSizeImage);
			cv::Mat imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData).clone();
			delete[]imageData;
			cv::flip(imageWithData, imageWithData, 0);
			image->setImage(imageWithData);
			imageWithData.release();
		}

		is.close();

		if (printHeader) std::cout << "Read Cine File " << filenames.at(0).toAscii().data() << " Finished" << std::endl << std::endl;
	}
}

CineVideo::~CineVideo(){

}

void CineVideo::setActiveFrame(int _activeFrame)
{
	std::ifstream is(filenames.at(0).toAscii().data(), std::ifstream::binary);
	if (_activeFrame < ImageCount && _activeFrame < image_addresses.size()){
		is.seekg(image_addresses[_activeFrame]);
		DWORD annotationSize;
		is.read((char*)&annotationSize, sizeof(DWORD));

		is.seekg(image_addresses[_activeFrame] + annotationSize);
		char * imageData = new char[biSizeImage];
		is.read(imageData, biSizeImage);
		cv::Mat imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData).clone();
		delete[]imageData;
		cv::flip(imageWithData, imageWithData, 0);
		image->setImage(imageWithData,false);
		imageWithData.release();
	}
	is.close();
}

QString CineVideo::getFrameName(int frameNumber)
{
	QFileInfo info(filenames.at(0));
	return info.fileName() + " Frame " + QString::number(frameNumber + 1);
}

void CineVideo::reloadFile()
{
	loadCineInfo();
}
