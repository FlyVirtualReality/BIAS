#ifndef FLYTRACK_PLUGIN_HPP
#define FLYTRACK_PLUGIN_HPP
#include "ui_flytrack_plugin.h"
#include <QDialog>
#include <QWidget>
#include <QList>
#include "bias_plugin.hpp"
#include "stamped_image.hpp"
#include "background_data_ufmf.hpp"
#include "rtn_status.hpp"
#include <QDir>
#include <QTextStream>
#include <QProgressBar> // progress bar obsolete, but function is still there
#include "flytrack_config.hpp"

namespace cv
{
    class Mat;
}

namespace bias
{
    class CameraWindow;

    struct EllipseParams
    {
        int frame;
        double x;
        double y;
        double a;
        double b;
        double theta;
    };

    // helper functions
    bool loadBackgroundModel(QString bgImageFilePath, cv::Mat& bgMedianImage);
    void computeBackgroundMedian(QString bgVideoFilePath, int nFramesBgEst, 
    int lastFrameSample,cv::Mat& bgMedianImage,QProgressBar* progressBar);
    int largestConnectedComponent(cv::Mat& isFg);
    void fitEllipse(cv::Mat& isFg, EllipseParams& flyEllipse);
    double mod2pi(double angle);
    QString ellipseToJson(EllipseParams ell);
    QString fishStatusToJson(bool trigger);
    bool checkFileExists(QString file);


    class FlyTrackPlugin : public BiasPlugin, public Ui::FlyTrackPluginDialog
    {
        Q_OBJECT

        public:

            static const QString PLUGIN_NAME;
            static const QString PLUGIN_DISPLAY_NAME;
            static const QString LOG_FILE_EXTENSION;
            static const QString LOG_FILE_POSTFIX;
            static const int LOGGING_PRECISION;
            static const unsigned int BG_HIST_NUM_BINS;
            static const unsigned int BG_HIST_BIN_SIZE;
            static const double MIN_VEL_MATCH_DOTPROD; // minimum dot product for velocity matching
	        static const cv::Rect ROI;
	        static const unsigned int fish_detect_intensity_threshold; // Maximum pixel intensity that defines a fish
	        static const unsigned int fish_detect_pixel_threshold; // Minimum pixel count that detects a fish outside the ROI
            static const unsigned int fish_size_threshold; // Minimum pixel count that identifies a blob as a fish
            bool trigger_pulsed; // Flag making sure trigger is pulsed only in the first frame after a fish leaves the ROI. Refresh after all fish return to ROI
            bool trigger;
            bool has_triggered; // Have all the fish entered the ROI since last resetting
	        //static const unsigned int ROI_x;
	        //static const unsigned int ROI_y;
	        //static const unsigned int ROI_width;
	        //static const unsigned int ROI_height;

    	FlyTrackPlugin(QWidget *parent=0);
            bool pluginsEnabled();
            void setPluginsEnabled(bool value);
            void getUiValues(FlyTrackConfig &config);
            void getUiBgEstValues(FlyTrackConfig& config);
            void getUiRoiValues(FlyTrackConfig& config);
            void processFramesTrackMode(QList<StampedImage> frameList);
            void processFramesBgEstMode(QList<StampedImage> frameList);
            void getCurrentImageTrackMode(cv::Mat& currentImageCopy);
            void getCurrentImageComputeBgMode(cv::Mat& currentImageCopy);
            RtnStatus popFrontTrack(EllipseParams& ell);
            RtnStatus popBackTrack(EllipseParams& ell);
            RtnStatus getLastClearTrack(EllipseParams& ell);
            RtnStatus getArenaParams(EllipseParams& ell);

            bool scanFishOutsideROI(cv::Mat& isFg, cv::Rect ROI);
            bool detectFishInsideROI(cv::Mat& isFg, cv::Rect ROI);

            QPointer<CameraWindow> getCameraWindow();

