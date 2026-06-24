#include "flytrack_config.hpp"
#include "json.hpp"
#include <iostream>
#include <QMessageBox>
#include <QtDebug>
#include <QFileInfo>

namespace bias
{

    const QString FlyTrackConfig::DEFAULT_BG_IMAGE_FILE_PATH = QString(""); // saved background median estimate
    const QString FlyTrackConfig::DEFAULT_TMP_OUT_DIR = QString("tmp"); // temporary output directory
    const int FlyTrackConfig::DEFAULT_BACKGROUND_THRESHOLD = 75; // foreground/background threshold, between 0 and 255
    const int FlyTrackConfig::DEFAULT_N_FRAMES_SKIP_BG_EST = 500; // number of frames used for background estimation, set to 0 to use all frames
    const FlyVsBgModeType FlyTrackConfig::DEFAULT_FLY_VS_BG_MODE = FLY_DARKER_THAN_BG; // whether the fly is darker than the background
    const ROIType FlyTrackConfig::DEFAULT_ROI_TYPE = CIRCLE; // type of ROI
    const int FlyTrackConfig::DEFAULT_HISTORY_BUFFER_LENGTH = 5; // number of frames to buffer velocity, orientation
	const int FlyTrackConfig::DEFAULT_MAX_TRACK_QUEUE_LENGTH = 10000; // maximum number of track frames to buffer
    const double FlyTrackConfig::DEFAULT_MIN_VELOCITY_MAGNITUDE = 1.0; // minimum velocity magnitude in pixels/frame to consider fly moving
    const double FlyTrackConfig::DEFAULT_HEAD_TAIL_WEIGHT_VELOCITY = 3.0; // weight of velocity dot product in head-tail orientation resolution
    const double FlyTrackConfig::DEFAULT_MIN_VEL_MATCH_DOTPROD = 0.25; // minimum dot product for velocity matching
    const bool FlyTrackConfig::DEFAULT_DEBUG = false; // flag for debugging
    const bool FlyTrackConfig::DEFAULT_COMPUTE_BG_MODE = false; // flag of whether to compute the background (true) when camera is running or track a fly (false)

    // wing tracking defaults (from production WingTrackingParameters)
    const bool FlyTrackConfig::DEFAULT_TRACK_WINGS = false;
    const int FlyTrackConfig::DEFAULT_MINDWING_HIGH = 50;
    const int FlyTrackConfig::DEFAULT_MINDWING_LOW = 30;
    const int FlyTrackConfig::DEFAULT_MINDBODY = 100;
    const double FlyTrackConfig::DEFAULT_MAX_WINGPX_ANGLE_DEG = 135.0; // 2.35619 rad
    const double FlyTrackConfig::DEFAULT_MIN_NONZERO_WING_ANGLE_DEG = 10.0; // 0.174533 rad
    const int FlyTrackConfig::DEFAULT_WING_MIN_PEAK_DIST_BINS = 3;
    const double FlyTrackConfig::DEFAULT_WING_MIN_PEAK_THRESHOLD_FRAC = 0.0;
    const int FlyTrackConfig::DEFAULT_NBINS_DTHETA_WING = 50;
    const double FlyTrackConfig::DEFAULT_WING_PEAK_MIN_FRAC_FACTOR = 2.0;
    const int FlyTrackConfig::DEFAULT_MIN_SINGLE_WING_AREA = 10;
    const int FlyTrackConfig::DEFAULT_RADIUS_DILATE_BODY = 1;
    const int FlyTrackConfig::DEFAULT_RADIUS_OPEN_WING = 1;
    const int FlyTrackConfig::DEFAULT_WING_RADIUS_QUADFIT_BINS = 1;
    const double FlyTrackConfig::DEFAULT_HEAD_TAIL_WEIGHT_WING = 10.0;

