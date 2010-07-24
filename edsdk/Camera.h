#ifndef CAMERA_H
#define CAMERA_H

#include <string>
using namespace std;

#include "EDSDK.h"

class Camera
{
public: // variables
    typedef void (liveViewFrameCallback *) (char * frame);

    enum CameraState {
        Ready,
        TooManyCameras,
        NoCameraConnected,
        DeviceBusy
    };

public: // methods
    Camera * instance();

    void takeSinglePicture(string outFolder);

    void beginFastPictures();
    void takeFastPicture(string outFolder);
    void endFastPictures();

    // opens up live view and begins calling callback for each frame.
    void startLiveView(liveViewFrameCallback callback);
    void stopLiveView();

    // size of the frames coming through live view. only valid once
    // live view has started.
    EdsSize liveViewImageSize() const;

    // move the zoom point of live view around
    EdsPoint zoomPosition() const;
    void setZoomPosition(EdsPoint position);

    // get or set the zoom factor of live view. zoomRatio is an integer which
    // is a multiplier.
    int zoomRatio() const;
    void setZoomRatio(int zoomRatio);

    // white balance property
    EdsWhiteBalance whiteBalance() const;
    void setWhiteBalance(EdsWhiteBalance whiteBalance);

    // name of the camera model
    string name() const;

private: // variables
    static Camera * s_instance;

    static const string c_cameraName_5D;
    static const string c_cameraName_40D;
    static const string c_cameraName_7D;

    struct CameraModelData {
        EdsPoint zoom100MaxPosition;
        EdsPoint zoom500MaxPosition;
        EdsSize zoom100ImageSize;
        EdsSize zoom500ImageSize;
        EdsSize zoomBoxSize;
    };

    struct TransferItem {
        EdsBaseRef sdkRef;
        string outFile;
    };

    map<string, CameraModelData> m_modelData;
    
    EdsCameraRef m_cam;
    EdsObjectEventHandler m_objectEventHandler;
    EdsStateEventHandler m_stateEventHandler;
    EdsPropertyEventHandler m_propertyEventHandler;

    bool m_waitingOnPic;
    string m_picOutFolder;

    // live view
    static const int c_liveViewDelay;
    static const int c_liveViewFrameBufferSize;
    // TODO: Private m_liveViewThread as Thread
    bool m_liveViewOn;
    bool m_waitingToStartLiveView;
    // TODO: Private m_liveViewPicBox as PictureBox
    bool m_stoppingLiveView;
    // TODO: Private m_liveViewFrameBuffer as Byte()
    // TODO: Private m_liveViewBufferHandle as GCHandle
    EdsStreamRef m_liveViewStreamPtr;
    EdsSize m_liveViewImageSize;

    EdsPoint * m_zoomPosition;
    bool m_pendingZoomPosition;
    EdsPoint * m_pendingZoomPoint;
    int * m_zoomRatio;
    bool m_pendingZoomRatio;
    int * m_whiteBalance;
    bool m_pendingWhiteBalance;
    bool m_fastPictures;

    bool m_haveSession;

    bool m_fastPicturesInteruptingLiveView;
    // TODO: Private m_fastPicturesLiveViewBox as PictureBox

    // how many milliseconds to wait before giving up
    static const int c_sleepTimeout;
    // how many milliseconds to sleep before doing the event pump
    static const int c_sleepAmount;

    // do we make sure that output pictures are jpegs?
    static const bool c_forceJpeg;

    // pictures we need to download but are saving for later
    queue<TransferItem> m_transferQueue;

    // TODO: we left out CoInitializeEx. make sure it still works.
private: // methods
    Camera();
    ~Camera();

    CameraModelData cameraSpecificData() const;
    void resetState();

    // try to connect to a camera. returns success.
    bool establishSession();

    // release the session with the camera.
    void releaseSession();
    
    void checkError();

    static EdsError staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext);
    static EdsError staticStateEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext);
    static EdsError staticPropertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext);

    EdsError objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext);
    EdsError stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext);
    EdsError propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext);

    string ensureDoesNotExist(string outfile);

    void transferOneItem(EdsBaseRef inRef, string outFolder);
    void checkBusy();
    void lieToTheCameraAboutHowMuchSpaceWeHaveOnTheComputer();

    // internal take picture function. returns true upon success
    bool takePicture(string outFile);

    void updateLiveView();
    void showLiveViewFrame();
}

#endif
