//  ----------------------------------
//  XMALab -- Copyright   2015, Brown University, Providence, RI.
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
//  PROVIDED  AS IS , INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file MarkerTracking.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerTracking.h" 

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#ifdef HAVE_OPENCV_CUDALEGACY
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudalegacy.hpp>
#endif

//#define WRITEIMAGES 0

using namespace xma;

int MarkerTracking::nbInstances = 0;

// Helper: Check for CUDA availability (cached)
static bool isCudaAvailable()
{
#ifdef HAVE_OPENCV_CUDALEGACY
    static int cudaChecked = -1;
    if (cudaChecked < 0) {
        try {
            cudaChecked = (cv::cuda::getCudaEnabledDeviceCount() > 0) ? 1 : 0;
        }
        catch (...) {
            cudaChecked = 0;
        }
    }
    return cudaChecked == 1;
#else
    return false;
#endif
}

MarkerTracking::MarkerTracking(int camera, int trial, int frame_from, int frame_to, int marker, bool forward) : QObject(),
m_camera(camera), m_trial(trial), m_frame_from(frame_from), m_frame_to(frame_to), m_marker(marker), m_forward(forward)
{
    nbInstances++;
    x_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].x;
    y_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].y;
    searchArea = 30;
    size = (int)(Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() + 0.5);
    size = (size < 5) ? 5 : size;

    Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(templ, size + 3, x_from, y_from);
    maxPenalty = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMaxPenalty();
#ifdef WRITEIMAGES
    cv::imwrite("Tra_Template.png", templ);
    fprintf(stderr, "Start Track Marker : Camera %d Pos %lf %lf Size %d\n", camera, x_from, y_from, size);
#endif
}

MarkerTracking::~MarkerTracking()
{
}

void MarkerTracking::trackMarker()
{
    m_FutureWatcher = new QFutureWatcher<void>();
    connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(trackMarker_threadFinished()));

    QFuture<void> future = QtConcurrent::run(&MarkerTracking::trackMarker_thread, this);
    m_FutureWatcher->setFuture(future);
}

void MarkerTracking::trackMarker_thread()
{
    cv::Mat ROI_to;
    int used_size = size + searchArea + 3;

    int prediction = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMarkerPrediction(m_camera, m_frame_to, x_to, y_to, m_forward);

    if (prediction <= 1) maxPenalty /= 3;

#ifdef WRITEIMAGES
    fprintf(stderr, "Prediction Track Marker : Camera %d Pos %lf %lf\n", m_camera, x_to, y_to);
#endif
    int off_x = (int)(x_to - used_size + 0.5);
    int off_y = (int)(y_to - used_size + 0.5);

    Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(ROI_to, used_size, off_x, off_y);

#ifdef WRITEIMAGES
    cv::imwrite("Tra_Target.png", ROI_to);
#endif
    /// Create the result matrix
    int result_cols = ROI_to.cols - templ.cols + 1;
    int result_rows = ROI_to.rows - templ.rows + 1;

    cv::Mat result;
    result.create(result_rows, result_cols, CV_32FC1);

    // --- CUDA path ---
    if (isCudaAvailable()) {
#ifdef HAVE_OPENCV_CUDALEGACY
        try {
            cv::cuda::GpuMat d_ROI_to, d_templ, d_result;
            d_ROI_to.upload(ROI_to);
            d_templ.upload(templ);
            cv::cuda::matchTemplate(d_ROI_to, d_templ, d_result, cv::TM_CCORR_NORMED);
            d_result.download(result);
        }
        catch (const cv::Exception& e) {
            // On any CUDA error fallback to CPU implementation
            cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
        }
#else
        // If headers not available, fall back to CPU
        cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
#endif
    }
    else {
        // --- CPU path ---
        cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
    }

    normalize(result, result, 0, (100 - maxPenalty), cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
    cv::imwrite("Tra_Result.png", result);
#endif

    cv::Mat springforce;
    springforce.create(result_cols, result_rows, CV_32FC1);
    double halfcol = 0.5 * result_cols;
    double halfrow = 0.5 * result_rows;
    double sigma = halfcol * 3;
    double inv_2sigma_sq = 1.0 / (2.0 * sigma * sigma);

    // Use direct pointer access for better performance
    float* springforce_ptr = springforce.ptr<float>();
    for (int i = 0; i < result_rows; i++)
    {
        double di = halfrow - i;
        double di_sq = di * di;
        for (int j = 0; j < result_cols; j++)
        {
            double dj = halfcol - j;
            double val = exp((di_sq + dj * dj) * inv_2sigma_sq);
            springforce_ptr[i * result_cols + j] = static_cast<float>(val);
        }
    }
    normalize(springforce, springforce, 0, maxPenalty, cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
    cv::imwrite("Tra_Penalty.png", springforce);
#endif

    result = result - springforce;

#ifdef WRITEIMAGES
    cv::imwrite("Tra_PenResult.png", result);
#endif

    /// Localizing the best match with minMaxLoc
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::Point matchLoc;

    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

    matchLoc = maxLoc;

    x_to = matchLoc.x + off_x + size + 3;
    y_to = matchLoc.y + off_y + size + 3;

#ifdef WRITEIMAGES
    fprintf(stderr, "Tracked %lf %lf\n", x_to, y_to);
    fprintf(stderr, "Val %lf\n", maxVal);
    fprintf(stderr, "Tracked (local) %d %d\n", matchLoc.x, matchLoc.y);
    fprintf(stderr, "Stop Track Marker : Camera %d Pos %lf %lf\n", m_camera, x_to, y_to);
#endif


    ROI_to.release();
    result.release();
}

void MarkerTracking::trackMarker_threadFinished()
{
    Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame_to, x_to, y_to, TRACKED);
    delete m_FutureWatcher;
    nbInstances--;
    if (nbInstances == 0)
    {
        emit trackMarker_finished();
    }
    delete this;
}