	FlyTrackConfig::FlyTrackConfig()
    {
        computeBgMode = DEFAULT_COMPUTE_BG_MODE;
        bgImageFilePath = DEFAULT_BG_IMAGE_FILE_PATH;
        tmpOutDir = DEFAULT_TMP_OUT_DIR;
		backgroundThreshold = DEFAULT_BACKGROUND_THRESHOLD;
        nFramesSkipBgEst = DEFAULT_N_FRAMES_SKIP_BG_EST;
		flyVsBgMode = DEFAULT_FLY_VS_BG_MODE;
		roiType = DEFAULT_ROI_TYPE;
		historyBufferLength = DEFAULT_HISTORY_BUFFER_LENGTH;
		maxTrackQueueLength = DEFAULT_MAX_TRACK_QUEUE_LENGTH;
		minVelocityMagnitude = DEFAULT_MIN_VELOCITY_MAGNITUDE;
		headTailWeightVelocity = DEFAULT_HEAD_TAIL_WEIGHT_VELOCITY;
		DEBUG = DEFAULT_DEBUG;
		roiCenterX = 0;
		roiCenterY = 0;
		roiRadius = 0;
        trackFileName = QString(""); // empty string means it is not set
        tmpTrackFilePath = QString(""); // empty string means it is not set
        // wing tracking
        trackWings = DEFAULT_TRACK_WINGS;
        mindWingHigh = DEFAULT_MINDWING_HIGH;
        mindWingLow = DEFAULT_MINDWING_LOW;
        mindBody = DEFAULT_MINDBODY;
        maxWingPxAngleDeg = DEFAULT_MAX_WINGPX_ANGLE_DEG;
        minNonzeroWingAngleDeg = DEFAULT_MIN_NONZERO_WING_ANGLE_DEG;
        wingMinPeakDistBins = DEFAULT_WING_MIN_PEAK_DIST_BINS;
        wingMinPeakThresholdFrac = DEFAULT_WING_MIN_PEAK_THRESHOLD_FRAC;
        nBinsDThetaWing = DEFAULT_NBINS_DTHETA_WING;
        wingPeakMinFracFactor = DEFAULT_WING_PEAK_MIN_FRAC_FACTOR;
        minSingleWingArea = DEFAULT_MIN_SINGLE_WING_AREA;
        radiusDilateBody = DEFAULT_RADIUS_DILATE_BODY;
        radiusOpenWing = DEFAULT_RADIUS_OPEN_WING;
        wingRadiusQuadfitBins = DEFAULT_WING_RADIUS_QUADFIT_BINS;
        wingFracFilter = { 0.25, 0.5, 0.25 };
        headTailWeightWing = DEFAULT_HEAD_TAIL_WEIGHT_WING;
	}

    FlyTrackConfig FlyTrackConfig::copy() {
    	FlyTrackConfig config;
        config.computeBgMode = computeBgMode;
		config.bgImageFilePath = bgImageFilePath;
		config.tmpOutDir = tmpOutDir;
		config.backgroundThreshold = backgroundThreshold;
        config.nFramesSkipBgEst = nFramesSkipBgEst;
		config.flyVsBgMode = flyVsBgMode;
		config.roiType = roiType;
		config.historyBufferLength = historyBufferLength;
		config.maxTrackQueueLength = maxTrackQueueLength;
		config.minVelocityMagnitude = minVelocityMagnitude;
		config.headTailWeightVelocity = headTailWeightVelocity;
		config.DEBUG = DEBUG;
		config.roiCenterX = roiCenterX;
		config.roiCenterY = roiCenterY;
		config.roiRadius = roiRadius;
        config.trackFileName = trackFileName;
        config.tmpTrackFilePath = tmpTrackFilePath;
        config.trackWings = trackWings;
        config.mindWingHigh = mindWingHigh;
        config.mindWingLow = mindWingLow;
        config.mindBody = mindBody;
        config.maxWingPxAngleDeg = maxWingPxAngleDeg;
        config.minNonzeroWingAngleDeg = minNonzeroWingAngleDeg;
        config.wingMinPeakDistBins = wingMinPeakDistBins;
        config.wingMinPeakThresholdFrac = wingMinPeakThresholdFrac;
        config.nBinsDThetaWing = nBinsDThetaWing;
        config.wingPeakMinFracFactor = wingPeakMinFracFactor;
        config.minSingleWingArea = minSingleWingArea;
        config.radiusDilateBody = radiusDilateBody;
        config.radiusOpenWing = radiusOpenWing;
        config.wingRadiusQuadfitBins = wingRadiusQuadfitBins;
        config.wingFracFilter = wingFracFilter;
        config.headTailWeightWing = headTailWeightWing;
		return config;

    }

