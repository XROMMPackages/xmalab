#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerTracking3D.h"

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/Camera.h"
#include "processing/MarkerDetection.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <map>
#include <mutex>
#include <algorithm>
#include <cmath>

using namespace xma;

int MarkerTracking3D::nbInstances = 0;
namespace
{
    struct PenaltyCacheEntry
    {
        cv::Mat mat;
    };

    const PenaltyCacheEntry& getPenaltySurface(int rows, int cols)
    {
        static std::mutex cacheMutex;
        static std::map<std::pair<int, int>, PenaltyCacheEntry> cache;

        std::lock_guard<std::mutex> lock(cacheMutex);
        const auto key = std::make_pair(rows, cols);
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;

        auto [insertIt, inserted] = cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(key), std::forward_as_tuple());
        PenaltyCacheEntry& entry = insertIt->second;
        entry.mat.create(rows, cols, CV_32FC1);

        double halfcol = 0.5 * cols;
        double halfrow = 0.5 * rows;
        double sigma = halfcol * 3.0;
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
        return entry;
    }
}

MarkerTracking3D::MarkerTracking3D(int trial, int frame_from, int frame_to, int marker, bool forward)
    : QObject()
    , m_trial(trial)
    , m_frame_from(frame_from)
    , m_frame_to(frame_to)
    , m_marker(marker)
    , m_forward(forward)
{
    nbInstances++;

    Marker* mkr = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];
    int size = static_cast<int>(mkr->getSize() + 0.5);
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

std::vector<MarkerTracking3D::Peak> MarkerTracking3D::extractPeaks(const cv::Mat& ncc_map, int max_peaks, double min_dist)
{
    std::vector<Peak> peaks;

    cv::Mat dilated;
    cv::dilate(ncc_map, dilated, cv::Mat());
    cv::Mat peaks_mask = (ncc_map == dilated) & (ncc_map > 0.3f);

    std::vector<cv::Point> locations;
    cv::findNonZero(peaks_mask, locations);

    for (const auto& loc : locations)
    {
        float score = ncc_map.at<float>(loc.y, loc.x);
        peaks.push_back({ cv::Point2d(loc.x, loc.y), score });
    }

    std::sort(peaks.begin(), peaks.end(), [](const Peak& a, const Peak& b) {
        return a.score > b.score;
    });

    if (max_peaks > 0 && static_cast<int>(peaks.size()) > max_peaks)
        peaks.resize(max_peaks);

    return peaks;
}

bool MarkerTracking3D::triangulatePair(const cv::Point2d& pt1, int cam1,
                                       const cv::Point2d& pt2, int cam2,
                                       cv::Point3d& result) const
{
    Camera* camera1 = Project::getInstance()->getCameras()[cam1];
    Camera* camera2 = Project::getInstance()->getCameras()[cam2];
    int refCal = Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage();

    cv::Point2d u1 = camera1->undistortPoint(pt1, true);
    cv::Point2d u2 = camera2->undistortPoint(pt2, true);

    cv::Mat P1 = camera1->getProjectionMatrix(refCal);
    cv::Mat P2 = camera2->getProjectionMatrix(refCal);

    cv::Mat A(4, 4, CV_64F);
    A.at<double>(0, 0) = u1.x * P1.at<double>(2, 0) - P1.at<double>(0, 0);
    A.at<double>(0, 1) = u1.x * P1.at<double>(2, 1) - P1.at<double>(0, 1);
    A.at<double>(0, 2) = u1.x * P1.at<double>(2, 2) - P1.at<double>(0, 2);
    A.at<double>(0, 3) = u1.x * P1.at<double>(2, 3) - P1.at<double>(0, 3);

    A.at<double>(1, 0) = u1.y * P1.at<double>(2, 0) - P1.at<double>(1, 0);
    A.at<double>(1, 1) = u1.y * P1.at<double>(2, 1) - P1.at<double>(1, 1);
    A.at<double>(1, 2) = u1.y * P1.at<double>(2, 2) - P1.at<double>(1, 2);
    A.at<double>(1, 3) = u1.y * P1.at<double>(2, 3) - P1.at<double>(1, 3);

    A.at<double>(2, 0) = u2.x * P2.at<double>(2, 0) - P2.at<double>(0, 0);
    A.at<double>(2, 1) = u2.x * P2.at<double>(2, 1) - P2.at<double>(0, 1);
    A.at<double>(2, 2) = u2.x * P2.at<double>(2, 2) - P2.at<double>(0, 2);
    A.at<double>(2, 3) = u2.x * P2.at<double>(2, 3) - P2.at<double>(0, 3);

    A.at<double>(3, 0) = u2.y * P2.at<double>(2, 0) - P2.at<double>(1, 0);
    A.at<double>(3, 1) = u2.y * P2.at<double>(2, 1) - P2.at<double>(1, 1);
    A.at<double>(3, 2) = u2.y * P2.at<double>(2, 2) - P2.at<double>(1, 2);
    A.at<double>(3, 3) = u2.y * P2.at<double>(2, 3) - P2.at<double>(1, 3);

    cv::Mat X;
    cv::SVD::solveZ(A, X);

    if (std::abs(X.at<double>(3, 0)) < 1e-12)
        return false;

    result.x = X.at<double>(0, 0) / X.at<double>(3, 0);
    result.y = X.at<double>(1, 0) / X.at<double>(3, 0);
    result.z = X.at<double>(2, 0) / X.at<double>(3, 0);

    return true;
}

