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
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>
#include <map>
#include <mutex>

//#define WRITEIMAGES 0

using namespace xma;

namespace
{
    void ensureOpenClInitialized()
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            if (cv::ocl::haveOpenCL())
            {
                cv::ocl::setUseOpenCL(true);
            }
            else
            {
                cv::ocl::setUseOpenCL(false);
            }
        });
    }

    struct PenaltyCacheEntry
    {
        cv::Mat mat;
        cv::UMat umat;
    };

    const PenaltyCacheEntry& getNormalizedPenaltySurface(int rows, int cols)
    {
        static std::mutex cacheMutex;
        static std::map<std::pair<int, int>, PenaltyCacheEntry> cache;

        std::lock_guard<std::mutex> lock(cacheMutex);
        const auto key = std::make_pair(rows, cols);
        auto it = cache.find(key);
        if (it != cache.end())
        {
            return it->second;
        }

        auto [insertIt, inserted] = cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple());

        PenaltyCacheEntry& entry = insertIt->second;
        entry.mat.create(rows, cols, CV_32FC1);

        double halfcol = 0.5 * cols;
        double halfrow = 0.5 * rows;
        double sigma = halfcol * 3;
        double inv_2sigma_sq = 1.0 / (2.0 * sigma * sigma);

        float* data = entry.mat.ptr<float>();
        for (int i = 0; i < rows; ++i)
        {
            double di = halfrow - i;
            double di_sq = di * di;
            for (int j = 0; j < cols; ++j)
            {
                double dj = halfcol - j;
                double val = exp((di_sq + dj * dj) * inv_2sigma_sq);
                data[i * cols + j] = static_cast<float>(val);
            }
        }

        cv::normalize(entry.mat, entry.mat, 0.0f, 1.0f, cv::NORM_MINMAX);
        entry.mat.copyTo(entry.umat);

        return entry;
    }
}

int MarkerTracking::nbInstances = 0;

MarkerTracking::MarkerTracking(int camera, int trial, int frame_from, int frame_to, int marker, bool forward) : QObject(),
m_camera(camera), m_trial(trial), m_frame_from(frame_from), m_frame_to(frame_to), m_marker(marker), m_forward(forward)
{
    nbInstances++;
    ensureOpenClInitialized();
    x_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].x;
    y_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].y;
    searchArea = 30;
    size = (int)(Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() + 0.5);
    size = (size < 5) ? 5 : size;

    Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(templ, size + 3, x_from, y_from);
    templ_umat = templ.getUMat(cv::ACCESS_READ);
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
    ensureOpenClInitialized();
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

    if (result_cols <= 0 || result_rows <= 0)
    {
        ROI_to.release();
        return;
    }

    const auto& penaltyEntry = getNormalizedPenaltySurface(result_rows, result_cols);

    if (cv::ocl::useOpenCL() && !templ_umat.empty())
    {
        roi_buffer = ROI_to.getUMat(cv::ACCESS_READ);
        result_buffer.create(result_rows, result_cols, CV_32FC1);
        springforce_buffer.create(result_rows, result_cols, CV_32FC1);

        cv::matchTemplate(roi_buffer, templ_umat, result_buffer, cv::TM_CCORR_NORMED);
        cv::normalize(result_buffer, result_buffer, 0, (100 - maxPenalty), cv::NORM_MINMAX);

#ifdef WRITEIMAGES
        cv::Mat dbgResult = result_buffer.getMat(cv::ACCESS_READ);
        cv::imwrite("Tra_Result.png", dbgResult);
#endif

        cv::multiply(penaltyEntry.umat, cv::Scalar(static_cast<float>(maxPenalty)), springforce_buffer);

#ifdef WRITEIMAGES
        cv::Mat dbgPenalty = springforce_buffer.getMat(cv::ACCESS_READ);
        cv::imwrite("Tra_Penalty.png", dbgPenalty);
#endif

        cv::subtract(result_buffer, springforce_buffer, result_buffer);

#ifdef WRITEIMAGES
        cv::Mat dbgPenResult = result_buffer.getMat(cv::ACCESS_READ);
        cv::imwrite("Tra_PenResult.png", dbgPenResult);
#endif

    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::minMaxLoc(result_buffer, &minVal, &maxVal, &minLoc, &maxLoc);

        cv::Point matchLoc = maxLoc;
        x_to = matchLoc.x + off_x + size + 3;
        y_to = matchLoc.y + off_y + size + 3;

#ifdef WRITEIMAGES
        fprintf(stderr, "Tracked %lf %lf\n", x_to, y_to);
        fprintf(stderr, "Val %lf\n", maxVal);
        fprintf(stderr, "Tracked (local) %d %d\n", matchLoc.x, matchLoc.y);
        fprintf(stderr, "Stop Track Marker : Camera %d Pos %lf %lf\n", m_camera, x_to, y_to);
#endif
    }
    else
    {
        cv::Mat result;
        result.create(result_rows, result_cols, CV_32FC1);

        cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
    normalize(result, result, 0, (100 - maxPenalty), cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
        cv::imwrite("Tra_Result.png", result);
#endif

    cv::Mat springforce;
    cv::multiply(penaltyEntry.mat, cv::Scalar(static_cast<float>(maxPenalty)), springforce);

#ifdef WRITEIMAGES
        cv::imwrite("Tra_Penalty.png", springforce);
#endif

        result = result - springforce;

#ifdef WRITEIMAGES
        cv::imwrite("Tra_PenResult.png", result);
#endif

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

        result.release();
        springforce.release();
    }

    ROI_to.release();
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

