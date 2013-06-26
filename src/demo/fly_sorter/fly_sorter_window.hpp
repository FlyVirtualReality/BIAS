#ifndef FLY_SORTER_WINDOW_HPP
#define FLY_SORTER_WINDOW_HPP

#include "ui_fly_sorter_window.h"
#include "parameters.hpp"
#include "image_grabber.hpp"
#include "blob_finder.hpp"
#include <memory>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPointer>
#include <opencv2/core/core.hpp>

class QThreadPool;
class ImageGrabber;
class QTimer;
class QNetworkAccessManager;
class QNetworkReply;
class QVarianMap;
class QByteArray;

class FlySorterWindow : public QMainWindow, private Ui::FlySorterWindow
{
    Q_OBJECT

    public:

        FlySorterWindow(QWidget *parent=0);

    signals:

        void stopCapture();

    protected:

        void resizeEvent(QResizeEvent *event);
        void closeEvent(QCloseEvent *event);
        void showEvent(QShowEvent *event);

    private slots:

        void startPushButtonClicked();
        void httpOutputCheckBoxChanged(int state);
        void newImage(ImageData imageData);
        void updateDisplayOnTimer(); 
        void networkAccessManagerFinished(QNetworkReply *reply);

    private:

        bool running_;
        FlySorterParam param_;
        float displayFreq_;
        QPointer<QThreadPool> threadPoolPtr_;
        QPointer<ImageGrabber> imageGrabberPtr_;
        QPointer<QTimer> displayTimerPtr_;
        QPointer<QNetworkAccessManager> networkAccessManagerPtr_;
        QPixmap previewPixmapOrig_;
        QPixmap thresholdPixmapOrig_;
        ImageData imageData_;
        BlobFinderData blobFinderData_;

        void connectWidgets();
        void initialize();
        void updateAllImageLabels();
        void updateImageLabel(QLabel *labelPtr, QPixmap &pixmap); 
        void resizeAllImageLabels();
        void resizeImageLabel(QLabel *labelPtr, QPixmap &pixmap);
        void setupImageLabels();
        void setupDisplayTimer();
        void setupNetworkAccessManager();
        void sendDataViaHttpRequest();
        QVariantMap dataToMap();
        QByteArray dataToJson();

};

#endif // #ifndef FLY_SORTER_WINDOW_HPP