double MarkerTracking3D::evaluate3D(const cv::Point3d& p3d, const cv::Point3d& pred3D,
                                    const std::vector<CameraResult>& cam_results,
                                    int& valid_cams) const
{
    double score = 0.0;
    valid_cams = 0;

    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        if (!cam_results[i].ncc_map.empty())
        {
            cv::Point2d proj = Project::getInstance()->getCameras()[i]->projectPoint(
                p3d, Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage());

            double u = proj.x - cam_results[i].offset.x;
            double v = proj.y - cam_results[i].offset.y;

            if (u >= 0 && u < cam_results[i].ncc_map.cols - 1 &&
                v >= 0 && v < cam_results[i].ncc_map.rows - 1)
            {
                int ui = static_cast<int>(u);
                int vi = static_cast<int>(v);
                double uf = u - ui;
                double vf = v - vi;

                float s00 = cam_results[i].ncc_map.at<float>(vi, ui);
                float s10 = cam_results[i].ncc_map.at<float>(vi, ui + 1);
                float s01 = cam_results[i].ncc_map.at<float>(vi + 1, ui);
                float s11 = cam_results[i].ncc_map.at<float>(vi + 1, ui + 1);

                double s0 = s00 * (1.0 - uf) + s10 * uf;
                double s1 = s01 * (1.0 - uf) + s11 * uf;
                double interp = s0 * (1.0 - vf) + s1 * vf;

                score += interp;
                valid_cams++;
            }
            else
            {
                score -= 1.0;
            }
        }
    }

    double dist_sq = (p3d.x - pred3D.x) * (p3d.x - pred3D.x) +
                     (p3d.y - pred3D.y) * (p3d.y - pred3D.y) +
                     (p3d.z - pred3D.z) * (p3d.z - pred3D.z);
    double penalty_sigma = 5.0;
    double penalty = exp(-dist_sq / (2.0 * penalty_sigma * penalty_sigma));
    score *= (0.5 + 0.5 * penalty);

    return score;
}

