#ifndef FLYTRACK_CONFIG_HPP
#define FLYTRACK_CONFIG_HPP

#include "rtn_status.hpp"
#include <QVariantMap>
#include <QColor>
#include <vector>


namespace bias
{

    const int N_ROI_TYPES = 2;
    enum ROIType { CIRCLE, NONE };

    const int N_FLY_VS_BG_MODES = 3;
    enum FlyVsBgModeType { FLY_DARKER_THAN_BG, FLY_BRIGHTER_THAN_BG, FLY_ANY_DIFFERENCE_BG };

    bool roiTypeToString(ROIType roiType, QString& roiTypeString);
    bool roiTypeFromString(QString roiTypeString, ROIType& roiType);
    bool flyVsBgModeToString(FlyVsBgModeType flyVsBgMode, QString& flyVsBgModeString);
    bool flyVsBgModeFromString(QString flyVsBgModeString, FlyVsBgModeType& flyVsBgMode);

    class FlyTrackConfig
    {

        public:

            // default parameters
            static const QString DEFAULT_BG_IMAGE_FILE_PATH; // saved background median estimate
            static const QString DEFAULT_TMP_OUT_DIR; // temporary output directory

            static const int DEFAULT_BACKGROUND_THRESHOLD; // foreground/background threshold, between 0 and 255
            static const int DEFAULT_N_FRAMES_SKIP_BG_EST; // number of frames skipped between frames added to background model
            static const FlyVsBgModeType DEFAULT_FLY_VS_BG_MODE; // whether the fly is darker than the background
            static const ROIType DEFAULT_ROI_TYPE; // type of ROI
            static const double DEFAULT_ROI_CENTER_X_FRAC; // x-coordinate of ROI center, relative
            static const double DEFAULT_ROI_CENTER_Y_FRAC; // y-coordinate of ROI center, relative
            static const double DEFAULT_ROI_RADIUS_FRAC; // radius of ROI, relative
            static const int DEFAULT_HISTORY_BUFFER_LENGTH; // number of frames to buffer velocity, orientation
            static const int DEFAULT_MAX_TRACK_QUEUE_LENGTH; // maximum number of track frames to buffer
            static const double DEFAULT_MIN_VELOCITY_MAGNITUDE; // minimum velocity magnitude in pixels/frame to consider fly moving
            static const double DEFAULT_HEAD_TAIL_WEIGHT_VELOCITY; // weight of velocity dot product in head-tail orientation resolution
            static const double DEFAULT_MIN_VEL_MATCH_DOTPROD; // minimum dot product for velocity matching
            static const bool DEFAULT_DEBUG; // flag for debugging
            static const bool DEFAULT_COMPUTE_BG_MODE; // flag of whether to compute the background (true) when camera is running or track a fly (false)

            // wing tracking defaults
            static const bool DEFAULT_TRACK_WINGS; // whether to track wings
            static const int DEFAULT_MINDWING_HIGH; // wing hysteresis high seed threshold on bg difference
            static const int DEFAULT_MINDWING_LOW; // wing hysteresis low threshold on bg difference
            static const int DEFAULT_MINDBODY; // body vs wing threshold on bg difference
            static const double DEFAULT_MAX_WINGPX_ANGLE_DEG; // max angle (deg) of a wing pixel from the rear axis
            static const double DEFAULT_MIN_NONZERO_WING_ANGLE_DEG; // min |wing angle| (deg) used in same-side rejection
            static const int DEFAULT_WING_MIN_PEAK_DIST_BINS; // min bin gap between the two histogram peaks
            static const double DEFAULT_WING_MIN_PEAK_THRESHOLD_FRAC; // min fraction in the primary peak bin
            static const int DEFAULT_NBINS_DTHETA_WING; // number of wing-angle histogram bins
            static const double DEFAULT_WING_PEAK_MIN_FRAC_FACTOR; // 2nd-peak threshold = factor / nBins
            static const int DEFAULT_MIN_SINGLE_WING_AREA; // min wing pixels to attempt a fit / per detected wing
            static const int DEFAULT_RADIUS_DILATE_BODY; // disk radius for body-mask dilation
            static const int DEFAULT_RADIUS_OPEN_WING; // disk radius for wing-mask open+close
            static const int DEFAULT_WING_RADIUS_QUADFIT_BINS; // +/- bins for sub-bin quadratic peak refine
            static const double DEFAULT_HEAD_TAIL_WEIGHT_WING; // weight of wing fit in head-tail resolution


