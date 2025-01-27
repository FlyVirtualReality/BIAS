#include "grab_detector_plugin.hpp"
#include "image_label.hpp"
#include <QtDebug>
#include <QTimer>
#include <QMessageBox>
#include <QThread>
//#include <cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <sstream>
#include <iostream>

namespace bias
{
    // Public static variables 
    // ------------------------------------------------------------------------
    const QString GrabDetectorPlugin::PLUGIN_NAME = QString("grabDetector");
    const QString GrabDetectorPlugin::PLUGIN_DISPLAY_NAME = QString("Grab Detector");
    int GrabDetectorPlugin::DEFAULT_LIVEPLOT_UPDATE_DT = 75;
    double GrabDetectorPlugin::DEFAULT_LIVEPLOT_TIME_WINDOW = 10.0; 
    double GrabDetectorPlugin::DEFAULT_LIVEPLOT_SIGNAL_WINDOW = 255.0;
    const QString GrabDetectorPlugin::LOG_FILE_EXTENSION = QString("txt");
    const QString GrabDetectorPlugin::LOG_FILE_POSTFIX = QString("grab_detector_log");


    // Public Methods
    // ------------------------------------------------------------------------
    
    GrabDetectorPlugin::GrabDetectorPlugin(ImageLabel *imageLabelPtr, QWidget *parentPtr) : BiasPlugin(parentPtr)
    {
        imageLabelPtr_ = imageLabelPtr;
        setupUi(this);
        connectWidgets();
        initialize();
    }

    void GrabDetectorPlugin::reset()
    {
        openLogFile();
    }


    void GrabDetectorPlugin::stop()
    {
        closeLogFile();
    }


    void GrabDetectorPlugin::processFrames(QList<StampedImage> frameList)
    {
        // --------------------------------------------------------------
        // NOTE: called in separate thread.
        // --------------------------------------------------------------
        

        int medianFilterSize = getMedianFilter();
        int threshold = getThreshold();
        bool inverted = getInverted();
        bool found = false;
        double signalMin; 
        double signalMax;

        int frameListSize = frameList.size();

        StampedImage latestFrame = frameList.back();
        frameList.clear();

        cv::Mat workingImage = latestFrame.image.clone();
        if ((workingImage.rows != 0) && (workingImage.cols != 0))
        {
            cv::Rect boxRect = getDetectionBoxCv();
            cv::Mat roiImage = workingImage(boxRect);
            cv::medianBlur(roiImage,roiImage,medianFilterSize);
            cv::minMaxLoc(roiImage,&signalMin,&signalMax);

            bool thresholdTest = false;
            if (inverted)
            {
                thresholdTest = signalMax < double(threshold);
            }
            else
            {
                thresholdTest = signalMax > double(threshold);
            }

            if (thresholdTest)
            {
                found = true;
            }
            else
            { 
                found = false;
            }

            acquireLock();
            currentImage_ = workingImage;
            signalMin_ = signalMin;
            signalMax_ = signalMax;
            found_ = found;
            frameCount_ = latestFrame.frameCount;
            livePlotTimeVec_.append(latestFrame.timeStamp);
            livePlotSignalVec_.append(signalMax);
            if (found && config_.triggerArmedState)
            {
                if (config_.triggerEnabled)
                {
                    TriggerData triggerData;
                    triggerData.frameCount = latestFrame.frameCount;
                    triggerData.timeStamp = latestFrame.timeStamp;
                    triggerData.threshold = double(threshold);
                    triggerData.signal = signalMax;
                    emit triggerFired(triggerData);
                }
            }
            releaseLock();
        }
    }