void MarkerTracking3D::trackMarker_thread()
{
    Marker* marker = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];

    cv::Point3d pred3D = marker->getPoints3D()[m_frame_from];
    cv::Point3d pred3D_prev;
    bool have_velocity = false;
    cv::Point3d velocity(0, 0, 0);

    int prev_frame = m_forward ? m_frame_from - 1 : m_frame_from + 1;
    if (prev_frame >= 0 && prev_frame < static_cast<int>(marker->getPoints3D().size()))
    {
        cv::Point3d prev3D = marker->getPoints3D()[prev_frame];
        if (prev3D.x != 0 || prev3D.y != 0 || prev3D.z != 0)
        {
            velocity.x = pred3D.x - prev3D.x;
            velocity.y = pred3D.y - prev3D.y;
            velocity.z = pred3D.z - prev3D.z;
            have_velocity = true;
        }
    }

    double speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    int marker_size = static_cast<int>(marker->getSize() + 0.5);
    marker_size = (marker_size < 5) ? 5 : marker_size;

    double min_radius = marker_size * 2.0;
    double dynamic_radius = speed * 2.0 + min_radius;
    int search_radius_px = 30;

    const auto& cameras = Project::getInstance()->getCameras();
    unsigned int num_cameras = cameras.size();
    int used_template_size = marker_size + 3;

    std::vector<CameraResult> cam_results(num_cameras);

    for (unsigned int i = 0; i < num_cameras; i++)
    {
        if (!cameras[i]->isVisible() || m_templates[i].empty())
        {
            cam_results[i].ncc_map = cv::Mat();
            continue;
        }

        cv::Mat templ = m_templates[i];
        double x_to, y_to;
        marker->getMarkerPrediction(i, m_frame_to, x_to, y_to, m_forward);

        int off_x = static_cast<int>(x_to - search_radius_px - used_template_size + 0.5);
        int off_y = static_cast<int>(y_to - search_radius_px - used_template_size + 0.5);

        cv::Mat ROI_to;
        Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage()->getSubImage(
            ROI_to, 2 * search_radius_px + used_template_size, off_x, off_y);

        int result_cols = ROI_to.cols - templ.cols + 1;
        int result_rows = ROI_to.rows - templ.rows + 1;

        if (result_cols <= 0 || result_rows <= 0)
        {
            cam_results[i].ncc_map = cv::Mat();
            continue;
        }

        cv::Mat result;
        result.create(result_rows, result_cols, CV_32FC1);
        cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);

        const auto& entry = getPenaltySurface(result_rows, result_cols);
        cv::subtract(result, entry.mat, result);

        cam_results[i].ncc_map = result;
        cam_results[i].offset = cv::Point2d(off_x + used_template_size, off_y + used_template_size);

        cam_results[i].peaks = extractPeaks(result, 3, 3.0);

        ROI_to.release();
        templ.release();
    }

    std::vector<int> visible_cams;
    for (unsigned int i = 0; i < num_cameras; i++)
    {
        if (!cam_results[i].ncc_map.empty())
            visible_cams.push_back(static_cast<int>(i));
    }

    double best_score = -1e9;
    cv::Point3d best_p3d = pred3D;
    bool found_valid = false;

    if (visible_cams.size() >= 2)
    {
        for (size_t ai = 0; ai < visible_cams.size(); ai++)
        {
            for (size_t bj = ai + 1; bj < visible_cams.size(); bj++)
            {
                int cam_a = visible_cams[ai];
                int cam_b = visible_cams[bj];

                const auto& peaks_a = cam_results[cam_a].peaks;
                const auto& peaks_b = cam_results[cam_b].peaks;

                if (peaks_a.empty() || peaks_b.empty())
                    continue;

                for (const auto& pa : peaks_a)
                {
                    cv::Point2d img_pt_a = pa.pt + cam_results[cam_a].offset;

                    for (const auto& pb : peaks_b)
                    {
                        cv::Point2d img_pt_b = pb.pt + cam_results[cam_b].offset;

                        cv::Point3d p3d_candidate;
                        if (triangulatePair(img_pt_a, cam_a, img_pt_b, cam_b, p3d_candidate))
                        {
                            double dx = p3d_candidate.x - pred3D.x;
                            double dy = p3d_candidate.y - pred3D.y;
                            double dz = p3d_candidate.z - pred3D.z;
                            double dist3D = std::sqrt(dx * dx + dy * dy + dz * dz);

                            if (have_velocity && dist3D > dynamic_radius * 2.0)
                                continue;

                            int valid_cams;
                            double score = evaluate3D(p3d_candidate, pred3D, cam_results, valid_cams);

                            if (valid_cams >= 2 && score > best_score)
                            {
                                best_score = score;
                                best_p3d = p3d_candidate;
                                found_valid = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (found_valid)
    {
        int refine_valid;
        double refine_score = evaluate3D(best_p3d, pred3D, cam_results, refine_valid);

        double best_refine_score = refine_score;
        cv::Point3d refined_p3d = best_p3d;
        bool refined = false;

        for (int iter = 0; iter < 5; iter++)
        {
            const double fine_step = 0.5;
            cv::Point3d best_neighbor = refined_p3d;
            bool improved = false;

            for (int sdx = -1; sdx <= 1; sdx++)
            {
                for (int sdy = -1; sdy <= 1; sdy++)
                {
                    for (int sdz = -1; sdz <= 1; sdz++)
                    {
                        if (sdx == 0 && sdy == 0 && sdz == 0)
                            continue;

                        cv::Point3d neighbor(refined_p3d.x + sdx * fine_step,
                                              refined_p3d.y + sdy * fine_step,
                                              refined_p3d.z + sdz * fine_step);

                        int nv;
                        double ns = evaluate3D(neighbor, pred3D, cam_results, nv);
                        if (nv >= 2 && ns > best_refine_score)
                        {
                            best_refine_score = ns;
                            best_neighbor = neighbor;
                            improved = true;
                        }
                    }
                }
            }

            if (improved)
            {
                refined_p3d = best_neighbor;
                refined = true;
            }
            else
            {
                break;
            }
        }

        if (refined)
        {
            best_p3d = refined_p3d;
        }
    }
    else if (have_velocity)
    {
        best_p3d.x = pred3D.x + velocity.x;
        best_p3d.y = pred3D.y + velocity.y;
        best_p3d.z = pred3D.z + velocity.z;
    }

    m_best3D = best_p3d;
    m_best2D.resize(num_cameras);
    for (unsigned int i = 0; i < num_cameras; i++)
    {
        if (cameras[i]->isVisible() && !cam_results[i].ncc_map.empty())
        {
            m_best2D[i] = cameras[i]->projectPoint(
                best_p3d, Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage());
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
                int method = marker->getMethod();
                int searchArea = static_cast<int>(marker->getSize() + 0.5) + 3;
                int masksize = marker->getSize() * 2;
                double threshold = marker->getThresholdOffset();

                cv::Point2d refined = MarkerDetection::detectionPoint(
                    Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage(),
                    method,
                    m_best2D[i],
                    searchArea,
                    masksize,
                    threshold,
                    NULL,
                    NULL,
                    false
                );

                if (refined.x > 0 && refined.y > 0 &&
                    std::abs(refined.x - m_best2D[i].x) <= searchArea &&
                    std::abs(refined.y - m_best2D[i].y) <= searchArea)
                {
                    m_best2D[i] = refined;
                }

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
