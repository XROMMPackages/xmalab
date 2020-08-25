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
///\file CineVideo.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CineVideo.h"

#include <fstream> 

#include <QtCore/QFileInfo>

#include <opencv2/opencv.hpp>
#include "Project.h"

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

static unsigned char masks[4][2] = {
	{ 0xFF, 0xC0 },
	{ 0x3F, 0xF0 },
	{ 0x0F, 0xFC },
	{ 0x03, 0xFF }
};

static int LinLUT[1024] =
{
	2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 17, 18,
	19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
	94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	110, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
	125, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154,
	156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 167, 168, 169, 170, 171, 172,
	173, 175, 176, 177, 178, 179, 181, 182, 183, 184, 186, 187, 188, 189, 191, 192,
	193, 194, 196, 197, 198, 200, 201, 202, 204, 205, 206, 208, 209, 210, 212, 213,
	215, 216, 217, 219, 220, 222, 223, 225, 226, 227, 229, 230, 232, 233, 235, 236,
	238, 239, 241, 242, 244, 245, 247, 249, 250, 252, 253, 255, 257, 258, 260, 261,
	263, 265, 266, 268, 270, 271, 273, 275, 276, 278, 280, 281, 283, 285, 287, 288,
	290, 292, 294, 295, 297, 299, 301, 302, 304, 306, 308, 310, 312, 313, 315, 317,
	319, 321, 323, 325, 327, 328, 330, 332, 334, 336, 338, 340, 342, 344, 346, 348,
	350, 352, 354, 356, 358, 360, 362, 364, 366, 368, 370, 372, 374, 377, 379, 381,
	383, 385, 387, 389, 391, 394, 396, 398, 400, 402, 404, 407, 409, 411, 413, 416,
	418, 420, 422, 425, 427, 429, 431, 434, 436, 438, 441, 443, 445, 448, 450, 452,
	455, 457, 459, 462, 464, 467, 469, 472, 474, 476, 479, 481, 484, 486, 489, 491,
	494, 496, 499, 501, 504, 506, 509, 511, 514, 517, 519, 522, 524, 527, 529, 532,
	535, 537, 540, 543, 545, 548, 551, 553, 556, 559, 561, 564, 567, 570, 572, 575,
	578, 581, 583, 586, 589, 592, 594, 597, 600, 603, 606, 609, 611, 614, 617, 620,
	623, 626, 629, 632, 635, 637, 640, 643, 646, 649, 652, 655, 658, 661, 664, 667,
	670, 673, 676, 679, 682, 685, 688, 691, 694, 698, 701, 704, 707, 710, 713, 716,
	719, 722, 726, 729, 732, 735, 738, 742, 745, 748, 751, 754, 758, 761, 764, 767,
	771, 774, 777, 781, 784, 787, 790, 794, 797, 800, 804, 807, 811, 814, 817, 821,
	824, 828, 831, 834, 838, 841, 845, 848, 852, 855, 859, 862, 866, 869, 873, 876,
	880, 883, 887, 890, 894, 898, 901, 905, 908, 912, 916, 919, 923, 927, 930, 934,
	938, 941, 945, 949, 952, 956, 960, 964, 967, 971, 975, 979, 982, 986, 990, 994,
	998, 1001, 1005, 1009, 1013, 1017, 1021, 1025, 1028, 1032, 1036, 1040, 1044, 1048, 1052, 1056,
	1060, 1064, 1068, 1072, 1076, 1080, 1084, 1088, 1092, 1096, 1100, 1104, 1108, 1112, 1116, 1120,
	1124, 1128, 1132, 1137, 1141, 1145, 1149, 1153, 1157, 1162, 1166, 1170, 1174, 1178, 1183, 1187,
	1191, 1195, 1200, 1204, 1208, 1212, 1217, 1221, 1225, 1230, 1234, 1238, 1243, 1247, 1251, 1256,
	1260, 1264, 1269, 1273, 1278, 1282, 1287, 1291, 1295, 1300, 1304, 1309, 1313, 1318, 1322, 1327,
	1331, 1336, 1340, 1345, 1350, 1354, 1359, 1363, 1368, 1372, 1377, 1382, 1386, 1391, 1396, 1400,
	1405, 1410, 1414, 1419, 1424, 1428, 1433, 1438, 1443, 1447, 1452, 1457, 1462, 1466, 1471, 1476,
	1481, 1486, 1490, 1495, 1500, 1505, 1510, 1515, 1520, 1524, 1529, 1534, 1539, 1544, 1549, 1554,
	1559, 1564, 1569, 1574, 1579, 1584, 1589, 1594, 1599, 1604, 1609, 1614, 1619, 1624, 1629, 1634,
	1639, 1644, 1649, 1655, 1660, 1665, 1670, 1675, 1680, 1685, 1691, 1696, 1701, 1706, 1711, 1717,
	1722, 1727, 1732, 1738, 1743, 1748, 1753, 1759, 1764, 1769, 1775, 1780, 1785, 1791, 1796, 1801,
	1807, 1812, 1818, 1823, 1828, 1834, 1839, 1845, 1850, 1856, 1861, 1867, 1872, 1878, 1883, 1889,
	1894, 1900, 1905, 1911, 1916, 1922, 1927, 1933, 1939, 1944, 1950, 1956, 1961, 1967, 1972, 1978,
	1984, 1989, 1995, 2001, 2007, 2012, 2018, 2024, 2030, 2035, 2041, 2047, 2053, 2058, 2064, 2070,
	2076, 2082, 2087, 2093, 2099, 2105, 2111, 2117, 2123, 2129, 2135, 2140, 2146, 2152, 2158, 2164,
	2170, 2176, 2182, 2188, 2194, 2200, 2206, 2212, 2218, 2224, 2231, 2237, 2243, 2249, 2255, 2261,
	2267, 2273, 2279, 2286, 2292, 2298, 2304, 2310, 2317, 2323, 2329, 2335, 2341, 2348, 2354, 2360,
	2366, 2373, 2379, 2385, 2392, 2398, 2404, 2411, 2417, 2423, 2430, 2436, 2443, 2449, 2455, 2462,
	2468, 2475, 2481, 2488, 2494, 2501, 2507, 2514, 2520, 2527, 2533, 2540, 2546, 2553, 2559, 2566,
	2572, 2579, 2586, 2592, 2599, 2605, 2612, 2619, 2625, 2632, 2639, 2645, 2652, 2659, 2666, 2672,
	2679, 2686, 2693, 2699, 2706, 2713, 2720, 2726, 2733, 2740, 2747, 2754, 2761, 2767, 2774, 2781,
	2788, 2795, 2802, 2809, 2816, 2823, 2830, 2837, 2844, 2850, 2857, 2864, 2871, 2878, 2885, 2893,
	2900, 2907, 2914, 2921, 2928, 2935, 2942, 2949, 2956, 2963, 2970, 2978, 2985, 2992, 2999, 3006,
	3013, 3021, 3028, 3035, 3042, 3049, 3057, 3064, 3071, 3078, 3086, 3093, 3100, 3108, 3115, 3122,
	3130, 3137, 3144, 3152, 3159, 3166, 3174, 3181, 3189, 3196, 3204, 3211, 3218, 3226, 3233, 3241,
	3248, 3256, 3263, 3271, 3278, 3286, 3294, 3301, 3309, 3316, 3324, 3331, 3339, 3347, 3354, 3362,
	3370, 3377, 3385, 3393, 3400, 3408, 3416, 3423, 3431, 3439, 3447, 3454, 3462, 3470, 3478, 3486,
	3493, 3501, 3509, 3517, 3525, 3533, 3540, 3548, 3556, 3564, 3572, 3580, 3588, 3596, 3604, 3612,
	3620, 3628, 3636, 3644, 3652, 3660, 3668, 3676, 3684, 3692, 3700, 3708, 3716, 3724, 3732, 3740,
	3749, 3757, 3765, 3773, 3781, 3789, 3798, 3806, 3814, 3822, 3830, 3839, 3847, 3855, 3863, 3872,
	3880, 3888, 3897, 3905, 3913, 3922, 3930, 3938, 3947, 3955, 3963, 3972, 3980, 3989, 3997, 4006,
	4014, 4022, 4031, 4039, 4048, 4056, 4064, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
};