    cv::Mat GrabDetectorPlugin::getCurrentImage()
    {
        acquireLock();
        cv::Mat currentImage = currentImage_;
        int signalMin = signalMin_;
        int signalMax = signalMax_;
        bool found = found_;
        int frameCount = frameCount_;
        releaseLock();

        cv::Rect boxRect = getDetectionBoxCv();
        int red = config_.detectBoxColor.red();
        int green = config_.detectBoxColor.green();
        int blue = config_.detectBoxColor.blue();
        cv::Scalar boxColor(blue,green,red);
        int boxLineWidth = 2;

        std::stringstream foundStream;
        if (found)
        {
            foundStream << "object found";
        }
        cv::Mat currentImageBGR;
        //cv::cvtColor(currentImage, currentImageBGR, CV_GRAY2BGR);
        cv::cvtColor(currentImage, currentImageBGR, cv::COLOR_GRAY2BGR);
        cv::rectangle(currentImageBGR, boxRect,boxColor, boxLineWidth);

        double fontScale = 1.0;
        int thickness = 2;
        int baseline = 0;


        //cv::Size textSize = cv::getTextSize(foundStream.str(), CV_FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);
        cv::Size textSize = cv::getTextSize(foundStream.str(), cv::FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);
        cv::Point textPoint(currentImage.cols/2 - textSize.width/2, textSize.height+baseline);
        //cv::putText(currentImageBGR, foundStream.str(), textPoint, CV_FONT_HERSHEY_SIMPLEX, fontScale, boxColor,thickness);
        cv::putText(currentImageBGR, foundStream.str(), textPoint, cv::FONT_HERSHEY_SIMPLEX, fontScale, boxColor,thickness);
        return currentImageBGR;
    }


    cv::Rect GrabDetectorPlugin::getDetectionBoxCv()
    {
        QRect box = getDetectionBox();
        cv::Rect boxCv(box.x(), box.y(), box.width(), box.height());
        return boxCv;
    }

    QRect GrabDetectorPlugin::getDetectionBox()
    {
        QRect box = QRect( 
                xPosSpinBoxPtr   -> value(),
                yPosSpinBoxPtr   -> value(),
                widthSpinBoxPtr  -> value(),
                heightSpinBoxPtr -> value()
                );

        return box;
    }

    void GrabDetectorPlugin::setDetectionBox(QRect box)
    {
        xPosSpinBoxPtr -> setValue(box.x());
        yPosSpinBoxPtr -> setValue(box.y());
        widthSpinBoxPtr -> setValue(box.width());
        heightSpinBoxPtr -> setValue(box.height());
    }


    bool GrabDetectorPlugin::isDetectionBoxLocked()
    {
        return !(boxLockedCheckboxPtr -> isChecked());
    }


    int GrabDetectorPlugin::getThreshold()
    {
        return trigThresholdSpinBoxPtr -> value();
    }


    int GrabDetectorPlugin::getMedianFilter()
    {
        return trigMedianFilterSpinBoxPtr -> value();
    }


