#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerTracking3D.h"

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/Camera.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace xma;

int MarkerTracking3D::nbInstances = 0;

MarkerTracking3D::MarkerTracking3D(int trial, int frame_from, int frame_to, int marker, bool forward) : QObject(),
m_trial(trial), m_frame_from(frame_from), m_frame_to(frame_to), m_marker(marker), m_forward(forward)
{
    nbInstances++;
    
    Marker* mkr = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];
    int size = (int)(mkr->getSize() + 0.5);
    size = (size < 5) ? 5 : size;

    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        if (Project::getInstance()->getCameras()[i]->isVisible())
        {
            double x_from = mkr->getPoints2D()[i][m_frame_from].x;
            double y_from = mkr->getPoints2D()[i][m_frame_from].y;
            cv::Mat templ;
            Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage()->getSubImage(templ, size + 3, x_from, y_from);
            m_templates.push_back(templ);
        }
        else
        {
            m_templates.push_back(cv::Mat());
        }
    }
}

MarkerTracking3D::~MarkerTracking3D()
{
}

void MarkerTracking3D::trackMarker()
{
    m_FutureWatcher = new QFutureWatcher<void>();
    connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(trackMarker_threadFinished()));

    QFuture<void> future = QtConcurrent::run(&MarkerTracking3D::trackMarker_thread, this);
    m_FutureWatcher->setFuture(future);
}

void MarkerTracking3D::trackMarker_thread()
{
    Marker* marker = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];
    
    // Predict 3D center using only the previous frame (0th order prediction)
    cv::Point3d pred3D = marker->getPoints3D()[m_frame_from];

    int size = (int)(marker->getSize() + 0.5);
    size = (size < 5) ? 5 : size;
    int searchArea = 30;
    int used_size = size + searchArea + 3;

    std::vector<cv::Mat> result_buffers;
    std::vector<cv::Point2d> offsets;

    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        if (Project::getInstance()->getCameras()[i]->isVisible() && !m_templates[i].empty())
        {
            cv::Mat templ = m_templates[i];

            double x_to, y_to;
            int prediction = marker->getMarkerPrediction(i, m_frame_to, x_to, y_to, m_forward);

            int off_x = (int)(x_to - used_size + 0.5);
            int off_y = (int)(y_to - used_size + 0.5);

            cv::Mat ROI_to;
            Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage()->getSubImage(ROI_to, used_size, off_x, off_y);

            int result_cols = ROI_to.cols - templ.cols + 1;
            int result_rows = ROI_to.rows - templ.rows + 1;

            if (result_cols > 0 && result_rows > 0)
            {
                cv::Mat result;
                result.create(result_rows, result_cols, CV_32FC1);
                cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);

                // Add a small circular penalty
                double halfcol = 0.5 * result_cols;
                double halfrow = 0.5 * result_rows;
                double sigma = halfcol * 3.0;
                double inv_2sigma_sq = 1.0 / (2.0 * sigma * sigma);
                for (int r = 0; r < result_rows; ++r) {
                    float* ptr = result.ptr<float>(r);
                    double dy = halfrow - r;
                    for (int c = 0; c < result_cols; ++c) {
                        double dx = halfcol - c;
                        double val = exp((dx*dx + dy*dy) * inv_2sigma_sq);
                        ptr[c] = ptr[c] - (1.0f - val) * 0.1f; // light penalty
                    }
                }

                result_buffers.push_back(result);
                offsets.push_back(cv::Point2d(off_x + size + 3, off_y + size + 3));
            }
            else
            {
                result_buffers.push_back(cv::Mat());
                offsets.push_back(cv::Point2d(0, 0));
            }
            ROI_to.release();
            templ.release();
        }
        else
        {
            result_buffers.push_back(cv::Mat());
            offsets.push_back(cv::Point2d(0, 0));
        }
    }

    double max_score = -1e9;
    cv::Point3d best_p3d = pred3D;

    // Search volume: +/- 10mm in X, Y, Z with 0.5mm steps
    double search_range = 10.0;
    double step = 0.5;

    for (double dx = -search_range; dx <= search_range; dx += step)
    {
        for (double dy = -search_range; dy <= search_range; dy += step)
        {
            for (double dz = -search_range; dz <= search_range; dz += step)
            {
                cv::Point3d p3d(pred3D.x + dx, pred3D.y + dy, pred3D.z + dz);
                double score = 0;
                int valid_cams = 0;

                for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
                {
                    if (!result_buffers[i].empty())
                    {
                        cv::Point2d proj = Project::getInstance()->getCameras()[i]->projectPoint(p3d, Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage());
                        
                        double u = proj.x - offsets[i].x + result_buffers[i].cols / 2.0;
                        double v = proj.y - offsets[i].y + result_buffers[i].rows / 2.0;

                        if (u >= 0 && u < result_buffers[i].cols - 1 && v >= 0 && v < result_buffers[i].rows - 1)
                        {
                            // Bilinear interpolation
                            int ui = (int)u;
                            int vi = (int)v;
                            double uf = u - ui;
                            double vf = v - vi;

                            float s00 = result_buffers[i].at<float>(vi, ui);
                            float s10 = result_buffers[i].at<float>(vi, ui + 1);
                            float s01 = result_buffers[i].at<float>(vi + 1, ui);
                            float s11 = result_buffers[i].at<float>(vi + 1, ui + 1);

                            double s0 = s00 * (1 - uf) + s10 * uf;
                            double s1 = s01 * (1 - uf) + s11 * uf;
                            score += s0 * (1 - vf) + s1 * vf;
                            valid_cams++;
                        }
                    }
                }

                // Apply a strict 3D distance penalty based on maxPenalty
                double dist_sq = dx * dx + dy * dy + dz * dz;
                double maxPenalty = marker->getMaxPenalty();
                double sigma_3d = (maxPenalty > 0) ? maxPenalty / 10.0 : 2.0; // scale 2D pixel penalty to ~mm stddev
                double inv_2sigma_sq = 1.0 / (2.0 * sigma_3d * sigma_3d);
                double penalty = exp(-dist_sq * inv_2sigma_sq);
                
                score *= penalty; // Penalize the joint correlation score

                if (valid_cams >= 2 && score > max_score)
                {
                    max_score = score;
                    best_p3d = p3d;
                }
            }
        }
    }

    m_best3D = best_p3d;
    m_best2D.resize(Project::getInstance()->getCameras().size());
    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        if (Project::getInstance()->getCameras()[i]->isVisible() && !result_buffers[i].empty())
        {
            m_best2D[i] = Project::getInstance()->getCameras()[i]->projectPoint(best_p3d, Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage());
        }
    }
}

void MarkerTracking3D::trackMarker_threadFinished()
{
    Marker* marker = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];
    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        if (Project::getInstance()->getCameras()[i]->isVisible())
        {
            if (m_best2D.size() > i && m_best2D[i].x > 0)
            {
                marker->setPoint(i, m_frame_to, m_best2D[i].x, m_best2D[i].y, TRACKED);
            }
        }
    }
    
    delete m_FutureWatcher;
    nbInstances--;
    if (nbInstances == 0)
    {
        emit trackMarker_finished();
    }
    delete this;
}