    QString FlyTrackConfig::toString() {
        QString configStr;
        configStr += QString("computeBgMode: %1\n").arg(computeBgMode);
        configStr += QString("bgImageFilePath: %1\n").arg(bgImageFilePath);
        configStr += QString("tmpOutDir: %1\n").arg(tmpOutDir);
        configStr += QString("trackFileName: %1\n").arg(trackFileName);
        configStr += QString("tmpTrackFilePath: %1\n").arg(tmpTrackFilePath);
        configStr += QString("backgroundThreshold: %1\n").arg(backgroundThreshold);
        configStr += QString("nFramesSkipBgEst: %1\n").arg(nFramesSkipBgEst);
        configStr += QString("flyVsBgMode: %1\n").arg(flyVsBgMode);
        QString roiTypeString;
        roiTypeToString(roiType, roiTypeString);
        configStr += QString("roiType: %1\n").arg(roiTypeString);
        configStr += QString("roiCenterX: %1\n").arg(roiCenterX);
        configStr += QString("roiCenterY: %1\n").arg(roiCenterY);
        configStr += QString("roiRadius: %1\n").arg(roiRadius);
        configStr += QString("historyBufferLength: %1\n").arg(historyBufferLength);
        configStr += QString("maxTrackQueueLength: %1\n").arg(maxTrackQueueLength);
        configStr += QString("minVelocityMagnitude: %1\n").arg(minVelocityMagnitude);
        configStr += QString("headTailWeightVelocity: %1\n").arg(headTailWeightVelocity);
        configStr += QString("headTailWeightWing: %1\n").arg(headTailWeightWing);
        configStr += QString("DEBUG: %1\n").arg(DEBUG);
        configStr += QString("trackWings: %1\n").arg(trackWings);
        configStr += QString("mindWingHigh: %1\n").arg(mindWingHigh);
        configStr += QString("mindWingLow: %1\n").arg(mindWingLow);
        configStr += QString("mindBody: %1\n").arg(mindBody);
        configStr += QString("maxWingPxAngleDeg: %1\n").arg(maxWingPxAngleDeg);
        configStr += QString("minNonzeroWingAngleDeg: %1\n").arg(minNonzeroWingAngleDeg);
        configStr += QString("wingMinPeakDistBins: %1\n").arg(wingMinPeakDistBins);
        configStr += QString("wingMinPeakThresholdFrac: %1\n").arg(wingMinPeakThresholdFrac);
        configStr += QString("nBinsDThetaWing: %1\n").arg(nBinsDThetaWing);
        configStr += QString("wingPeakMinFracFactor: %1\n").arg(wingPeakMinFracFactor);
        configStr += QString("minSingleWingArea: %1\n").arg(minSingleWingArea);
        configStr += QString("radiusDilateBody: %1\n").arg(radiusDilateBody);
        configStr += QString("radiusOpenWing: %1\n").arg(radiusOpenWing);
        configStr += QString("wingRadiusQuadfitBins: %1\n").arg(wingRadiusQuadfitBins);
        QStringList wingFracFilterStrList;
        for (size_t i = 0; i < wingFracFilter.size(); i++)
            wingFracFilterStrList << QString::number(wingFracFilter[i]);
        configStr += QString("wingFracFilter: %1\n").arg(wingFracFilterStrList.join(","));
        return configStr;

    }

    void FlyTrackConfig::print() {
		std::cout << toString().toStdString();
	}