template <typename T>
void readAndAdvance(T& output, char* & ptr)
{
	T* Tptr = (T *) ptr;
	ptr += sizeof(T);
	output = *Tptr;
}

template <typename T>
void readAndAdvance(T* output, char* & ptr, int length)
{
	for (int i = 0; i < length; i++)
	{
		T* Tptr = (T *) ptr;
		ptr += sizeof(T);
		output[i] = *Tptr;
	}
}

CineVideo::CineVideo(QStringList _filenames) : VideoStream(_filenames)
{
	lastFrame = -1;
	loadCineInfo();
}

void CineVideo::unpackImageData(char* packed, unsigned char* unpacked)
{
	unsigned short tmp;
	unsigned char * upacked = (unsigned char *) packed;
	int size = biWidth * biHeight;

	for (int i = 0; i < size; i += 4, upacked += 5)
	{
		for (int k = 0; k < 4; k++){
			tmp = (((unsigned short)(upacked[k] & masks[k][0])) << 8) | (upacked[k + 1] & masks[k][1]);
			//shift for correct 10bit
			tmp = tmp >> (2 * (3 - k));
			if (RecBPP == 12)
			{
				//lookup 12 bit and convert to 8 bit
				*unpacked++ = LinLUT[tmp] >> 4;
			}
			else{
				//convert 10 to 8 bit
				*unpacked++ = tmp >> 2;
			}
		}
	}
}

