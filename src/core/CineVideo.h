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
///\file CineVideo.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CINEVIDEO_H_
#define CINEVIDEO_H_

#include <QString>
#include <core/VideoStream.h>

#define MAXLENDESCRIPTION_OLD 121

namespace xma
{
	typedef unsigned char BYTE;
	typedef char CHAR;
	typedef unsigned short int WORD;
	typedef short int SHORT;
	typedef int BOOL;
	typedef unsigned short int WORD;
	typedef unsigned int DWORD;
	typedef int LONG;
	typedef int INT;
	typedef unsigned int UINT;
	typedef float FLOAT;
	typedef double DOUBLE;
	typedef double WBGAIN;
	typedef char STRING;

	class CineVideo : public VideoStream
	{
	public:
		CineVideo(QStringList _filenames);
		virtual ~CineVideo();

		void setActiveFrame(int _activeFrame) override;
		QString getFrameName(int frameNumber) override;
		void reloadFile() override;
	private:
		//IMAGE POSITIONS
		std::vector<unsigned long long> image_addresses;

		void unpackImageData(char* packed, unsigned char* unpacked);

		void loadCineInfo();

		int lastFrame;

		//CINEFILEHEADER
		CHAR Type[2];
		WORD Headersize;
		WORD Compression;
		WORD Version;
		LONG FirstMovieImage;
		DWORD TotalImageCount;
		LONG FirstImageNo;
		DWORD ImageCount;
		DWORD OffImageHeader;
		DWORD OffSetup;
		DWORD OffImageOffsets;
		DWORD TriggerTimeFractions;
		DWORD TriggerTimeSections;

		//BITMAPINFOHEADER
		DWORD biSize;
		LONG biWidth;
		LONG biHeight;
		WORD biPlanes;
		WORD biBitCount;
		DWORD biCompression;
		DWORD biSizeImage;
		LONG biXPelsPerMeter;
		LONG biYPelsPerMeter;
		DWORD biClrUsed;

		//CAMERA SETUP
		WORD FrameRate16;
		WORD Shutter16;
		WORD PostTrigger16;
		WORD FrameDelay16;
		WORD AspectRatio;
		WORD Contrast16;
		WORD Bright16;
		BYTE Rotate16;
		BYTE TimeAnnotation;
		BYTE TrigCine;
		BYTE TrigFrame;
		BYTE ShutterOn;
		CHAR DescriptionOld[MAXLENDESCRIPTION_OLD];
		CHAR Mark[2];
		WORD Length;
		WORD Binning;
		WORD SigOption;
		SHORT BinChannels;
		BYTE SamplesPerImage;
		STRING BinName[8][11];
		WORD AnaOption;
		SHORT AnaChannels;
		BYTE Res6;
		BYTE AnaBoard;
		SHORT ChOption[8];
		FLOAT AnaGain[8];
		STRING AnaUnit[8][6];
		STRING AnaName[8][11];
		LONG lFirstImage;
		DWORD dwImageCount;
		SHORT nQFactor;
		WORD wCineFileType;
		STRING szCinePath[4][65];
		WORD bMainsFreq;
		BYTE bTimeCode;
		BYTE bPriority;
		WORD wLeapSecDY;
		DOUBLE dDelayTC;
		DOUBLE dDelayPPS;
		WORD GenBits;
		INT Res1;
		INT Res2;
		INT Res3;
		WORD ImWidth;
		WORD ImHeight;
		WORD EDRShutter16;
		UINT Serial;
		INT Saturation;
		BYTE Res5;
		UINT AutoExposure;
		BOOL bFlipH;
		BOOL bFlipV;
		UINT Grid;
		UINT FrameRate;
		UINT Shutter;
		UINT EDRShutter;
		UINT PostTrigger;
		UINT FrameDelay;
		BOOL bEnableColor;
		UINT CameraVersion;
		UINT FirmwareVersion;
		UINT SoftwareVersion;
		INT RecordingTimeZone;
		UINT CFA;
		INT Bright;
		INT Contrast;
		INT Gamma;
		UINT Reserved1;
		UINT AutoExpLevel;
		UINT AutoExpSpeed;
		UINT AutoExpRect[4];
		FLOAT WBGain1[2];
		FLOAT WBGain2[2];
		FLOAT WBGain3[2];
		FLOAT WBGain4[2];
		INT Rotate;
		FLOAT WBView[2];
		UINT RealBPP;
		UINT Conv8Min;
		UINT Conv8Max;
		INT FilterCode;
		INT FilterParam;
		INT UF_Dim;
		INT UF_Shifts;
		INT UF_Bias;
		INT UF_Coeff[25];
		UINT BlackCalSVer;
		UINT WhiteCalSVer;
		UINT GrayCalSVer;
		BOOL bStampTime;
		UINT SoundDest;
		UINT FRPSteps;
		INT FRPImgNr[16];
		UINT FRPRate[16];
		UINT FRPExp[16];
		INT MCCnt;
		FLOAT MCPercent[64];
		UINT CICalib;
		UINT CalibWidth;
		UINT CalibHeight;
		UINT CalibRate;
		UINT CalibExp;
		UINT CalibEDR;
		UINT CalibTemp;
		UINT HeadSerial[4];
		UINT RangeCode;
		UINT RangeSize;
		UINT Decimation;
		UINT MasterSerial;
		UINT Sensor;
		UINT ShutterNs;
		UINT EDRShutterNs;
		UINT FrameDelayNs;
		UINT ImPosXAcq;
		UINT ImPosYAcq;
		UINT ImWidthAcq;
		UINT ImHeightAcq;
		STRING Description[4096];
	};
}

#endif /* CINEVIDEO_H_ */