    bool GrabDetectorPlugin::getInverted()
    {
        Qt::CheckState checkState = trigInvertedCheckBoxPtr -> checkState();
        if (checkState == Qt::Checked)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void GrabDetectorPlugin::setTriggerEnabled(bool value)
    {
        config_.triggerEnabled = value;
        trigEnabledCheckBoxPtr -> setChecked(value);

    }

    QString GrabDetectorPlugin::getName()
    {
        return PLUGIN_NAME;
    }


    QString GrabDetectorPlugin::getDisplayName()
    {
        return PLUGIN_DISPLAY_NAME;
    }

    RtnStatus GrabDetectorPlugin::runCmdFromMap(QVariantMap cmdMap, bool showErrorDlg, QString& runCmdFromMap)
    {
        RtnStatus rtnStatus;
        rtnStatus.success = true;
        rtnStatus.message = QString("");


        QString errMsgTitle("Plugin runCmdFromMap Error");

        if (!cmdMap.contains("cmd"))
        {
            QString errMsgText("cmd not found in map");
            if (showErrorDlg)
            {
                QMessageBox::critical(this,errMsgTitle,errMsgText);
            }
            rtnStatus.success = false;
            rtnStatus.message = errMsgText;
            return rtnStatus;
        }
        if (!cmdMap["cmd"].canConvert<QString>())
        {
            QString errMsgText("runPluginCmd: unable to convert plugin name to string");
            if (showErrorDlg)
            {
                QMessageBox::critical(this,errMsgTitle,errMsgText);
            }
            rtnStatus.success = false;
            rtnStatus.message = errMsgText;
            return rtnStatus;
        }
        QString cmd  = cmdMap["cmd"].toString();

        if (cmd == QString("reset"))
        {
            resetTrigger();
        }
        else if (cmd == QString("connect"))
        {
            rtnStatus = connectTriggerDev();
        }
        else if (cmd == QString("disconnect"))
        {
            rtnStatus = disconnectTriggerDev();
        }
        else if (cmd == QString("set-config"))
        {
            QVariantMap configMap = cmdMap["config"].toMap();
            if (!configMap.isEmpty())
            {
                rtnStatus = setConfigFromMap(configMap);
            }
            else
            {
                rtnStatus.message = QString("configuration map is empty");
                rtnStatus.success = false;
            }
        }
        else
        {
            QString errMsgText = QString("runPluginCmd: unknown cmd %1").arg(cmd);
            if (showErrorDlg)
            {
                QMessageBox::critical(this,errMsgTitle,errMsgText);
            }
            rtnStatus.success = false;
            rtnStatus.message = errMsgText;
        }

        return rtnStatus;
    }


    QVariantMap GrabDetectorPlugin::getConfigAsMap()   
    {
        QVariantMap configMap = config_.toMap();
        return configMap;
    } 

    RtnStatus GrabDetectorPlugin::setConfigFromMap(QVariantMap configMap)
    {
        GrabDetectorConfig config;
        RtnStatus rtnStatus = config.fromMap(configMap);
        if (rtnStatus.success)
        {
            rtnStatus = setFromConfig(config);
        }
        return rtnStatus;
    } 


    RtnStatus GrabDetectorPlugin::setConfigFromJson(QByteArray jsonArray)
    {
        GrabDetectorConfig config;
        RtnStatus rtnStatus = config.fromJson(jsonArray); 
        if (rtnStatus.success)
        {
            rtnStatus = setFromConfig(config);
        }
        return rtnStatus;
    }

    QString GrabDetectorPlugin::getLogFileExtension()
    {
        return LOG_FILE_EXTENSION;
    }


    QString GrabDetectorPlugin::getLogFilePostfix()
    {
        return LOG_FILE_POSTFIX;
    }


    void GrabDetectorPlugin::resetTrigger()
    {
        config_.triggerArmedState = true;
        updateTrigStateInfo();
    }


    RtnStatus GrabDetectorPlugin::connectTriggerDev()
    {
        RtnStatus rtnStatus;

        tabWidgetPtr -> setEnabled(false);
        tabWidgetPtr -> repaint(); 

        if (pulseDevice_.isOpen())
        {
            tabWidgetPtr -> setEnabled(true);
            rtnStatus.success = true;
            rtnStatus.message = QString("device already connected");
            return rtnStatus;
        }
        
        statusLabelPtr -> setText(QString("Status: connecting ..."));
        statusLabelPtr -> repaint();
        if (serialInfoList_.size() > 0)
        {
            int index = comPortComboBoxPtr -> currentIndex();
            QSerialPortInfo serialInfo = serialInfoList_.at(index);
            pulseDevice_.setPort(serialInfo);
            pulseDevice_.open();
        }

        // Check to see if device is opene or closed and set status string accordingly
        if (pulseDevice_.isOpen())
        {
            statusLabelPtr -> setText(QString("Status: connected"));
            connectPushButtonPtr -> setText("Disconnect");
            refreshPortListPushButtonPtr -> setEnabled(false);
            comPortComboBoxPtr -> setEnabled(false);
            devOutputGroupBoxPtr -> setEnabled(true);

            // Get list of allowed output pins
            bool ok;
            QVector<int> allowedOutputPin = pulseDevice_.getAllowedOutputPin(&ok);
            if (ok)
            {
                allowedOutputPin_ = allowedOutputPin;
                for (int i=0; i<allowedOutputPin_.size(); i++)
                {
                    int pin = allowedOutputPin_[i];
                    outputPinComboBoxPtr -> addItem(QString::number(pin));
                }
                outputPinComboBoxReady_ = true;
            }

            // Set output pin
            bool pinAllowed = false;
            int pinIndex = -1;

            for (int i=0; i<allowedOutputPin_.size(); i++)
            {
                if (config_.outputPin == allowedOutputPin_[i])
                {
                    pinAllowed = true;
                    pinIndex = i;
                    break;
                }
            }

            ok = false;
            if (pinAllowed)
            {
                outputPinComboBoxPtr -> setCurrentIndex(pinIndex);
                ok = pulseDevice_.setOutputPin(config_.outputPin);
            }
            else
            {
                if (!allowedOutputPin_.isEmpty())
                {
                    outputPinComboBoxPtr -> setCurrentIndex(0);
                    config_.outputPin = allowedOutputPin_[0];
                    ok = pulseDevice_.setOutputPin(config_.outputPin);
                }
            }


            // If bad response or unable to set output pin 
            if (!ok)
            {
                allowedOutputPin_.clear();
                config_.outputPin = -1;
                outputPinComboBoxPtr -> clear();
                outputPinComboBoxReady_ = false;
            }

            // Set pulse length 
            ok =  pulseDevice_.setPulseLength(1.0e6*config_.devicePulseDuration);
            if (ok)
            {
                durationDblSpinBoxPtr -> setValue(config_.devicePulseDuration);
            }
        }
        else
        {
            statusLabelPtr -> setText(QString("Status: not connected"));
            connectPushButtonPtr -> setText("Connect");
            refreshPortListPushButtonPtr -> setEnabled(true);
            comPortComboBoxPtr -> setEnabled(true);
            devOutputGroupBoxPtr -> setEnabled(false);

            tabWidgetPtr -> setEnabled(true);
            rtnStatus.success = false;
            rtnStatus.message = QString("failed to open device");
            return rtnStatus;
        }


        tabWidgetPtr -> setEnabled(true);
        rtnStatus.success = true;
        rtnStatus.message = QString("");
        return rtnStatus;
    }


    RtnStatus GrabDetectorPlugin::disconnectTriggerDev()
    {
        RtnStatus rtnStatus;
        tabWidgetPtr -> setEnabled(false);
        tabWidgetPtr -> repaint(); 

        if (pulseDevice_.isOpen())
        {
            statusLabelPtr -> setText(QString("Status: disconnecting ... "));
            statusLabelPtr -> repaint();
            pulseDevice_.close();
        }
        else
        {
            tabWidgetPtr -> setEnabled(true);
            rtnStatus.success = true;
            rtnStatus.message = QString("device already disconnecter");
            return rtnStatus;
        }

        allowedOutputPin_.clear();

        statusLabelPtr -> setText(QString("Status: not connected"));
        connectPushButtonPtr -> setText("Connect");
        refreshPortListPushButtonPtr -> setEnabled(true);
        comPortComboBoxPtr -> setEnabled(true);
        devOutputGroupBoxPtr -> setEnabled(false);
        outputPinComboBoxPtr -> clear();
        outputPinComboBoxReady_ = false;

        tabWidgetPtr -> setEnabled(true);
        rtnStatus.success = true;
        rtnStatus.message = QString("");
        return rtnStatus;
    }


    RtnStatus GrabDetectorPlugin::setFromConfig(GrabDetectorConfig config)
    {
        RtnStatus rtnStatus;

        GrabDetectorConfig oldConfig = config_;
        config_= config;

        bool reconnect = false;
        if (pulseDevice_.isOpen())
        {
            disconnectTriggerDev();
            reconnect = true;
        }

        durationDblSpinBoxPtr -> setValue(config_.devicePulseDuration);
        autoConnectCheckBoxPtr -> setChecked(config_.deviceAutoConnect);

        xPosSpinBoxPtr -> setValue(config_.detectBoxXPos);
        yPosSpinBoxPtr -> setValue(config_.detectBoxYPos);
        widthSpinBoxPtr -> setValue(config_.detectBoxWidth);
        heightSpinBoxPtr -> setValue(config_.detectBoxHeight);

        setTriggerEnabled(config_.triggerEnabled);
        trigThresholdSpinBoxPtr -> setValue(config_.triggerThreshold);
        trigMedianFilterSpinBoxPtr ->  setValue(config_.triggerMedianFilter);
        trigInvertedCheckBoxPtr -> setChecked(config_.triggerInverted);

        updateColorExampleLabel();
        updateTrigStateInfo();

        bool portFound = false;
        QSerialPortInfo portInfo;
        refreshPortList();

        for (QSerialPortInfo serialInfo : serialInfoList_)
        {
            if (config_.devicePortName == serialInfo.portName())
            {
                portFound = true;
                portInfo = serialInfo;
                break;
            }
        }

        if (portFound)
        {
            int portIndex = comPortComboBoxPtr -> findText(config_.devicePortName); 
            comPortComboBoxPtr -> setCurrentIndex(portIndex);
            if ( (config_.deviceAutoConnect) || reconnect)
            {
                if (reconnect)
                {
                    QThread::msleep(1000);
                }
                rtnStatus = connectTriggerDev();

                if (!rtnStatus.success)
                {
                    rtnStatus.success = false;
                    rtnStatus.appendMessage(QString("unable to connect to port %1").arg(config_.devicePortName));
                }
            }
        }
        else
        {
            rtnStatus.success = false;
            rtnStatus.appendMessage(QString("port %1 not found").arg(config_.devicePortName));
        }

        if (!pulseDevice_.isOpen())
        {
            statusLabelPtr -> setText(QString("Status: not connected "));
            devOutputGroupBoxPtr -> setEnabled(false);
        }

        return rtnStatus;
    }



    // Protected Methods
    // ------------------------------------------------------------------------
    
    void GrabDetectorPlugin::connectWidgets()
    {
        connect(
                comPortComboBoxPtr,
                SIGNAL(currentIndexChanged(QString)),
                this,
                SLOT(comPortComboBoxIndexChanged(QString))
                );

        connect(
                connectPushButtonPtr,
                SIGNAL(clicked()),
                this,
                SLOT(connectPushButtonClicked())
               );

        connect(
                refreshPortListPushButtonPtr,
                SIGNAL(clicked()),
                this,
                SLOT(refreshPortListPushButtonClicked())
               );

        connect(
                outputTestPushButtonPtr,
                SIGNAL(clicked()),
                this,
                SLOT(outputTestPushButtonClicked())
               );

        connect(
                outputPinComboBoxPtr,
                SIGNAL(currentIndexChanged(int)),
                this,
                SLOT(outputPinComboBoxIndexChanged(int))
               );

        connect(
                durationDblSpinBoxPtr,
                SIGNAL(valueChanged(double)),
                this,
                SLOT(durationDblSpinBoxValueChanged(double))
               );

        connect(
                colorSelectPushButtonPtr,
                SIGNAL(clicked()),
                this,
                SLOT(colorSelectPushButtonClicked())
               );

        connect(
                trigResetPushButtonPtr,
                SIGNAL(clicked()),
                this,
                SLOT(trigResetPushButtonClicked())
               );

        connect(
                trigEnabledCheckBoxPtr,
                SIGNAL(stateChanged(int)),
                this,
                SLOT(trigEnabledCheckBoxStateChanged(int))
               );

        connect(
                trigThresholdSpinBoxPtr,
                SIGNAL(valueChanged(int)),
                this,
                SLOT(trigThresholdSpinBoxChanged(int))
               );

        connect(
                trigMedianFilterSpinBoxPtr,
                SIGNAL(valueChanged(int)),
                this,
                SLOT(trigMedianFilterSpinBoxChanged(int))
               );

        connect(
                imageLabelPtr_,
                SIGNAL(selectBoxChanged(QRect)),
                this,
                SLOT(detectionBoxChanged(QRect))
               );

        qRegisterMetaType<TriggerData>("TriggerData");

        connect(
                this,
                SIGNAL(triggerFired(TriggerData)),
                this,
                SLOT(onTriggerFired(TriggerData))
               );

    }


    void GrabDetectorPlugin::initialize()
    {
        found_ = false;
        signalMax_ = 0.0;
        signalMin_ = 0.0;
        frameCount_ = 0;

        livePlotUpdateDt_ = DEFAULT_LIVEPLOT_UPDATE_DT;
        livePlotTimeWindow_ = DEFAULT_LIVEPLOT_TIME_WINDOW;
        livePlotSignalWindow_ = DEFAULT_LIVEPLOT_SIGNAL_WINDOW;

        // Setup live plot
        livePlotPtr -> addGraph();
        livePlotPtr -> addGraph();
        livePlotPtr -> graph(0) -> setPen(QPen(QColor(0,0,255,255),1.5));
        livePlotPtr -> graph(1) -> setPen(QPen(QColor(100,100,100,255),2.0));
        livePlotPtr -> xAxis -> setRange(-livePlotTimeWindow_,0);
        livePlotPtr -> yAxis -> setRange(0,livePlotSignalWindow_);
        livePlotPtr -> xAxis -> setLabel("time (sec)");
        livePlotPtr -> replot();

        // Setup plot update timer
        livePlotUpdateTimerPtr_ = new QTimer(this);
        connect(livePlotUpdateTimerPtr_, SIGNAL(timeout()), this, SLOT(updateLivePlotOnTimer()));
        livePlotUpdateTimerPtr_ -> start(livePlotUpdateDt_);

        refreshPortList();
        allowedOutputPin_.clear();
        outputPinComboBoxReady_ = false;

        setFromConfig(config_);
        setRequireTimer(false);

    }

    void GrabDetectorPlugin::updateTrigStateInfo()
    {
        if (config_.triggerArmedState)
        {
            trigStateLabelPtr -> setText("State: Ready");
        }
        else
        {
            trigStateLabelPtr -> setText("State: Stopped");
        }

        if (trigEnabledCheckBoxPtr -> isChecked())
        {
            trigStateLabelPtr -> setEnabled(true);
            trigResetPushButtonPtr -> setEnabled(true);
        }
        else
        {
            trigStateLabelPtr -> setEnabled(false);
            trigResetPushButtonPtr -> setEnabled(false);
        }
    }


    void GrabDetectorPlugin::refreshPortList()
    {
        comPortComboBoxPtr -> clear();
        serialInfoList_.clear();

        // Get list of serial ports and populate comports 
        QList<QSerialPortInfo> serialInfoListTmp = QSerialPortInfo::availablePorts();

        for (QSerialPortInfo serialInfo: serialInfoListTmp)
        {
            if (serialInfo.portName().contains("ttyS"))
            {
                continue;
            }
            else
            {
                serialInfoList_.append(serialInfo);
            }
        }

        for (QSerialPortInfo serialInfo : serialInfoList_)
        {
            comPortComboBoxPtr -> addItem(serialInfo.portName());
        }
    }


    void GrabDetectorPlugin::updateColorExampleLabel()
    { 
        QPalette palette = colorExampleLabelPtr -> palette();
        palette.setColor(colorExampleLabelPtr -> backgroundRole(), config_.detectBoxColor);
        colorExampleLabelPtr -> setPalette(palette);
        colorExampleLabelPtr -> setAutoFillBackground(true);
    }


    void GrabDetectorPlugin::writeLogData(TriggerData data)
    {
        logStream_ << data.frameCount << " " << data.timeStamp << " " << data.threshold << " " << data.signal << '\n';
    }


    // Private Slots
    // ------------------------------------------------------------------------
    
    void GrabDetectorPlugin::comPortComboBoxIndexChanged(QString text)
    {
        if (!text.isEmpty())
        {
            config_.devicePortName = text;
        }
    }


    void GrabDetectorPlugin::connectPushButtonClicked()
    {
        if (pulseDevice_.isOpen())
        {
            disconnectTriggerDev();
        }
        else
        {
            connectTriggerDev();
        }
    }


    void GrabDetectorPlugin::refreshPortListPushButtonClicked()
    {
        refreshPortList();
    }


    void GrabDetectorPlugin::outputTestPushButtonClicked()
    {
        if (pulseDevice_.isOpen())
        {
            bool ok = pulseDevice_.startPulse(); 
            if (!ok)
            {
                QString msgTitle("PulseDevice Error");
                QString msgText("Error sending command to device");
                QMessageBox::warning(this,msgTitle,msgText);
            }
        }
    }


    void GrabDetectorPlugin::outputPinComboBoxIndexChanged(int index)
    {
        if (pulseDevice_.isOpen() && (index >= 0) && outputPinComboBoxReady_ )
        {
            int newOutputPin = allowedOutputPin_[index];
            if (config_.outputPin != newOutputPin)
            {
                bool ok = pulseDevice_.setOutputPin(newOutputPin);
                if (ok)
                {
                    config_.outputPin = newOutputPin;
                } 
            }
        }
    }


    void GrabDetectorPlugin::durationDblSpinBoxValueChanged(double value)
    {
        if (pulseDevice_.isOpen())
        {
            unsigned long pulseLength_us = (unsigned long)(value*1.0e6);
            bool ok = pulseDevice_.setPulseLength(pulseLength_us);
        }
    }


    void GrabDetectorPlugin::colorSelectPushButtonClicked()
    {
        QColor newColor = QColorDialog::getColor(config_.detectBoxColor, this);
        if (newColor.isValid())
        {
            config_.detectBoxColor = newColor;
        }
        updateColorExampleLabel();
    }


    void GrabDetectorPlugin::trigResetPushButtonClicked()
    {
        resetTrigger();
    }


    void GrabDetectorPlugin::trigEnabledCheckBoxStateChanged(int state)
    {
        if (state == Qt::Unchecked)
        {
            config_.triggerEnabled = false;
        }
        else
        {
            config_.triggerEnabled= true;
        }
        updateTrigStateInfo();
    }


    void GrabDetectorPlugin::trigThresholdSpinBoxChanged(int value)
    {
        config_.triggerThreshold = value;
    }

    void GrabDetectorPlugin::trigMedianFilterSpinBoxChanged(int value)
    {
        config_.triggerMedianFilter = value;
    }


    void GrabDetectorPlugin::detectionBoxChanged(QRect box)
    {
        if (isActive() && pluginsEnabled() && !isDetectionBoxLocked())
        {
            setDetectionBox(box);
        }
    }

    void GrabDetectorPlugin::updateLivePlotOnTimer()
    {
        if (livePlotTimeVec_.empty())
        {
            return;
        }

        acquireLock();
        double lastTime = livePlotTimeVec_.last();
        double firstTime = livePlotTimeVec_.first();

        if (lastTime < firstTime)
        {
            livePlotTimeVec_.clear();
            livePlotSignalVec_.clear();
            releaseLock();
            return;
        }

        while (lastTime - firstTime > livePlotTimeWindow_)
        {
            livePlotTimeVec_.pop_front();
            livePlotSignalVec_.pop_front();
            firstTime = livePlotTimeVec_.first();
        }

        double threshold = double(getThreshold());
        QVector<double> threshSignalVec = {threshold, threshold};
        QVector<double> threshTimeVec;
        if (lastTime < livePlotTimeWindow_)
        {
            double windowStartTime= -livePlotTimeWindow_ + lastTime;
            livePlotPtr -> xAxis -> setRange(windowStartTime,lastTime);
            threshTimeVec = QVector<double>({windowStartTime, lastTime});

        }
        else
        {
            livePlotPtr -> xAxis -> setRange(firstTime, lastTime);
            threshTimeVec = QVector<double>({firstTime, lastTime});
        }

        livePlotPtr -> graph(0) -> setData(livePlotTimeVec_,livePlotSignalVec_);

        livePlotPtr -> graph(1) -> setData(threshTimeVec, threshSignalVec);
        livePlotPtr -> replot();
        releaseLock();

    }

    void GrabDetectorPlugin::onTriggerFired(TriggerData data)
    {
        if (config_.triggerArmedState)
        {
            if (pulseDevice_.isOpen())
            {
                pulseDevice_.startPulse();
            }
            config_.triggerArmedState = false;
            if (loggingEnabled_)
            {
                writeLogData(data);
            }
            updateTrigStateInfo();
        }
    }
}
