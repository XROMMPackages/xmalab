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
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <sstream>

using namespace xma;

int MarkerTracking3D::nbInstances = 0;
namespace
{
    // ------------------------------------------------------------------
    // Diagnostics. Set the environment variable XMALAB_TRACK3D_DEBUG to a
    // directory before launching XMALab and the tracker writes, per tracked
    // (frame, marker, camera): the template, the search ROI, the raw NCC map
    // and the spatially weighted NCC map as PNGs, plus a text log
    // (track3d_log.txt) with predictions, peaks, 3D candidates and results.
    // Leaving the variable unset disables all of this at negligible cost.
    // ------------------------------------------------------------------
    const std::string& debugDir()
    {
        static const std::string dir = []() {
            const char* env = std::getenv("XMALAB_TRACK3D_DEBUG");
            std::string d = env ? env : "";
            if (!d.empty())
                QDir().mkpath(QString::fromStdString(d));
            return d;
        }();
        return dir;
    }

    bool debugEnabled()
    {
        return !debugDir().empty();
    }

    // Runtime A/B switches for diagnosis. XMALAB_TRACK3D_DISABLE is a comma-separated
    // list of feature names to turn off: "mask" (disc-masked template), "claims"
    // (down-weighting of other markers' positions), "guard" (merged-blob snap guard),
    // "nms" (peak separation by marker radius, reverts to 3 px), "size" (recording the
    // marker size for 3D-tracked frames). Unset means everything is on.
    bool featureDisabled(const char* name)
    {
        static const std::vector<std::string> disabled = []() {
            std::vector<std::string> out;
            const char* env = std::getenv("XMALAB_TRACK3D_DISABLE");
            if (!env)
                return out;
            std::string s(env);
            std::string::size_type start = 0;
            while (start <= s.size())
            {
                std::string::size_type end = s.find(',', start);
                if (end == std::string::npos)
                    end = s.size();
                std::string tok = s.substr(start, end - start);
                tok.erase(std::remove_if(tok.begin(), tok.end(), [](unsigned char c) { return std::isspace(c); }), tok.end());
                if (!tok.empty())
                    out.push_back(tok);
                start = end + 1;
            }
            return out;
        }();
        return std::find(disabled.begin(), disabled.end(), std::string(name)) != disabled.end();
    }

    std::string debugPath(int frame, int marker, int cam, const char* what, const char* ext = ".png")
    {
        std::ostringstream s;
        s << debugDir() << "/f" << frame << "_m" << marker;
        if (cam >= 0)
            s << "_c" << cam;
        s << "_" << what << ext;
        return s.str();
    }

    void debugLog(const std::string& line)
    {
        if (!debugEnabled())
            return;
        static std::mutex log_mutex;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream out(debugDir() + "/track3d_log.txt", std::ios::app);
        out << line << "\n";
    }

    // Writes a float map scaled to 0..255 (or an 8-bit image as is).
    void debugWriteImage(const std::string& path, const cv::Mat& m)
    {
        if (m.empty())
            return;
        cv::Mat out8;
        if (m.depth() == CV_8U)
        {
            out8 = m;
        }
        else
        {
            cv::normalize(m, out8, 0, 255, cv::NORM_MINMAX, CV_8U);
        }
        cv::imwrite(path, out8);
    }

    std::string fmtPt(const cv::Point2d& p)
    {
        std::ostringstream s;
        s.precision(3);
        s << std::fixed << "(" << p.x << ", " << p.y << ")";
        return s.str();
    }

    std::string fmtPt(const cv::Point3d& p)
    {
        std::ostringstream s;
        s.precision(3);
        s << std::fixed << "(" << p.x << ", " << p.y << ", " << p.z << ")";
        return s.str();
    }