            // parameters
            bool computeBgMode; // flag of whether to compute the background (true) when camera is running or track a fly (false)
            QString bgImageFilePath; // saved background median estimate
            QString tmpOutDir; // temporary output directory
            QString logFilePath; // log file path
            int backgroundThreshold; // foreground threshold
            int nFramesSkipBgEst; // number of frames to skip between frames added to background model
            FlyVsBgModeType flyVsBgMode; // whether the fly is darker than the background
            ROIType roiType; // type of ROI
            double roiCenterX; // x-coordinate of ROI center
            double roiCenterY; // y-coordinate of ROI center
            double roiRadius; // radius of ROI
            int historyBufferLength; // number of frames to buffer velocity, orientation
            int maxTrackQueueLength; // number of tracks to buffer
            double minVelocityMagnitude; // minimum velocity magnitude in pixels/frame to consider fly moving
            double headTailWeightVelocity; // weight of velocity dot product in head-tail orientation resolution
            bool DEBUG; // flag for debugging
            QString trackFileName; // relative name of output track file
            QString tmpTrackFilePath; // absolute path of track file -- not stored in config file

            // wing tracking parameters
            bool trackWings; // whether to track wings
            int mindWingHigh; // wing hysteresis high seed threshold on bg difference
            int mindWingLow; // wing hysteresis low threshold on bg difference
            int mindBody; // body vs wing threshold on bg difference
            double maxWingPxAngleDeg; // max angle (deg) of a wing pixel from the rear axis
            double minNonzeroWingAngleDeg; // min |wing angle| (deg) for same-side rejection
            int wingMinPeakDistBins; // min bin gap between the two histogram peaks
            double wingMinPeakThresholdFrac; // min fraction in the primary peak bin
            int nBinsDThetaWing; // number of wing-angle histogram bins
            double wingPeakMinFracFactor; // 2nd-peak threshold = factor / nBins
            int minSingleWingArea; // min wing pixels to attempt a fit / per detected wing
            int radiusDilateBody; // disk radius for body-mask dilation
            int radiusOpenWing; // disk radius for wing-mask open+close
            int wingRadiusQuadfitBins; // +/- bins for sub-bin quadratic peak refine
            std::vector<double> wingFracFilter; // wing-angle histogram smoothing kernel
            double headTailWeightWing; // weight of wing fit in head-tail resolution

            FlyTrackConfig();
            FlyTrackConfig FlyTrackConfig::copy();
            void setRoiParams(ROIType roiTypeNew, double roiCenterXNew, double roiCenterYNew, double roiRadiusNew);

            RtnStatus setBgEstFromMap(QVariantMap configMap);
            RtnStatus setRoiFromMap(QVariantMap configMap);
            RtnStatus setBgSubFromMap(QVariantMap configMap);
            RtnStatus setHeadTailFromMap(QVariantMap configMap);
            RtnStatus setWingFromMap(QVariantMap configMap);
            RtnStatus setMiscFromMap(QVariantMap configMap);
            QVariantMap toMap();
            RtnStatus fromMap(QVariantMap configMap);
            RtnStatus fromJson(QByteArray jsonConfigArray);
            QByteArray toJson();
            QString toString();
            bool trackFilePathSet();
            bool trackFileNameSet();

            void print();

    };
}

#endif