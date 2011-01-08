#ifndef CAMERA_H
#define CAMERA_H

#include <string>
#include <map>
#include <queue>
#include <sstream>
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

        enum ErrorLevel {
            Debug,
            Warning,
            Error,
            None,
        };

        struct ErrorMessage {
            ErrorLevel level;
            string msg;
        };

        enum MeteringMode {
            SpotMetering = 1,
            EvaluativeMetering = 3,
            PartialMetering = 4,
            CenterWeightedMetering = 5,
        };

        enum DriveMode {
            SingleFrameShooting = 0x00000000,
            ContinuousShooting = 0x00000001,
            Video = 0x00000002,
            HighSpeedContinuousShooting = 0x00000004,
            LowSpeedContinuousShooting = 0x00000005,
            SilentSingleShooting = 0x00000006,
            TenSecSelfTimerPlusShots = 0x00000007,
            TenSecSelfTimer = 0x00000010,
            TwoSecSelfTimer = 0x00000011,
        };

        enum AFMode {
            OneShotAF = 0,
            AIServoAF = 1,
            AIFocusAF = 2,
            ManualFocus = 3,
        };

        struct CameraModelData {
            EdsPoint zoom100MaxPosition;
            EdsPoint zoom500MaxPosition;
            EdsSize zoom100ImageSize;
            EdsSize zoom500ImageSize;
            EdsSize zoomBoxSize;
        };

    public: // methods
        ~Camera();

        // you are responsible for deleting it when you're done
        static Camera * getFirstCamera();

        // try to connect to a camera. returns success.
        bool connect();
        bool disconnect();

        // use if you want to start over
        static void terminate();

        string name() const { return m_name; }
        const CameraModelData * cameraSpecificData() const;

        // takes a picture with the camera and puts it in outFile.
        // returns immediately but the picture won't be finished immediately.
        bool takeSinglePicture(string outFile);

        // if you want to be notified when a picture is finally done, use this:
        void setPictureCompleteCallback(takePictureCompleteCallback callback);

        // you have to put the camera in "live view mode" before you can get live view frames.
        bool startLiveView();
        bool stopLiveView();
        // this function refreshes the frame buffer with a new image from the camera.
        bool grabLiveViewFrame();

        // move the zoom point of live view around
        EdsPoint zoomPosition() const;
        void setZoomPosition(EdsPoint position);

        // get or set the zoom factor of live view. zoomRatio is an integer which
        // is a multiplier.
        int zoomRatio() const;
        void setZoomRatio(int zoomRatio);

        EdsWhiteBalance whiteBalance() const;
        void setWhiteBalance(EdsWhiteBalance whiteBalance);

        MeteringMode meteringMode() const;
        void setMeteringMode(MeteringMode mode);

        DriveMode driveMode() const;
        void setDriveMode(DriveMode mode);

        AFMode afMode() const;
        void setAFMode(AFMode mode);

        // get the oldest message from the event queue and remove it.
        // the return value is the filename of the completed picture
        string popPictureDoneQueue();
        int pictureDoneQueueSize() const;

        // get a pointer to the live view frame data
        const unsigned char * liveViewFrameBuffer() const;
        // length in bytes of the live view frame data
        int liveViewFrameBufferSize() const; 

        // perform auto focus once right now
        bool autoFocus();

        // sets the log level for error messages. messages are added to a
        // queue that you can access with popErrMsg()
        static void setErrorLevel(ErrorLevel level);

        // gets the oldest error message from the queue
        static ErrorMessage popErrMsg();
        static int errMsgQueueSize();

    private: // variables
        static bool s_initialized;
        static bool s_staticDataInitialized;

        static stringstream * s_err;
        static queue<ErrorMessage> s_errMsgQueue;
        static ErrorLevel s_errorLevel;

        class LiveView {
            public:
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

            // if you set m_state to either of the Waiting states, you need to also set m_desiredNewState
            State m_state;
            State m_desiredNewState; // what state to try to get to after we're done waiting

            EdsStreamRef m_streamPtr;

            // the allocated space we have set aside for frame data
            unsigned char * m_frameBuffer;

            LiveView();
            ~LiveView();
        };
        LiveView * m_liveView;

        static const string c_cameraName_5D;
        static const string c_cameraName_40D;
        static const string c_cameraName_7D;

        static map<string, CameraModelData> s_modelData;

        struct TransferItem {
            EdsBaseRef sdkRef;
            string outFile;
        };

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

        queue<string> m_pictureDoneQueue;

        takePictureCompleteCallback m_pictureCompleteCallback;

        bool m_connected;

        CameraModelData * m_cameraData;

        string m_name;

    private: // methods
        static void initialize();
        static void initStaticData();

        Camera();

        static EdsError EDSCALLBACK staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext);
        static EdsError EDSCALLBACK staticStateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext);
        static EdsError EDSCALLBACK staticPropertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext);

        void objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef);
        void stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData);
        void propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam);

        string ensureDoesNotExist(string outfile);

        bool transferOneItem(EdsBaseRef inRef, string outFolder);
        bool setComputerCapabilities();

        bool pauseLiveView();
        bool resumeLiveView();

        static void pushErrMsg(ErrorLevel level = Error);

        bool _startLiveView();
        bool _stopLiveView();

        void handleCameraIsReady();

        // name of the camera model
        string getName() const;


    friend class LiveView;
};

#endif