    // Down-weight map locations near positions claimed by other markers (image
    // coordinates). Each claim multiplies the map by 1 - a*exp(-d^2/(2 sigma^2)) with
    // sigma = 0.6 r, which flattens the neighbour's own NCC peak (width ~r) while
    // leaving a touching marker's peak, one full radius away, at ~80% or more.
    void applyOtherMarkerSuppression(cv::Mat& ncc_map, const cv::Point2d& offset,
                                     const std::vector<cv::Point2d>& claims, int marker_size)
    {
        if (claims.empty())
            return;

        const double amplitude = 0.75;
        const double sigma = 0.6 * marker_size;
        const double inv_2sigma_sq = 1.0 / (2.0 * sigma * sigma);
        const double cutoff_sq = (4.0 * sigma) * (4.0 * sigma);

        int rows = ncc_map.rows;
        int cols = ncc_map.cols;
        float* data = ncc_map.ptr<float>();
        for (const auto& c : claims)
        {
            double cx = c.x - offset.x;
            double cy = c.y - offset.y;
            int i0 = std::max(0, static_cast<int>(std::floor(cy - 4.0 * sigma)));
            int i1 = std::min(rows - 1, static_cast<int>(std::ceil(cy + 4.0 * sigma)));
            int j0 = std::max(0, static_cast<int>(std::floor(cx - 4.0 * sigma)));
            int j1 = std::min(cols - 1, static_cast<int>(std::ceil(cx + 4.0 * sigma)));
            for (int i = i0; i <= i1; ++i)
            {
                double di = cy - i;
                for (int j = j0; j <= j1; ++j)
                {
                    double dj = cx - j;
                    double d_sq = di * di + dj * dj;
                    if (d_sq > cutoff_sq)
                        continue;
                    double w = 1.0 - amplitude * exp(-d_sq * inv_2sigma_sq);
                    data[i * cols + j] *= static_cast<float>(w);
                }
            }
        }
    }