void CineVideo::loadCineInfo()
{
	//Open File
	if (QFile::exists(filenames.at(0)))
	{
		std::ifstream is(filenames.at(0).toStdString(), std::ifstream::binary);

		char* counter;
		if (printHeader)std::cout << "Read Cine File " << filenames.at(0).toStdString() << std::endl;
		//Read CINEFILEHEADER
		is.seekg(0);
		char* header = new char[44];
		counter = header;
		is.read(header, 44);

		readAndAdvance(&Type[0], counter, 2);
		//if (printHeader) SHOWSTRING(Type);
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
		char* bitmapInfoHeader = new char[lengthBitmapInfoHeader];
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
		char* camerasetup = new char[5740];
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
		//if (printHeader) SHOWSTRING(Mark);
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
		if (printHeader)
		{
			SHOWNAME(BinName) ;
			for (int i = 0; i < 8; i++)SHOWSTRINGONLY(BinName[i]);
			SHOWEND;
		}
		readAndAdvance(AnaOption, counter);
		if (printHeader) SHOW(AnaOption);
		readAndAdvance(AnaChannels, counter);
		if (printHeader) SHOW(AnaChannels);
		readAndAdvance(Res6, counter);
		if (printHeader) SHOW(Res6);
		readAndAdvance(AnaBoard, counter);
		if (printHeader) SHOW(AnaBoard);
		readAndAdvance(ChOption, counter, 8);
		if (printHeader)
		{
			SHOWNAME(ChOption) ;
			for (int i = 0; i < 8; i++) SHOWVALONLY(ChOption[i]);
			SHOWEND;
		}
		readAndAdvance(AnaGain, counter, 8);
		if (printHeader)
		{
			SHOWNAME(AnaGain) ;
			for (int i = 0; i < 8; i++)SHOWVALONLY(AnaGain[i]);
			SHOWEND;
		}
		for (int i = 0; i < 8; i++)readAndAdvance(&AnaUnit[i][0], counter, 6);
		if (printHeader)
		{
			SHOWNAME(AnaUnit) ;
			for (int i = 0; i < 8; i++)SHOWSTRINGONLY(AnaUnit[i]);
			SHOWEND;
		}
		for (int i = 0; i < 8; i++)readAndAdvance(&AnaName[i][0], counter, 11);
		if (printHeader)
		{
			SHOWNAME(AnaName) ;
			for (int i = 0; i < 8; i++)SHOWSTRINGONLY(AnaName[i]);
			SHOWEND;
		}
		readAndAdvance(lFirstImage, counter);
		if (printHeader) SHOW(lFirstImage);
		readAndAdvance(dwImageCount, counter);
		if (printHeader) SHOW(dwImageCount);
		readAndAdvance(nQFactor, counter);
		if (printHeader) SHOW(nQFactor);
		readAndAdvance(wCineFileType, counter);
		if (printHeader) SHOW(wCineFileType);
		for (int i = 0; i < 4; i++)readAndAdvance(&szCinePath[i][0], counter, 65);
		if (printHeader)
		{
			SHOWNAME(szCinePath) ;
			for (int i = 0; i < 4; i++)SHOWSTRINGONLY(szCinePath[i]);
			SHOWEND;
		}
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
		if (printHeader)
		{
			SHOWNAME(AutoExpRect) ;
			for (int i = 0; i < 4; i++) SHOWVALONLY(AutoExpRect[i]);
			SHOWEND;
		}
		readAndAdvance(WBGain1, counter, 2);
		if (printHeader)
		{
			SHOWNAME(WBGain1) ;
			for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain1[i]);
			SHOWEND;
		}
		readAndAdvance(WBGain2, counter, 2);
		if (printHeader)
		{
			SHOWNAME(WBGain2) ;
			for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain2[i]);
			SHOWEND;
		}
		readAndAdvance(WBGain3, counter, 2);
		if (printHeader)
		{
			SHOWNAME(WBGain3) ;
			for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain3[i]);
			SHOWEND;
		}
		readAndAdvance(WBGain4, counter, 2);
		if (printHeader)
		{
			SHOWNAME(WBGain4) ;
			for (int i = 0; i < 2; i++) SHOWVALONLY(WBGain4[i]);
			SHOWEND;
		}
		readAndAdvance(Rotate, counter);
		if (printHeader) SHOW(Rotate);
		readAndAdvance(WBView, counter, 2);
		if (printHeader)
		{
			SHOWNAME(WBView) ;
			for (int i = 0; i < 2; i++) SHOWVALONLY(WBView[i]);
			SHOWEND;
		}
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
		if (printHeader)
		{
			SHOWNAME(UF_Coeff) ;
			for (int i = 0; i < 25; i++) SHOWVALONLY(UF_Coeff[i]);
			SHOWEND;
		}
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
		if (printHeader)
		{
			SHOWNAME(FRPImgNr) ;
			for (int i = 0; i < 16; i++) SHOWVALONLY(FRPImgNr[i]);
			SHOWEND;
		}
		readAndAdvance(FRPRate, counter, 16);
		if (printHeader)
		{
			SHOWNAME(FRPRate) ;
			for (int i = 0; i < 16; i++) SHOWVALONLY(FRPRate[i]);
			SHOWEND;
		}
		readAndAdvance(FRPExp, counter, 16);
		if (printHeader)
		{
			SHOWNAME(FRPExp) ;
			for (int i = 0; i < 16; i++) SHOWVALONLY(FRPExp[i]);
			SHOWEND;
		}
		readAndAdvance(MCCnt, counter);
		if (printHeader) SHOW(MCCnt);
		readAndAdvance(MCPercent, counter, 64);
		if (printHeader)
		{
			SHOWNAME(MCPercent) ;
			for (int i = 0; i < 64; i++) SHOWVALONLY(MCPercent[i]);
			SHOWEND;
		}
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
		if (printHeader)
		{
			SHOWNAME(HeadSerial) ;
			for (int i = 0; i < 4; i++) SHOWVALONLY(HeadSerial[i]);
			SHOWEND;
		}
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

		if (SoftwareVersion > 720){
			is.seekg(OffSetup + 10112);
			char* recBPP_tmp = new char[sizeof(UINT)];
			UINT *  recBPP_u = reinterpret_cast<UINT*>(recBPP_tmp);
			is.read(recBPP_tmp, sizeof(UINT));
			RecBPP = *recBPP_u;
			if (printHeader) SHOW(RecBPP);
			delete[] recBPP_tmp;
		}

		//IMAGE POSITIONS
		is.seekg(OffImageOffsets);
		int addressSize = (Version == 0) ? 4 : 8;
		char* imageAddressarray = new char[addressSize * ImageCount];
		is.read(imageAddressarray, addressSize * ImageCount);
		counter = imageAddressarray;

		image_addresses.clear();
		for (unsigned int i = 0; i < ImageCount; i++)
		{
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

		if (ImageCount > 0)
		{
			is.seekg(image_addresses[0]);
			DWORD annotationSize;
			is.read((char*)&annotationSize, sizeof(DWORD));

			is.seekg(image_addresses[0] + annotationSize);
			cv::Mat imageWithData;
			char* imageData = new char[biSizeImage];
			is.read(imageData, biSizeImage);

			if (biCompression == 256)
			{
				unsigned char* imageData_unpacked = new unsigned char[biWidth * biHeight];
				unpackImageData(imageData, imageData_unpacked);
				imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData_unpacked).clone();

				delete[] imageData_unpacked;
			}
			else{
				if (biBitCount == 16)
				{
					double mod = 1.0f / pow(2, (RealBPP - 8));
					cv::Mat(biHeight, biWidth, CV_16U, imageData).convertTo(imageWithData, CV_8U, mod);
				}
				else
				{
					imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData).clone();
				}
			}

			delete[]imageData;
			cv::flip(imageWithData, imageWithData, 0);
			if (isFlipped)
				cv::flip(imageWithData, imageWithData, 1);

			image->setImage(imageWithData);
			imageWithData.release();
		}

		is.close();

		if (printHeader) std::cout << "Read Cine File " << filenames.at(0).toStdString() << " Finished" << std::endl << std::endl;
	}
}

