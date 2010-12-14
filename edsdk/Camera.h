#ifndef CAMERA_H
#define CAMERA_H

#include <string>
#include <map>
#include <queue>
using namespace std;

#include "EDSDK.h"
#include "EDSDKErrors.h"
#include "EDSDKTypes.h"


class Camera
{
    public: // variables
        typedef void (* takePictureCompleteCallback) (string filename);

        enum CameraState {
            Ready,
            TooManyCameras,
            NoCameraConnected,
            DeviceBusy
        };

    public: // methods
        ~Camera();

        // you are responsible for deleting it when you're done
        static Camera * getFirstCamera();

        // true if everything is ok
        bool good() const;

        // name of the camera model
        string name() const;

        // takes a picture with the camera and puts it in outFile.
        // returns immediately but the picture won't be finished immediately.
        void takeSinglePicture(string outFile);

        // if you want to be notified when a picture is finally done, use this:
        void setPictureCompleteCallback(takePictureCompleteCallback callback);

        // you have to put the camera in "live view mode" before you can get live view frames.
        void startLiveView();
        void stopLiveView();
        // this function refreshes the frame buffe with a new image from the camera.
        void grabLiveViewFrame();

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

        // get the oldest message from the event queue and remove it.
        // the return value is the filename of the completed picture
        string popPictureDoneQueue();
        int pictureDoneQueueSize() const;

        // get a pointer to the live view frame data
        const unsigned char * liveViewFrameBuffer() const;
        // length in bytes of the live view frame data
        int liveViewFrameBufferSize() const; 

        // perform auto focus once right now
        void autoFocus();

        void pressShutterHalfway();

    private: // variables
        static bool s_initialized;

        class LiveView {
            public:
            // TODO: Private m_liveViewThread as Thread
            static const int c_delay;
            static const int c_frameBufferSize;
            
            enum State {
                // we don't want live view on.
                Off,
                // we have requested live view to turn on but it has not complied yet
                WaitingToStart,
                // it's on and we're currently streaming.
                On,
                // we'd like it to be on, but we are doing something which must
                // interupt live view, like taking a picture
                Paused,
                // we have requested live view to turn off but it has not complied yet
                WaitingToStop
            };

            State m_state;

            EdsStreamRef m_streamPtr;
            EdsSize m_imageSize;

            // the allocated space we have set aside for frame data
            unsigned char * m_frameBuffer;

            LiveView();
            ~LiveView();
        };
        LiveView m_liveView;

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

        static map<string, CameraModelData> s_modelData;

        EdsCameraRef m_cam;

        // what file to save the next picture as
        string m_picOutFile;


        EdsPoint m_zoomPosition;
        bool m_pendingZoomPosition;
        EdsPoint m_pendingZoomPoint;
        EdsUInt32 m_zoomRatio;
        bool m_pendingZoomRatio;
        EdsWhiteBalance m_whiteBalance;
        bool m_pendingWhiteBalance;

        // how many milliseconds to wait before giving up
        static const int c_sleepTimeout;
        // how many milliseconds to sleep before doing the event pump
        static const int c_sleepAmount;

        // do we make sure that output pictures are jpegs?
        static const bool c_forceJpeg;

        queue<string> m_pictureDoneQueue;

        // true if everything is working
        bool m_good;

        takePictureCompleteCallback m_pictureCompleteCallback;

    private: // methods
        static void initialize();

        Camera(EdsCameraRef cam);

        CameraModelData cameraSpecificData() const;

        // try to connect to a camera. returns success.
        void establishSession();

        static EdsError EDSCALLBACK staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext);
        static EdsError EDSCALLBACK staticStateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext);
        static EdsError EDSCALLBACK staticPropertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext);

        void objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef);
        void stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData);
        void propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam);

        string ensureDoesNotExist(string outfile);

        void transferOneItem(EdsBaseRef inRef, string outFolder);
        bool isBusy();
        void setComputerCapabilities();

        // internal take picture function
        void takePicture();

        void pauseLiveView();
        void resumeLiveView();

};

#endif