    // Gaussian prior around the 2D prediction, given in NCC-map coordinates.
    void applySpatialWeight(cv::Mat& ncc_map, double pred_cx, double pred_cy, int search_radius_px)
    {
        int rows = ncc_map.rows;
        int cols = ncc_map.cols;
        double sigma = search_radius_px * 2.0;
        double inv_2sigma_sq = 1.0 / (2.0 * sigma * sigma);

        float* data = ncc_map.ptr<float>();
        for (int i = 0; i < rows; ++i)
        {
            double di = pred_cy - i;
            for (int j = 0; j < cols; ++j)
            {
                double dj = pred_cx - j;
                double w = exp((di * di + dj * dj) * -inv_2sigma_sq);
                data[i * cols + j] *= static_cast<float>(w);
            }
        }
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

    // The per-marker tracking penalty (0..100, default 50) sets how much the
    // distance-from-prediction prior weighs against the reprojected NCC score.
    m_penaltyWeight = std::min(100, std::max(0, mkr->getMaxPenalty())) / 100.0;

    // Disc mask for the (2*(size+3)+1) square template: keep the marker (radius ~size)
    // plus a 2 px ring of background, drop the corners where a touching neighbour
    // intrudes most, so it cannot bias the normalised cross-correlation.
    int templ_half = size + 3;
    int templ_dim = 2 * templ_half + 1;
    m_templateMask = cv::Mat::zeros(templ_dim, templ_dim, CV_8UC1);
    cv::circle(m_templateMask, cv::Point(templ_half, templ_half), size + 2, cv::Scalar(255), cv::FILLED);

    for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
    {
        // A camera in which the marker is undefined at the source frame has its 2D point
        // at the (-2,-2) placeholder; cutting a template there yields garbage, so leave the
        // template empty and the camera is skipped by trackMarker_thread.
        if (Project::getInstance()->getCameras()[i]->isVisible() &&
            mkr->getStatus2D()[i][m_frame_from] > UNDEFINED)
        {
            double x_from = mkr->getPoints2D()[i][m_frame_from].x;
            double y_from = mkr->getPoints2D()[i][m_frame_from].y;
            cv::Mat templ;
            Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage()->getSubImage(templ, size + 3, x_from, y_from);
            m_templates.push_back(templ);

            if (debugEnabled())
            {
                debugWriteImage(debugPath(m_frame_to, m_marker, i, "templ"), templ);
                std::ostringstream s;
                s << "f" << m_frame_to << " m" << m_marker << " c" << i
                  << " template from frame " << m_frame_from << " at " << fmtPt(cv::Point2d(x_from, y_from))
                  << " half-size " << size + 3;
                debugLog(s.str());
            }
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

    // Greedy non-maximum suppression: keep a peak only if it is at least min_dist
    // from every stronger peak already kept, so max_peaks slots go to distinct blobs.
    std::vector<Peak> kept;
    double min_dist_sq = min_dist * min_dist;
    for (const auto& p : peaks)
    {
        bool suppressed = false;
        for (const auto& k : kept)
        {
            double dx = p.pt.x - k.pt.x;
            double dy = p.pt.y - k.pt.y;
            if (dx * dx + dy * dy < min_dist_sq)
            {
                suppressed = true;
                break;
            }
        }
        if (!suppressed)
        {
            kept.push_back(p);
            if (max_peaks > 0 && static_cast<int>(kept.size()) >= max_peaks)
                break;
        }
    }

    return kept;
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
    // With the default penalty of 50 this is the former fixed 0.5 + 0.5 * penalty.
    score *= ((1.0 - m_penaltyWeight) + m_penaltyWeight * penalty);

    return score;
}

void MarkerTracking3D::trackMarker_thread()
{
    Marker* marker = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];

    // Marker stores an undefined 3D point as (-1000,-1000,-1000) with status3D <= UNDEFINED,
    // so the status is the reliable test (a literal (0,0,0) check never fired).
    if (marker->getStatus3D()[m_frame_from] <= UNDEFINED)
    {
        m_best3D = cv::Point3d(-1000, -1000, -1000);
        if (debugEnabled())
        {
            std::ostringstream s;
            s << "f" << m_frame_to << " m" << m_marker << " no 3D point at frame " << m_frame_from << ", nothing written";
            debugLog(s.str());
        }
        return;
    }

    cv::Point3d pred3D = marker->getPoints3D()[m_frame_from];

    bool have_velocity = false;
    cv::Point3d velocity(0, 0, 0);

    int prev_frame = m_forward ? m_frame_from - 1 : m_frame_from + 1;
    if (prev_frame >= 0 && prev_frame < static_cast<int>(marker->getPoints3D().size()))
    {
        if (marker->getStatus3D()[prev_frame] > UNDEFINED)
        {
            cv::Point3d prev3D = marker->getPoints3D()[prev_frame];
            velocity.x = pred3D.x - prev3D.x;
            velocity.y = pred3D.y - prev3D.y;
            velocity.z = pred3D.z - prev3D.z;
            have_velocity = true;
        }
    }

    double speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    int marker_size = static_cast<int>(marker->getSize() + 0.5);
    marker_size = (marker_size < 5) ? 5 : marker_size;

    double min_radius = 5.0;
    double dynamic_radius = speed * 2.0 + min_radius;
    int search_radius_px = std::max(30, marker_size * 3);

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
        int prediction = marker->getMarkerPrediction(i, m_frame_to, x_to, y_to, m_forward);
        if (prediction == 0)
        {
            // No 2D history in this camera (can only happen if the source frame is undefined
            // here, which the constructor already excludes; kept as a safety net so x_to/y_to
            // are never read uninitialised). Predict from the 3D point instead.
            cv::Point2d proj = cameras[i]->projectPoint(pred3D,
                Project::getInstance()->getTrials()[m_trial]->getReferenceCalibrationImage());
            x_to = proj.x;
            y_to = proj.y;
        }
        cam_results[i].pred2D = cv::Point2d(x_to, y_to);

        // Image::getSubImage(mat, half, off_x, off_y) returns a (2*half+1) square whose
        // top-left is (off_x, off_y). To search +-search_radius_px around the prediction
        // with a template of half-size used_template_size, the ROI half-size is
        // search_radius_px + used_template_size; the resulting NCC map is then
        // (2*search_radius_px+1) square with the prediction at its centre.
        int off_x = static_cast<int>(x_to - search_radius_px - used_template_size + 0.5);
        int off_y = static_cast<int>(y_to - search_radius_px - used_template_size + 0.5);

        cv::Mat ROI_to;
        Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage()->getSubImage(
            ROI_to, search_radius_px + used_template_size, off_x, off_y);

        int result_cols = ROI_to.cols - templ.cols + 1;
        int result_rows = ROI_to.rows - templ.rows + 1;

        if (result_cols <= 0 || result_rows <= 0)
        {
            cam_results[i].ncc_map = cv::Mat();
            continue;
        }

        cv::Mat result;
        result.create(result_rows, result_cols, CV_32FC1);
        if (featureDisabled("mask"))
            cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
        else
            cv::matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED, m_templateMask);

        if (debugEnabled())
        {
            cv::Mat masked_templ;
            templ.copyTo(masked_templ, m_templateMask);
            debugWriteImage(debugPath(m_frame_to, m_marker, i, "templ_masked"), masked_templ);
            debugWriteImage(debugPath(m_frame_to, m_marker, i, "roi"), ROI_to);
            debugWriteImage(debugPath(m_frame_to, m_marker, i, "ncc_raw"), result);
            double mn, mx;
            cv::minMaxLoc(result, &mn, &mx);
            std::ostringstream s;
            s << "f" << m_frame_to << " m" << m_marker << " c" << i
              << " raw ncc range [" << mn << ", " << mx << "]";
            debugLog(s.str());
        }

        // TM_CCORR_NORMED without mean subtraction is ~0.98-0.99 everywhere on grey X-ray
        // background, so the true peak beats the background by well under 1%. Any
        // multiplicative prior applied to that raw map dominates it. Stretch the map to
        // [0, 1] first, as the 2D tracker does, so the priors act on the real contrast.
        cv::normalize(result, result, 0.0, 1.0, cv::NORM_MINMAX);

        // Map pixel (k, l) corresponds to a template centre at image (off_x + t + k, off_y + t + l).
        cam_results[i].offset = cv::Point2d(off_x + used_template_size, off_y + used_template_size);

        applySpatialWeight(result, x_to - cam_results[i].offset.x, y_to - cam_results[i].offset.y, search_radius_px);

        // Positions the trial's other markers claim in this camera at the target frame:
        // their point there if already defined, otherwise their own 2D prediction.
        // A claim within one radius of our prediction is ignored, because then one of
        // the two markers has already jumped and we cannot tell which.
        std::vector<cv::Point2d> claims;
        if (!featureDisabled("claims"))
        {
            const auto& all_markers = Project::getInstance()->getTrials()[m_trial]->getMarkers();
            for (unsigned int j = 0; j < all_markers.size(); j++)
            {
                if (static_cast<int>(j) == m_marker)
                    continue;
                Marker* other = all_markers[j];
                cv::Point2d claim;
                if (other->getStatus2D()[i][m_frame_to] > UNDEFINED)
                {
                    claim = other->getPoints2D()[i][m_frame_to];
                }
                else
                {
                    double ox, oy;
                    if (other->getMarkerPrediction(i, m_frame_to, ox, oy, m_forward) == 0)
                        continue;
                    claim = cv::Point2d(ox, oy);
                }
                double ddx = claim.x - x_to;
                double ddy = claim.y - y_to;
                if (ddx * ddx + ddy * ddy < static_cast<double>(marker_size) * marker_size)
                    continue;
                // Only claims that can touch the search map matter.
                if (std::abs(ddx) > search_radius_px + 3.0 * marker_size ||
                    std::abs(ddy) > search_radius_px + 3.0 * marker_size)
                    continue;
                claims.push_back(claim);
            }
        }
        applyOtherMarkerSuppression(result, cam_results[i].offset, claims, marker_size);

        if (debugEnabled() && !claims.empty())
        {
            std::ostringstream s;
            s << "f" << m_frame_to << " m" << m_marker << " c" << i << " suppressing claims:";
            for (const auto& c : claims)
                s << " " << fmtPt(c);
            debugLog(s.str());
        }

        cam_results[i].ncc_map = result;

        // Two distinct markers are at least ~2 radii apart, so a minimum peak separation
        // of one radius drops sub-peaks of the same blob without merging neighbours.
        cam_results[i].peaks = extractPeaks(result, 2, featureDisabled("nms") ? 3.0 : marker_size);

        if (debugEnabled())
        {
            debugWriteImage(debugPath(m_frame_to, m_marker, i, "ncc_weighted"), result);
            double mn, mx;
            cv::minMaxLoc(result, &mn, &mx);
            std::ostringstream s;
            s << "f" << m_frame_to << " m" << m_marker << " c" << i
              << " pred2D " << fmtPt(cv::Point2d(x_to, y_to))
              << " roi off " << fmtPt(cv::Point2d(off_x, off_y))
              << " map " << result.cols << "x" << result.rows
              << " ncc range [" << mn << ", " << mx << "]"
              << " peaks:";
            for (const auto& p : cam_results[i].peaks)
                s << " " << fmtPt(p.pt + cam_results[i].offset) << "=" << p.score;
            debugLog(s.str());
        }

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

                            if (debugEnabled())
                            {
                                std::ostringstream s;
                                s << "f" << m_frame_to << " m" << m_marker
                                  << " cand c" << cam_a << fmtPt(img_pt_a) << " x c" << cam_b << fmtPt(img_pt_b)
                                  << " -> " << fmtPt(p3d_candidate) << " dist " << dist3D
                                  << " score " << score << " cams " << valid_cams;
                                debugLog(s.str());
                            }

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

    if (debugEnabled())
    {
        std::ostringstream s;
        s << "f" << m_frame_to << " m" << m_marker
          << " pred3D " << fmtPt(pred3D) << " velocity " << fmtPt(velocity)
          << " result3D " << fmtPt(best_p3d)
          << (found_valid ? " (candidate)" : (have_velocity ? " (velocity fallback)" : " (prediction fallback)"))
          << " score " << best_score;
        for (unsigned int i = 0; i < num_cameras; i++)
            if (cameras[i]->isVisible() && !cam_results[i].ncc_map.empty())
                s << " c" << i << fmtPt(m_best2D[i]);
        debugLog(s.str());
    }
}

void MarkerTracking3D::trackMarker_threadFinished()
{
	Marker* marker = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker];