            virtual void reset();
            virtual void stop();
            virtual void setActive(bool value);
            virtual void processFrames(QList<StampedImage> frameList);
            virtual void setFileAutoNamingString(QString autoNamingString);
            virtual void setFileVersionNumber(unsigned verNum);
            virtual cv::Mat getCurrentImage();
            virtual QString getName();
            virtual QString getDisplayName();
            virtual QVariantMap getConfigAsMap();  
            RtnStatus setFromConfig(FlyTrackConfig config);
            virtual RtnStatus setConfigFromMap(QVariantMap configMap);
            virtual RtnStatus setConfigFromJson(QByteArray jsonArray);
            virtual RtnStatus runCmdFromMap(QVariantMap cmdMap, bool showErrorDlg=true, QString& value = QString(""));
            virtual QString getLogFileExtension();
            virtual QString getLogFilePostfix();
            virtual QString getLogFileName(bool includeAutoNaming);
            virtual QString getLogFileFullPath(bool includeAutoNaming);
            virtual void showEvent(QShowEvent *event);

        signals:

            void setCaptureDurationRequest(unsigned long);

        protected:

            QWidget *parent_;

            void initialize();
            void initializeUi();
            void setUiEnabled();
            void setRoiUIValues();
            void connectWidgets();
            bool setBgImageFilePath(QString newBgImageFilePath);
            bool saveBgMedianImage(cv::Mat bgMedianImage, QString bgImageFilePath);
            void setPreviewImage(cv::Mat matImage, FlyTrackConfig config);

            //void setBackgroundModel();
            void setBackgroundModel(cv::Mat& bgMedianImage, FlyTrackConfig& config);
            cv::Mat circleROI(double centerX, double centerY, double centerRadius);
            cv::Mat rectangleROI(double centerX, double centerY, double width, double height);
            void backgroundSubtraction();
            void setROI(FlyTrackConfig config);
            void updateVelocityHistory();
            void updateOrientationHistory();
            void updateEllipseHistory();
            void resolveHeadTail();
            void flipFlyOrientationHistory();
            void logCurrentFrame();
            void finishComputeBgMode();

            bool active_;
            bool requireTimer_;
            cv::Mat currentImage_;

            double timeStamp_;
            unsigned long frameCount_;

            QString fileAutoNamingString_;
            unsigned int fileVersionNumber_;

            QDir logFileDir_;
            bool loggingEnabled_;
            QFile logFile_;
            QTextStream logStream_;

            // parameters
            FlyTrackConfig config_; 

            // background model
            cv::Mat bgMedianImage_; // median background image
            cv::Mat bgLowerBoundImage_; // lower bound image for background
            cv::Mat bgUpperBoundImage_; // upper bound image for background
			bool bgImageComputed_; // flag indicating if background image has been computed

            BackgroundData_ufmf backgroundData_; // background estimation data
            int lastFrameAdded_; // last frame added to background model
            int nFramesAddedBgEst_; // number of frames added to background model so far

			// processing of current frame
            bool isFirst_; // flag indicating if this is the first frame
            cv::Mat isFg_; // foreground mask
            cv::Mat inROI_; // mask for ROI
            EllipseParams flyEllipse_; // fly ellipse parameters
            int lastFramePreviewed_; // last frame shown in preview window
            int lastFrameMedianComputed_; // last frame median computed
            cv::Mat lastImagePreviewed_; // last image shown in preview window

            // tracking history
            std::shared_ptr<LockableDeque<EllipseParams>> flyEllipseDequePtr_; // queue for serving tracking
            std::vector<EllipseParams> flyEllipseHistory_; // tracked ellipses
            std::deque<cv::Point2d> velocityHistory_; // tracked velocity buffer
            std::deque<double> orientationHistory_; // tracked orientation buffer
            cv::Point2d meanFlyVelocity_; // mean velocity of fly
            double meanFlyOrientation_; // mean orientation of fly
            bool headTailResolved_; // flag indicating if head-tail orientation has been resolved ever

            // for writing images
            std::vector<int> imwriteParams_;

            void setRequireTimer(bool value);
            void openLogFile();
            void closeLogFile();

        private slots:

            void applyPushButtonClicked();
            void donePushButtonClicked();
            void cancelPushButtonClicked();

            void loadBgPushButtonClicked();
            void roiUiChanged(int v);
            void bgImageFilePathToolButtonClicked();
            void logFilePathToolButtonClicked();
            void tmpOutDirToolButtonClicked();
            void computeBgModeComboBoxChanged();

    };

}


#endif 