    bool FlyTrackConfig::trackFilePathSet() {
  		return !tmpTrackFilePath.isEmpty();
    }
    bool FlyTrackConfig::trackFileNameSet(){
        return !trackFileName.isEmpty();
    }


    RtnStatus FlyTrackConfig::fromMap(QVariantMap configMap) {
        RtnStatus rtnStatus;
        rtnStatus.success = true;

        QVariantMap oldConfigMap = toMap();
        RtnStatus rtnStatusBgEst = setBgEstFromMap(configMap["bgEst"].toMap());
        RtnStatus rtnStatusRoi = setRoiFromMap(configMap["roi"].toMap());
        RtnStatus rtnStatusBgSub = setBgSubFromMap(configMap["bgSub"].toMap());
        RtnStatus rtnStatusHeadTail = setHeadTailFromMap(configMap["headTail"].toMap());
        RtnStatus rtnStatusWing = setWingFromMap(configMap["wing"].toMap());
        RtnStatus rtnStatusMisc = setMiscFromMap(configMap["misc"].toMap());

        rtnStatus.success = rtnStatusBgEst.success && rtnStatusRoi.success && rtnStatusBgSub.success && rtnStatusHeadTail.success && rtnStatusWing.success;
        rtnStatus.message += rtnStatusBgEst.message + QString(", ");
        rtnStatus.message += rtnStatusRoi.message + QString(", ");
        rtnStatus.message += rtnStatusBgSub.message + QString(", ");
        rtnStatus.message += rtnStatusHeadTail.message + QString(", ");
        rtnStatus.message += rtnStatusWing.message + QString(", ");
        rtnStatus.message += rtnStatusMisc.message;

        return rtnStatus;

    }