	if (m_best2D.empty())
	{
		delete m_FutureWatcher;
		nbInstances--;
		if (nbInstances == 0)
			emit trackMarker_finished();
		delete this;
		return;
	}

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

                // detectionPoint returns the input centre unchanged (and leaves *size
                // uninitialised) when it finds nothing, so "found" is an exact comparison.
                double detected_size = -1.0;
                cv::Point2d refined = MarkerDetection::detectionPoint(
                    Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[i]->getImage(),
                    method,
                    m_best2D[i],
                    searchArea,
                    masksize,
                    threshold,
                    &detected_size,
                    NULL,
                    false
                );
                bool found = (refined.x != m_best2D[i].x || refined.y != m_best2D[i].y);

                // Two touching markers threshold into one blob whose enclosing circle is
                // about twice the marker's; its centroid sits between them. Refuse to snap
                // onto such a blob and keep the projected 3D position instead. The mean
                // size is only trusted once it has a history (updateMeanSize accepts 1..50).
                double mean_size = marker->getSize();
                bool merged = found && mean_size > 1.0 && detected_size > 1.5 * mean_size && !featureDisabled("guard");

                bool accepted = found && !merged && refined.x > 0 && refined.y > 0 &&
                    std::abs(refined.x - m_best2D[i].x) <= searchArea &&
                    std::abs(refined.y - m_best2D[i].y) <= searchArea;

                if (debugEnabled())
                {
                    std::ostringstream s;
                    s << "f" << m_frame_to << " m" << m_marker << " c" << i
                      << " snap from " << fmtPt(m_best2D[i]) << " to " << fmtPt(refined)
                      << " size " << (found ? detected_size : -1.0) << " mean " << mean_size
                      << (accepted ? " accepted" : (merged ? " rejected (merged blob)" : " rejected"));
                    debugLog(s.str());
                }

                if (accepted)
                {
                    m_best2D[i] = refined;
                    // Same order as MarkerDetection::detectMarker_threadFinished: size first,
                    // then the point (setPoint triggers the 3D reconstruction).
                    if (!featureDisabled("size"))
                        marker->setSize(i, m_frame_to, detected_size);
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
