#include "camera_window.hpp"
#include "camera_facade.hpp"
#include "mat_to_qimage.hpp"
#include "stamped_image.hpp"
#include "lockable.hpp"
#include "lockable_queue.hpp"
#include "image_grabber.hpp"
#include "image_dispatcher.hpp"
#include <cstdlib>
#include <cmath>
#include <QtGui>
#include <QTimer>
#include <QThreadPool>
#include <opencv2/core/core.hpp>
#include <iostream>

namespace bias
{

    // Public methods
    // ----------------------------------------------------------------------------------

    CameraWindow::CameraWindow(Guid cameraGuid, QWidget *parent) : QMainWindow(parent)
    {
        setupUi(this);
        connectWidgets();
        initialize(cameraGuid);
    }

    // Protected methods
    // ----------------------------------------------------------------------------------

    void CameraWindow::resizeEvent(QResizeEvent *event)
    {
        if (havePixmap_ && !(pixmapOriginal_.isNull()))  
        {
            QSize sizeImageLabel = imageLabelPtr_ -> size();
            QSize sizeAdjusted = pixmapOriginal_.size();
            sizeAdjusted.scale(sizeImageLabel, Qt::KeepAspectRatio);
            QSize sizeImageLabelPixmap = imageLabelPtr_ -> pixmap() -> size();
            if (sizeImageLabelPixmap != sizeAdjusted) 
            {
                updateImageLabel();
            }
        }
    }

    void CameraWindow::closeEvent(QCloseEvent *event)
    {
        if (capturing_)
        {
            stopImageCapture();
            threadPoolPtr_ -> waitForDone();
        }

        if (connected_)
        {
            disconnectCamera();
        }

        event -> accept();
    }

    // Private slots
    // ----------------------------------------------------------------------------------

    void CameraWindow::connectButtonClicked()
    {
        (!connected_) ? connectCamera() : disconnectCamera();
    }

    void CameraWindow::startButtonClicked()
    { 
        (!capturing_) ? startImageCapture() : stopImageCapture();
    }
   

    void CameraWindow::updateImageDisplay()
    {
        // Get information from image dispatcher
        imageDispatcherPtr_ -> acquireLock();
        cv::Mat mat = imageDispatcherPtr_ -> getImage();
        QImage img = matToQImage(mat);
        double fps = imageDispatcherPtr_ -> getFPS();
        double stamp = imageDispatcherPtr_ -> getTimeStamp();
        frameCount_ = imageDispatcherPtr_ -> getFrameCount();
        imageDispatcherPtr_ -> releaseLock();

        // Update image
        if (!img.isNull()) 
        {
            pixmapOriginal_ = QPixmap::fromImage(img);
            updateImageLabel();
            havePixmap_ = true;
        }

        // Update statusbar message
        QString statusMsg("Capturing,  logging = ");
        statusMsg += boolToOnOffQString(logging_);
        statusMsg += QString(", timer = ");
        statusMsg += boolToOnOffQString(timer_);
        statusMsg += QString().sprintf(",  %1.1f fps", fps);
        statusbarPtr_ -> showMessage(statusMsg);

        // Update capture time
        setCaptureTimeLabel(stamp);
    }

    void CameraWindow::startImageCaptureError(unsigned int errorId, QString errorMsg)
    {
        stopImageCapture();
        QString msgTitle("Start Image Capture Error");
        QString msgText("Failed to start image capture\n\nError ID: ");
        msgText += QString::number(errorId);
        msgText += "\n\n";
        msgText += errorMsg;
        QMessageBox::critical(this, msgTitle, msgText);
    }

    void CameraWindow::stopImageCaptureError(unsigned int errorId, QString errorMsg)
    {
        QString msgTitle("Stop Image Capture Error");
        QString msgText("Failed to stop image capture\n\nError ID: ");
        msgText += QString::number(errorId);
        msgText += "\n\n";
        msgText += errorMsg;
        QMessageBox::critical(this, msgTitle, msgText);
    }