CineVideo::~CineVideo()
{
}



void CineVideo::setActiveFrame(int _activeFrame)
{
	if (lastFrame == _activeFrame)
		return;

	lastFrame = _activeFrame;

	std::ifstream is(filenames.at(0).toStdString(), std::ifstream::binary);
	if (_activeFrame < (int) ImageCount && _activeFrame < (int) image_addresses.size())
	{
		is.seekg(image_addresses[_activeFrame]);
		DWORD annotationSize;
		is.read((char*)&annotationSize, sizeof(DWORD));
		is.seekg(image_addresses[_activeFrame] + annotationSize);
		cv::Mat imageWithData;
		char* imageData = new char[biSizeImage];
		is.read(imageData, biSizeImage);
		if (biCompression == 256)
		{
			unsigned char* imageData_unpacked = new unsigned char[biWidth * biHeight];
			unpackImageData(imageData, imageData_unpacked);
			imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData_unpacked).clone();

			delete[] imageData_unpacked;
		}
		else{
			if (biBitCount == 16)
			{
				double mod = 1.0f / pow(2, (RealBPP - 8));
				cv::Mat(biHeight, biWidth, CV_16U, imageData).convertTo(imageWithData, CV_8U, mod);
			}
			else
			{
				imageWithData = cv::Mat(biHeight, biWidth, CV_8U, imageData).clone();
			}
		}
		delete[]imageData;

		cv::flip(imageWithData, imageWithData, 0);
		if (isFlipped)
			cv::flip(imageWithData, imageWithData, 1);
		image->setImage(imageWithData, false);
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