    RtnStatus FlyTrackConfig::setBgEstFromMap(QVariantMap configMap) {
		RtnStatus rtnStatus;
		rtnStatus.success = true;
        rtnStatus.message = QString("");

        if (configMap.isEmpty())
        {
            rtnStatus.message = QString("flyTrack bgEst config empty");
            return rtnStatus;
        }
        if (configMap.contains("computeBgMode")) {
			if (configMap["computeBgMode"].canConvert<bool>())
				computeBgMode = configMap["computeBgMode"].toBool();
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert computeBgMode to bool");
			}
		}
        if (configMap.contains("bgImageFilePath")) {
            if (configMap["bgImageFilePath"].canConvert<QString>())
                bgImageFilePath = configMap["bgImageFilePath"].toString();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert bgImageFilePath to string");
            }
        }
        if (configMap.contains("nFramesSkipBgEst")) {
            if (configMap["nFramesSkipBgEst"].canConvert<int>())
                nFramesSkipBgEst = configMap["nFramesSkipBgEst"].toInt();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert nFramesSkipBgEst to int");
            }
        }
		return rtnStatus;
	}

    RtnStatus FlyTrackConfig::setRoiFromMap(QVariantMap configMap) {
		RtnStatus rtnStatus;
		rtnStatus.success = true;
		rtnStatus.message = QString("");

        if (configMap.isEmpty())
        {
			rtnStatus.message = QString("flyTrack roi config empty");
			return rtnStatus;
		}

        if (configMap.contains("roiType")) {
            if (configMap["roiType"].canConvert<QString>()) {
                QString roiTypeStr = configMap["roiType"].toString();
                bool success = roiTypeFromString(roiTypeStr, roiType);
                if(!success) {
                    rtnStatus.success = false;
                    rtnStatus.appendMessage(QString("unknown roiType %1").arg(roiTypeStr));
                }
            }
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert roiType to string");
			}
		}

        if (configMap.contains("roiCenterX")) {
            if (configMap["roiCenterX"].canConvert<double>()) {
                roiCenterX = configMap["roiCenterX"].toDouble();
            }
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage(QString("unable to convert roiCenterX to double"));
            }
        }

        if (configMap.contains("roiCenterY")) {
            if (configMap["roiCenterY"].canConvert<double>()) {
                roiCenterY = configMap["roiCenterY"].toDouble();
            }
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage(QString("unable to convert roiCenterY to double"));
            }
        }

        if (configMap.contains("roiRadius")) {
            if (configMap["roiRadius"].canConvert<double>()) {
                roiRadius = configMap["roiRadius"].toDouble();
            }
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage(QString("unable to convert roiRadius to double"));
            }
        }
		return rtnStatus;
	}   

    RtnStatus FlyTrackConfig::setBgSubFromMap(QVariantMap configMap) {

        RtnStatus rtnStatus;
		rtnStatus.success = true;
		rtnStatus.message = QString("");

        if (configMap.isEmpty())
        {
			rtnStatus.message = QString("flyTrack bgSub config empty");
			return rtnStatus;
		}

        if (configMap.contains("backgroundThreshold")) {
			if(configMap["backgroundThreshold"].canConvert<int>()) 
                backgroundThreshold = configMap["backgroundThreshold"].toInt();
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert backgroundThreshold to int");
			}
		}
        if (configMap.contains("flyVsBgMode")) {
            if (configMap["flyVsBgMode"].canConvert<QString>()) {
				QString flyVsBgModeStr = configMap["flyVsBgMode"].toString();
				if (flyVsBgModeStr == "FLY_DARKER_THAN_BG") flyVsBgMode = FLY_DARKER_THAN_BG;
				else if (flyVsBgModeStr == "FLY_BRIGHTER_THAN_BG") flyVsBgMode = FLY_BRIGHTER_THAN_BG;
                else if (flyVsBgModeStr == "FLY_ANY_DIFFERENCE_BG") flyVsBgMode = FLY_ANY_DIFFERENCE_BG;
                else {
					rtnStatus.success = false;
					rtnStatus.appendMessage("unable to parse flyVsBgMode");
				}
			}
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert flyVsBgMode to string");
			}
		}
		return rtnStatus;
	
    }

    RtnStatus FlyTrackConfig::setHeadTailFromMap(QVariantMap configMap) {
		RtnStatus rtnStatus;
        rtnStatus.success = true;
        rtnStatus.message = QString("");
        if (configMap.isEmpty())
        {
			rtnStatus.message = QString("flyTrack headTail config empty");
			return rtnStatus;
		}
        if (configMap.contains("historyBufferLength")) {
            if (configMap["historyBufferLength"].canConvert<int>())
                historyBufferLength = configMap["historyBufferLength"].toInt();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert historyBufferLength to int");
            }
        }
        if (configMap.contains("maxTrackQueueLength")) {
            if (configMap["maxTrackQueueLength"].canConvert<int>())
                maxTrackQueueLength = configMap["maxTrackQueueLength"].toInt();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert maxTrackQueueLength to int");
            }
        }
        if (configMap.contains("minVelocityMagnitude")) {
            if (configMap["minVelocityMagnitude"].canConvert<double>())
                minVelocityMagnitude = configMap["minVelocityMagnitude"].toDouble();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert minVelocityMagnitude to double");
            }
        }
        if (configMap.contains("headTailWeightVelocity")) {
            if (configMap["headTailWeightVelocity"].canConvert<double>())
                headTailWeightVelocity = configMap["headTailWeightVelocity"].toDouble();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert headTailWeightVelocity to double");
            }
        }
        if (configMap.contains("headTailWeightWing")) {
            if (configMap["headTailWeightWing"].canConvert<double>())
                headTailWeightWing = configMap["headTailWeightWing"].toDouble();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert headTailWeightWing to double");
            }
        }
        return rtnStatus;
    }

    RtnStatus FlyTrackConfig::setWingFromMap(QVariantMap configMap) {
        RtnStatus rtnStatus;
        rtnStatus.success = true;
        rtnStatus.message = QString("");
        if (configMap.isEmpty())
        {
            rtnStatus.message = QString("flyTrack wing config empty");
            return rtnStatus;
        }
        if (configMap.contains("trackWings")) {
            if (configMap["trackWings"].canConvert<bool>())
                trackWings = configMap["trackWings"].toBool();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert trackWings to bool"); }
        }
        if (configMap.contains("mindWingHigh")) {
            if (configMap["mindWingHigh"].canConvert<int>())
                mindWingHigh = configMap["mindWingHigh"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert mindWingHigh to int"); }
        }
        if (configMap.contains("mindWingLow")) {
            if (configMap["mindWingLow"].canConvert<int>())
                mindWingLow = configMap["mindWingLow"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert mindWingLow to int"); }
        }
        if (configMap.contains("mindBody")) {
            if (configMap["mindBody"].canConvert<int>())
                mindBody = configMap["mindBody"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert mindBody to int"); }
        }
        if (configMap.contains("maxWingPxAngleDeg")) {
            if (configMap["maxWingPxAngleDeg"].canConvert<double>())
                maxWingPxAngleDeg = configMap["maxWingPxAngleDeg"].toDouble();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert maxWingPxAngleDeg to double"); }
        }
        if (configMap.contains("minNonzeroWingAngleDeg")) {
            if (configMap["minNonzeroWingAngleDeg"].canConvert<double>())
                minNonzeroWingAngleDeg = configMap["minNonzeroWingAngleDeg"].toDouble();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert minNonzeroWingAngleDeg to double"); }
        }
        if (configMap.contains("wingMinPeakDistBins")) {
            if (configMap["wingMinPeakDistBins"].canConvert<int>())
                wingMinPeakDistBins = configMap["wingMinPeakDistBins"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingMinPeakDistBins to int"); }
        }
        if (configMap.contains("wingMinPeakThresholdFrac")) {
            if (configMap["wingMinPeakThresholdFrac"].canConvert<double>())
                wingMinPeakThresholdFrac = configMap["wingMinPeakThresholdFrac"].toDouble();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingMinPeakThresholdFrac to double"); }
        }
        if (configMap.contains("nBinsDThetaWing")) {
            if (configMap["nBinsDThetaWing"].canConvert<int>())
                nBinsDThetaWing = configMap["nBinsDThetaWing"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert nBinsDThetaWing to int"); }
        }
        if (configMap.contains("wingPeakMinFracFactor")) {
            if (configMap["wingPeakMinFracFactor"].canConvert<double>())
                wingPeakMinFracFactor = configMap["wingPeakMinFracFactor"].toDouble();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingPeakMinFracFactor to double"); }
        }
        if (configMap.contains("minSingleWingArea")) {
            if (configMap["minSingleWingArea"].canConvert<int>())
                minSingleWingArea = configMap["minSingleWingArea"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert minSingleWingArea to int"); }
        }
        if (configMap.contains("radiusDilateBody")) {
            if (configMap["radiusDilateBody"].canConvert<int>())
                radiusDilateBody = configMap["radiusDilateBody"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert radiusDilateBody to int"); }
        }
        if (configMap.contains("radiusOpenWing")) {
            if (configMap["radiusOpenWing"].canConvert<int>())
                radiusOpenWing = configMap["radiusOpenWing"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert radiusOpenWing to int"); }
        }
        if (configMap.contains("wingRadiusQuadfitBins")) {
            if (configMap["wingRadiusQuadfitBins"].canConvert<int>())
                wingRadiusQuadfitBins = configMap["wingRadiusQuadfitBins"].toInt();
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingRadiusQuadfitBins to int"); }
        }
        if (configMap.contains("wingFracFilter")) {
            if (configMap["wingFracFilter"].canConvert<QVariantList>()) {
                QVariantList filterList = configMap["wingFracFilter"].toList();
                std::vector<double> filterVec;
                bool ok = true;
                for (int i = 0; i < filterList.size(); i++) {
                    if (filterList[i].canConvert<double>()) filterVec.push_back(filterList[i].toDouble());
                    else { ok = false; break; }
                }
                if (ok && !filterVec.empty()) wingFracFilter = filterVec;
                else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingFracFilter to list of doubles"); }
            }
            else { rtnStatus.success = false; rtnStatus.appendMessage("unable to convert wingFracFilter to list"); }
        }
        return rtnStatus;
    }

    RtnStatus FlyTrackConfig::setMiscFromMap(QVariantMap configMap) {
		RtnStatus rtnStatus;
		rtnStatus.success = true;
		rtnStatus.message = QString("");
        if (configMap.isEmpty())
        {
			rtnStatus.message = QString("flyTrack misc config empty");
			return rtnStatus;
		}
        if (configMap.contains("DEBUG")) {
            if (configMap["DEBUG"].canConvert<bool>())
                DEBUG = configMap["DEBUG"].toBool();
            else {
                rtnStatus.success = false;
                rtnStatus.appendMessage("unable to convert DEBUG to bool");
            }
        }
        if (configMap.contains("tmpOutDir")) {
			if (configMap["tmpOutDir"].canConvert<QString>())
				tmpOutDir = configMap["tmpOutDir"].toString();
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert tmpOutDir to string");
			}
		}
        if (configMap.contains("trackFileName")) {
			if (configMap["trackFileName"].canConvert<QString>())
				trackFileName = configMap["trackFileName"].toString();
            else {
				rtnStatus.success = false;
				rtnStatus.appendMessage("unable to convert trackFileName to string");
			}
		} 
        return rtnStatus;
    }

    QVariantMap FlyTrackConfig::toMap()
    {
        fprintf(stderr, "FlyTrackConfig::toMap\n");
        // Create Device map
        QVariantMap configMap;
        QVariantMap bgEstMap;
        bgEstMap.insert("computeBgMode", computeBgMode);
        bgEstMap.insert("bgImageFilePath", bgImageFilePath);
        bgEstMap.insert("nFramesSkipBgEst", nFramesSkipBgEst);

        QVariantMap roiMap;
        QString roiTypeString;
        roiTypeToString(roiType, roiTypeString);
        roiMap.insert("roiType", roiTypeString);
        roiMap.insert("roiCenterX", roiCenterX);
        roiMap.insert("roiCenterY", roiCenterY);
        roiMap.insert("roiRadius", roiRadius);

        QVariantMap bgSubMap;
        bgSubMap.insert("backgroundThreshold", backgroundThreshold);
        QString flyVsBgModeString;
        flyVsBgModeToString(flyVsBgMode, flyVsBgModeString);
        bgSubMap.insert("flyVsBgMode", flyVsBgModeString);

        QVariantMap headTailMap;
        headTailMap.insert("historyBufferLength", historyBufferLength);
        headTailMap.insert("minVelocityMagnitude", minVelocityMagnitude);
        headTailMap.insert("headTailWeightVelocity", headTailWeightVelocity);
        headTailMap.insert("headTailWeightWing", headTailWeightWing);

        QVariantMap wingMap;
        wingMap.insert("trackWings", trackWings);
        wingMap.insert("mindWingHigh", mindWingHigh);
        wingMap.insert("mindWingLow", mindWingLow);
        wingMap.insert("mindBody", mindBody);
        wingMap.insert("maxWingPxAngleDeg", maxWingPxAngleDeg);
        wingMap.insert("minNonzeroWingAngleDeg", minNonzeroWingAngleDeg);
        wingMap.insert("wingMinPeakDistBins", wingMinPeakDistBins);
        wingMap.insert("wingMinPeakThresholdFrac", wingMinPeakThresholdFrac);
        wingMap.insert("nBinsDThetaWing", nBinsDThetaWing);
        wingMap.insert("wingPeakMinFracFactor", wingPeakMinFracFactor);
        wingMap.insert("minSingleWingArea", minSingleWingArea);
        wingMap.insert("radiusDilateBody", radiusDilateBody);
        wingMap.insert("radiusOpenWing", radiusOpenWing);
        wingMap.insert("wingRadiusQuadfitBins", wingRadiusQuadfitBins);
        QVariantList wingFracFilterList;
        for (size_t i = 0; i < wingFracFilter.size(); i++)
            wingFracFilterList.append(wingFracFilter[i]);
        wingMap.insert("wingFracFilter", wingFracFilterList);

        QVariantMap miscMap;
        miscMap.insert("maxTrackQueueLength", maxTrackQueueLength);
        miscMap.insert("DEBUG", DEBUG);
        miscMap.insert("tmpOutDir", tmpOutDir);
        miscMap.insert("trackFileName", trackFileName);

		configMap.insert("bgEst", bgEstMap);
        configMap.insert("roi", roiMap);
        configMap.insert("bgSub", bgSubMap);
        configMap.insert("headTail", headTailMap);
        configMap.insert("wing", wingMap);
        configMap.insert("misc", miscMap);

        fprintf(stderr,"Done with FlyTrackConfig::toMap\n");

        return configMap;
    }

    RtnStatus FlyTrackConfig::fromJson(QByteArray jsonConfigArray) {
        RtnStatus rtnStatus;
        rtnStatus.success = true;
        rtnStatus.message = QString("");

        bool ok;
        QVariantMap configMap = QtJson::parse(QString(jsonConfigArray), ok).toMap();
        if (!ok)
        {
            rtnStatus.success = false;
            rtnStatus.message = QString("FlyTrack unable to parse json configuration string");
            return rtnStatus;

        }
        rtnStatus = fromMap(configMap);
        return rtnStatus;
    }

    QByteArray FlyTrackConfig::toJson()
    {
        QVariantMap configMap = toMap();

        bool ok;
        QByteArray jsonConfigArray = QtJson::serialize(configMap, ok);
        if (!ok)
        {
            jsonConfigArray = QByteArray();
        }
        return jsonConfigArray;
    }

    // helper functions

    bool roiTypeToString(ROIType roiType, QString& roiTypeString) {
        switch (roiType) {
        case CIRCLE:
            roiTypeString = QString("CIRCLE");
            return true;
        case NONE:
            roiTypeString = QString("NONE");
            return true;
        default:
            return false;
        }
    }
    bool roiTypeFromString(QString roiTypeString, ROIType& roiType) {
        if (roiTypeString == "CIRCLE") {
            roiType = CIRCLE;
            return true;
        }
        if (roiTypeString == "NONE") {
            roiType = NONE;
            return true;
        }
        roiTypeString = "UNKNOWN";
        return false;
    }

    bool flyVsBgModeToString(FlyVsBgModeType flyVsBgMode, QString& flyVsBgModeString) {
        switch (flyVsBgMode) {
        case FLY_DARKER_THAN_BG:
            flyVsBgModeString = QString("FLY_DARKER_THAN_BG");
            return true;
        case FLY_BRIGHTER_THAN_BG:
            flyVsBgModeString = QString("FLY_BRIGHTER_THAN_BG");
            return true;
        case FLY_ANY_DIFFERENCE_BG:
            flyVsBgModeString = QString("FLY_ANY_DIFFERENCE_BG");
            return true;
        default:
            return false;
        }
    }

    bool flyVsBgModeFromString(QString flyVsBgModeString, FlyVsBgModeType& flyVsBgMode) {
        if (flyVsBgModeString == "FLY_DARKER_THAN_BG") {
            flyVsBgMode = FLY_DARKER_THAN_BG;
            return true;
        }
        if (flyVsBgModeString == "FLY_BRIGHTER_THAN_BG") {
            flyVsBgMode = FLY_BRIGHTER_THAN_BG;
            return true;
        }
        if (flyVsBgModeString == "FLY_ANY_DIFFERENCE_BG") {
            flyVsBgMode = FLY_ANY_DIFFERENCE_BG;
            return true;
        }
        flyVsBgModeString = "UNKNOWN";
        return false;
    }

    void FlyTrackConfig::setRoiParams(ROIType roiTypeNew, double roiCenterXNew, double roiCenterYNew, double roiRadiusNew) {
        roiType = roiTypeNew;
        roiCenterX = roiCenterXNew;
        roiCenterY = roiCenterYNew;
        roiRadius = roiRadiusNew;
    }


}