    void CameraWindow::actionCameraInfoTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionCameraFormat7SettingsTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionLoggingEnabledTriggered()
    {
        logging_ = actionLoggingEnabledPtr_ -> isChecked();
        std::cout << "logging: " << boolToOnOffQString(logging_).toStdString() << std::endl;
    }

    void CameraWindow::actionLoggingVideoFileTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        QString videoFileFullPath = QFileDialog::getSaveFileName(
                this, 
                QString("Select Video File"),
                defaultSaveDir_.absolutePath()
                );

        std::cout << videoFileFullPath.toStdString() << std::endl;
    }

    void CameraWindow::actionLoggingSettingsTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionLoggingFormatTriggered()
    {
        // Get Format string
        QPointer<QAction> actionPtr = qobject_cast<QAction *>(sender());
        if (actionPtr != NULL) {
            QString formatString = actionPtr -> text();
            std:: cout << "logging format: " << formatString.toStdString() << std::endl;
        }
    }

    void CameraWindow::actionTimerEnabledTriggered()
    {
        timer_ = actionTimerEnabledPtr_ -> isChecked();
        std::cout << "timer: " << boolToOnOffQString(timer_).toStdString() << std::endl;
    }

    void CameraWindow::actionTimerSettingsTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionDisplayUpdateFreqTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionDisplayFlipVertTriggered()
    {
        flipVert_ = actionDisplayFlipVertPtr_ -> isChecked();
        std::cout << "flip vertical: " << boolToOnOffQString(flipVert_).toStdString() << std::endl;
    }

    void CameraWindow::actionDisplayFlipHorzTriggered()
    {
        flipHorz_ = actionDisplayFlipHorzPtr_ -> isChecked();
        std::cout << "flip horizontal: " << boolToOnOffQString(flipHorz_).toStdString() << std::endl;
    }

    void CameraWindow::actionDisplayRotTriggered()
    {
        // Get Rotation 
        QPointer<QAction> actionPtr = qobject_cast<QAction *>(sender());
        if (actionPtr != NULL) {
            QString rotString = actionPtr -> text();
            std:: cout << "display rotation: " << rotString.toStdString() << std::endl;
        }
    }

    void CameraWindow::actionHelpUserManualTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    void CameraWindow::actionHelpAboutTriggered()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    // Private methods
    // -----------------------------------------------------------------------------------

    void CameraWindow::initialize(Guid guid)
    {
        connected_ = false;
        capturing_ = false;
        havePixmap_ = false;
        logging_ = false;
        timer_ = false;
        flipVert_ = false;
        flipHorz_ = false;

        imageDisplayFreq_ = DEFAULT_IMAGE_DISPLAY_FREQ;
        cameraPtr_ = std::make_shared<Lockable<Camera>>(guid);

        threadPoolPtr_ = new QThreadPool(this);
        newImageQueuePtr_ = std::make_shared<LockableQueue<StampedImage>>();

        setDefaultSaveDir();
        setupCameraMenu();
        setupLoggingMenu();
        setupDisplayMenu();
        setupImageDisplayTimer();

        cameraPtr_ -> acquireLock();
        Guid cameraGuid = cameraPtr_ -> getGuid();
        cameraPtr_ -> releaseLock();

        QString windowTitle("BIAS Camera Window, Guid: ");
        windowTitle += QString::fromStdString(cameraGuid.toString());
        setWindowTitle(windowTitle);

        updateCameraInfoMessage();
        setCaptureTimeLabel(0.0);

        connectButtonPtr_ -> setText(QString("Connect"));
        startButtonPtr_ -> setText(QString("Start"));
        statusbarPtr_ -> showMessage(QString("Camera found, disconnected"));

        startButtonPtr_ -> setEnabled(false);
        connectButtonPtr_ -> setEnabled(true);

    }

    void CameraWindow::connectWidgets()
    {
        connect(
                startButtonPtr_, 
                SIGNAL(clicked()), 
                this, 
                SLOT(startButtonClicked())
                ); 

        connect(
                connectButtonPtr_, 
                SIGNAL(clicked()), 
                this, 
                SLOT(connectButtonClicked())
                );

        connect(
                actionCameraInfoPtr_, 
                SIGNAL(triggered()), 
                this, 
                SLOT(actionCameraInfoTriggered())
                );

        connect(
                actionCameraFormat7SettingsPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionCameraFormat7SettingsTriggered())
               );
       
        connect(
                actionLoggingEnabledPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingEnabledTriggered())
               );

        connect(
                actionLoggingVideoFilePtr_, 
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingVideoFileTriggered())
               );

        connect(
                actionLoggingSettingsPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingSettingsTriggered())
               );

        connect(
                actionLoggingFormatAVIPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingFormatTriggered())
               );

        connect(
                actionLoggingFormatFMFPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingFormatTriggered())
               );

        connect(
                actionLoggingFormatUFMFPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionLoggingFormatTriggered())
               );

        connect(
                actionTimerEnabledPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionTimerEnabledTriggered())
               );

        connect(
                actionTimerSettingsPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionTimerSettingsTriggered())
               );

        connect(
                actionDisplayUpdateFreqPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayUpdateFreqTriggered())
               );

        connect(
                actionDisplayFlipVertPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayFlipVertTriggered())
               );

        connect(
                actionDisplayFlipHorzPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayFlipHorzTriggered())
               );

        connect(
                actionDisplayRot0Ptr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayRotTriggered())
               );

        connect(
                actionDisplayRot90Ptr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayRotTriggered())
               );

        connect(
                actionDisplayRot180Ptr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayRotTriggered())
               );

        connect(
                actionDisplayRot270Ptr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionDisplayRotTriggered())
               );

        connect(
                actionHelpUserManualPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionHelpUserManualTriggered())
               );

        connect(
                actionHelpAboutPtr_,
                SIGNAL(triggered()),
                this,
                SLOT(actionHelpAboutTriggered())
               );
        
    }

    void CameraWindow::setDefaultSaveDir()
    {
#ifdef WIN32
        defaultSaveDir_ = QDir(QString(getenv("USERPROFILE")));
#else
        defaultSaveDir_ = QDir(QString(getenv("HOME")));
#endif 
        if (!defaultSaveDir_.exists())
        {
            // Something is wrong with home directory set to root.
            defaultSaveDir_ = QDir(QDir::rootPath());
        }
    }

    void CameraWindow::setupImageDisplayTimer()
    {
        imageDisplayTimerPtr_ = new QTimer(this);
        connect(
                imageDisplayTimerPtr_, 
                SIGNAL(timeout()), 
                this, 
                SLOT(updateImageDisplay())
                );
    }

    void CameraWindow::setupCameraMenu()
    {
        menuCameraPtr_ -> setEnabled(false);
        // TO DO ... temporary, disable format7 settings
        // -----------------------------------------------
        actionCameraFormat7SettingsPtr_ -> setEnabled(false);
        // -----------------------------------------------
        cameraTriggerActionGroupPtr_ = new QActionGroup(menuCameraPtr_);
        cameraTriggerActionGroupPtr_ -> addAction(actionCameraTriggerInternalPtr_);
        cameraTriggerActionGroupPtr_ -> addAction(actionCameraTriggerExternalPtr_);
    }

    void CameraWindow::setupLoggingMenu()
    {
        loggingFormatActionGroupPtr_ = new QActionGroup(menuLoggingFormatPtr_);
        loggingFormatActionGroupPtr_ -> addAction(actionLoggingFormatAVIPtr_);
        loggingFormatActionGroupPtr_ -> addAction(actionLoggingFormatFMFPtr_);
        loggingFormatActionGroupPtr_ -> addAction(actionLoggingFormatUFMFPtr_);
        actionLoggingFormatAVIPtr_ -> setChecked(true);
        actionLoggingFormatFMFPtr_ -> setChecked(false);
        actionLoggingFormatUFMFPtr_ -> setChecked(false);
    }

    void CameraWindow::setupDisplayMenu() 
    {
        setupDisplayRotMenu();
        setupDisplayOrientMenu();
    }

    void CameraWindow::setupDisplayOrientMenu()
    {

        actionDisplayFlipVertPtr_ -> setChecked(flipVert_);
        actionDisplayFlipHorzPtr_ -> setChecked(flipHorz_);
    }

    void CameraWindow::setupDisplayRotMenu()
    {
        rotationActionGroupPtr_ =  new QActionGroup(menuDisplayRotPtr_);
        rotationActionGroupPtr_ -> addAction(actionDisplayRot0Ptr_);
        rotationActionGroupPtr_ -> addAction(actionDisplayRot90Ptr_);
        rotationActionGroupPtr_ -> addAction(actionDisplayRot180Ptr_);
        rotationActionGroupPtr_ -> addAction(actionDisplayRot270Ptr_);
        actionDisplayRot0Ptr_ -> setChecked(true);
        actionDisplayRot90Ptr_ -> setChecked(false);
        actionDisplayRot180Ptr_ -> setChecked(false);
        actionDisplayRot270Ptr_ -> setChecked(false);
    }

    void CameraWindow::updateImageLabel()
    {
        QPixmap pixmapScaled =  pixmapOriginal_.scaled(
                imageLabelPtr_ -> size(),
                Qt::KeepAspectRatio, 
                Qt::SmoothTransformation
                );

        
        // Add framecount message
        QPainter painter(&pixmapScaled);
        QString msg;  
        msg.sprintf("%d",frameCount_);
        painter.setPen(QColor(0,220,0));
        painter.drawText(5,12, msg);

        imageLabelPtr_ -> setPixmap(pixmapScaled);
    } 

    void CameraWindow::connectCamera() 
    {
        bool error = false;
        unsigned int errorId;
        QString errorMsg;

        cameraPtr_ -> acquireLock();
        try
        {
            cameraPtr_ -> connect();
        }
        catch (RuntimeError &runtimeError)
        {
            error = true;
            errorId = runtimeError.id();
            errorMsg = QString::fromStdString(runtimeError.what());
        }
        cameraPtr_ -> releaseLock();

        if (error)
        {
            QString msgTitle("Connection Error");
            QString msgText("Failed to connect camera:\n\nError ID: ");
            msgText += QString::number(errorId);
            msgText += "\n\n";
            msgText += errorMsg;
            QMessageBox::critical(this, msgTitle, msgText);
            return;
        }
        connected_ = true;
        connectButtonPtr_ -> setText(QString("Disconnect"));
        statusbarPtr_ -> showMessage(QString("Connected, Stopped"));
        startButtonPtr_ -> setEnabled(true);
        menuCameraPtr_ -> setEnabled(true);
        updateCameraMenu();
        updateCameraInfoMessage();

    }

    void CameraWindow::disconnectCamera()
    {
        bool error = false;
        unsigned int errorId;
        QString errorMsg; 

        if (capturing_) 
        {
            stopImageCapture();
        }

        cameraPtr_ -> acquireLock();
        try
        {
            cameraPtr_ -> disconnect();
        }
        catch (RuntimeError &runtimeError)
        {
            error = true;
            errorId = runtimeError.id();
            errorMsg = QString::fromStdString(runtimeError.what());
        }
        cameraPtr_ -> releaseLock();

        if (error)
        {
            QString msgTitle("Disconnection Error");
            QString msgText("Failed to disconnect camera:\n\nError ID: ");
            msgText += QString::number(errorId);
            msgText += "\n\n";
            msgText += errorMsg;
            QMessageBox::critical(this, msgTitle, msgText);
            return;
        }
        connected_ = false;
        connectButtonPtr_ -> setText(QString("Connect"));
        statusbarPtr_ -> showMessage(QString("Disconnected"));
        startButtonPtr_ -> setEnabled(false);
        menuCameraPtr_ -> setEnabled(false);
        updateCameraInfoMessage();
        setCaptureTimeLabel(0.0);
    }
    
    void CameraWindow::startImageCapture() 
    {
        if (!connected_)
        {
            QString msgTitle("Capture Error");
            QString msgText("Unable to start image capture: not connected");
            QMessageBox::critical(this, msgTitle, msgText);
            return;
        }

        setMenuChildrenEnabled(menuLoggingPtr_, false);
        setMenuChildrenEnabled(menuTimerPtr_, false);

        imageGrabberPtr_ = new ImageGrabber(cameraPtr_, newImageQueuePtr_);
        imageDispatcherPtr_ = new ImageDispatcher(newImageQueuePtr_);

        connect(
                imageGrabberPtr_, 
                SIGNAL(startCaptureError(unsigned int, QString)),
                this,
                SLOT(startImageCaptureError(unsigned int, QString))
               );

        connect(
                imageGrabberPtr_,
                SIGNAL(stopCaptureError(unsigned int, QString)),
                this,
                SLOT(stopImageCaptureError(unsigned int, QString))
               );

        threadPoolPtr_ -> start(imageGrabberPtr_);
        threadPoolPtr_ -> start(imageDispatcherPtr_);

        unsigned int imageDisplayDt = int(1000.0/imageDisplayFreq_);
        imageDisplayTimerPtr_ -> start(imageDisplayDt);
        startButtonPtr_ -> setText(QString("Stop"));
        connectButtonPtr_ -> setEnabled(false);
        statusbarPtr_ -> showMessage(QString("Capturing"));
        capturing_ = true;
    }

    void CameraWindow::stopImageCapture()
    {
        if (!connected_)
        {
            QString msgTitle("Capture Error");
            QString msgText("Unable to stop image capture: not connected");
            QMessageBox::critical(this, msgTitle, msgText);
            return;
        }

        // Note, image grabber and dispatcher are destroyed by the 
        // threadPool when their run methods exit.
        imageDisplayTimerPtr_ -> stop();

        if (!imageGrabberPtr_.isNull())
        {
            imageGrabberPtr_ -> acquireLock();
            imageGrabberPtr_ -> stop();
            imageGrabberPtr_ -> releaseLock();
        }

        if (!imageDispatcherPtr_.isNull())
        {
            imageDispatcherPtr_ -> acquireLock();
            imageDispatcherPtr_ -> stop();
            imageDispatcherPtr_ -> releaseLock();
        }

        newImageQueuePtr_ -> clear();
        startButtonPtr_ -> setText(QString("Start"));
        connectButtonPtr_ -> setEnabled(true);
        statusbarPtr_ -> showMessage(QString("Connected, Stopped"));
        setMenuChildrenEnabled(menuLoggingPtr_, true);
        setMenuChildrenEnabled(menuTimerPtr_, true);
        capturing_ = false;
    }

    void CameraWindow::updateCameraInfoMessage()
    {
        if (connected_) 
        {
            cameraPtr_ -> acquireLock();
            QString vendorName = QString::fromStdString(
                    cameraPtr_ -> getVendorName()
                    );
            QString modelName = QString::fromStdString( 
                    cameraPtr_ -> getModelName()
                    );
            cameraPtr_ -> releaseLock();
            setCameraInfoMessage(vendorName, modelName);
        }
        else
        {
            setCameraInfoMessage("_____", "_____");
        }

    }
    
    void CameraWindow::setCameraInfoMessage(QString vendorName, QString modelName)
    {
        QString cameraInfoString("Camera:  ");
        cameraInfoString += vendorName;
        cameraInfoString += QString(",  ");
        cameraInfoString += modelName; 
        cameraInfoLabelPtr_ -> setText(cameraInfoString);
    }

    void CameraWindow::setMenuChildrenEnabled(QWidget *parentWidgetPtr, bool value)
    {
        QList<QMenu *> childList = parentWidgetPtr -> findChildren<QMenu *>();
        QList<QMenu *>::iterator childIt;
        for (childIt=childList.begin(); childIt!=childList.end(); childIt++)
        {
            QPointer<QMenu> menuPtr = *childIt;
            menuPtr -> setEnabled(value);
        }

        QList<QAction *> actionList = parentWidgetPtr -> actions();
        QList<QAction *>::iterator actionIt;
        for (actionIt=actionList.begin(); actionIt!=actionList.end(); actionIt++)
        {
            QPointer<QAction> actionPtr = *actionIt;
            actionPtr -> setEnabled(value);
        }
    } 

    void CameraWindow::updateCameraMenu()
    {
        populateVideoModeMenu();

    }

    void CameraWindow::populateVideoModeMenu()
    {
        bool error = false;
        unsigned int errorId;
        QString errorMsg;
        VideoModeList modeList;

        // Delete any existing actions
        deleteMenuActions(menuVideoModePtr_);

        // Add action for each allowed video mode
        cameraPtr_ -> acquireLock();
        try
        {
            modeList= cameraPtr_ -> getAllowedVideoModes(); 
        }
        catch (RuntimeError &runtimeError)
        {
            error = true;
            errorId = runtimeError.id();
            errorMsg = QString::fromStdString(runtimeError.what());
        }
        cameraPtr_ -> releaseLock();

        if (error) 
        {
            QString msgTitle("Camera Query Error");
            QString msgText("Failed to read allowed video modes from camera:\n\nError ID: ");
            msgText += QString::number(errorId);
            msgText += "\n\n";
            msgText += errorMsg;
            QMessageBox::critical(this, msgTitle, msgText);
            return;
        }

        VideoModeList::iterator modeIt;
        for (modeIt=modeList.begin(); modeIt!=modeList.end(); modeIt++)
        {
            VideoMode mode = *modeIt;
            QString modeString = QString::fromStdString(getVideoModeString(mode));
            QPointer<QAction> modeActionPtr = menuVideoModePtr_ -> addAction(modeString);
            modeActionPtr -> setCheckable(true);
            modeActionPtr -> setEnabled(false);

            // TO DO .. temporary only allow format7
            // -------------------------------------
            if (mode != VIDEOMODE_FORMAT7)
            {
                modeActionPtr -> setChecked(false);
            }
            else 
            {
                modeActionPtr -> setChecked(true);
            }
            // -------------------------------------
        }

    }

    void CameraWindow::deleteMenuActions(QMenu *menuPtr)
    {
        QList<QAction *> actionList = menuPtr -> actions();
        QList<QAction *>::iterator actionIt;
        for (actionIt=actionList.begin(); actionIt!=actionList.end(); actionIt++)
        {
            QPointer<QAction> actionPtr = *actionIt;
            menuPtr -> removeAction(actionPtr);
        }
    }

    void CameraWindow::setCaptureTimeLabel(double timeStamp)
    {
        QString stampString = timeStampToQString(timeStamp); 
        captureTimeLabelPtr_ -> setText(stampString);
    }


    // Utility functions
    // ----------------------------------------------------------------------------------

    QString boolToOnOffQString(bool value)
    {
        return (value) ? QString("on") : QString("off");
    }

    QString timeStampToQString(double timeStamp)
    {
        double hrs, min, sec;
        double rem = timeStamp; 

        hrs = std::floor(timeStamp/3600.0);
        rem -= hrs;
        min = std::floor(rem/60.0);
        rem -= min;
        sec = std::floor(rem);

        QString timeString;
        timeString.sprintf("%02d:%02d:%02d", int(hrs), int(min), int(sec));
        return timeString;
    }

} // namespace bias

       

