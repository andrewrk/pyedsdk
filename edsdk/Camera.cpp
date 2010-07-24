#include "Camera.h"
#include <cassert>

Camera * Camera::s_instance = NULL;

const string Camera::c_cameraName_5D = "Canon EOS 5D Mark II";
const string Camera::c_cameraName_40D = "Canon EOS 40D";
const string Camera::c_cameraName_7D = "Canon EOS 7D";

const int Camera::c_liveViewDelay = 200;
const int Camera::c_liveViewFrameBufferSize = 0x800000;

const int Camera::c_sleepTimeout = 10000;
const int Camera::c_sleepAmount = 50;

const bool Camera::c_forceJpeg = false;

Camera * Camera::instance()
{
    if (! s_instance)
        s_instance = new Camera();
    return s_instance;
}

Camera::Camera()
{
    // camera-specific information
    // 40D
    CameraModelData data40D;
    data40D.zoom100ImageSize.width = 1024;
    data40D.zoom100ImageSize.height = 680;
    data40D.zoom500ImageSize.width = 768;
    data40D.zoom500ImageSize.height = 800;
    data40D.zoom100MaxPosition.x = 3104;
    data40D.zoom100MaxPosition.y = 2016;
    data40D.zoom500MaxPosition.x = 3104;
    data40D.zoom500MaxPosition.y = 2080;
    data40D.zoomBoxSize.width = 204;
    data40D.zoomBoxSize.height = 208;

    // 5D
    CameraModelData data5D;
    data5D.zoom100ImageSize.width = 1024;
    data5D.zoom100ImageSize.height = 680;
    data5D.zoom500ImageSize.width = 1120;
    data5D.zoom500ImageSize.height = 752;
    data5D.zoom100MaxPosition.x = 4464;
    data5D.zoom100MaxPosition.y = 2976;
    data5D.zoom500MaxPosition.x = 4464;
    data5D.zoom500MaxPosition.y = 2976;
    data5D.zoomBoxSize.width = 202;
    data5D.zoomBoxSize.height = 135;

    // 7D
    CameraModelData data7D;
    data7D.zoom100ImageSize.width = 1056;
    data7D.zoom100ImageSize.height = 704;
    data7D.zoom500ImageSize.width = 1024;
    data7D.zoom500ImageSize.height = 680;
    data7D.zoom100MaxPosition.x = 4136;
    data7D.zoom100MaxPosition.y = 2754;
    data7D.zoom500MaxPosition.x = 4136;
    data7D.zoom500MaxPosition.y = 2754;
    data7D.zoomBoxSize.width = 212;
    data7D.zoomBoxSize.height = 144;

    m_modelData.clear();
    m_modelData[c_cameraName_40D] = data40D;
    m_modelData[c_cameraName_5D] = data5D;
    m_modelData[c_cameraName_7D] = data7D;

    resetState();
    checkError(EdsInitializeSDK());

    int limit = 20;
    int count = 0;
    while(! establishSession() && count < limit) {
        ++count;
    }
    if (count >= limit) {
        cerr << "FATAL: Error establishing a session with the camera." << endl;
        exit(-1);
    }
}

Camera::~Camera()
{
    if (! m_haveSession)
        return;

    stopLiveView(); // stops it only if it's running

    /*
    flushTransferQueue();
    releaseSession();
    EdsTerminateSDK();
    */

    s_instance = NULL;
}

CameraModelData Camera::cameraSpecificData() const
{
    string myName = name();

    if (m_modelData.count(myName) > 0)
        return m_modelData[myName];

    // default to 40D
    return m_modelData[c_cameraName_40D];
}

void Camera::resetState()
{
    m_waitingOnPic = false;
    m_liveViewOn = false;
    m_waitingToStartLiveView = false;
    // TODO: live view pic box and liveviewthread
    m_stoppingLiveView = false;
    // TODO: live view frame buffer and live view buffer handle
    m_liveViewStreamPtr = NULL;
    m_liveViewImageSize.width = 0;
    m_liveViewImageSize.height = 0;
    m_transferQueue.clear();
    m_haveSession = false;
    m_zoomPosition = NULL;
    m_pendingZoomPoint = NULL;
    m_zoomRatio = NULL;
    m_whiteBalance = NULL;

    m_pendingZoomRatio = false;
    m_pendingZoomPosition = false;
    m_pendingWhiteBalance = false;

    m_fastPictures = false;
    m_fastPicturesInteruptingLiveView = false;
}

bool Camera::establishSession()
{
    if (m_haveSession)
        return;

    EdsCameraListRef camList;
    int camCount;

    checkError(EdsGetCameraList(camList));
    checkError(EdsGetChildCount(camList, &camCount));

    if (camCount > 1) {
        cerr << "FATAL: Too many cameras connected." << endl;
        exit(-1);
    } else if (camCount == 0) {
        cerr << "FATAL: No camera connected." << endl;
        exit(-1);
    }

    // get the only camera
    checkError(EdsGetChildAtIndex(camList, 0, m_cam));

    // release the camera list data
    checkError(EdsRelease(camList));

    // open a session
    checkError(EdsOpenSession(m_cam));

    // handlers
    m_stateEventHandler = &staticStateEventHandler;
    m_objectEventHandler = &staticObjectEventHandler;
    m_propertyEventHandler = &staticPropertyEventHandler;

    checkError(EdsSetCameraStateEventHandler(m_cam, kEdsStateEvent_All, m_stateEventHandler, NULL));
    checkError(EdsSetObjectEventHandler(m_cam, kEdsObjectEvent_All, m_objectEventHandler, NULL));
    checkError(EdsSetCameraStateEventHandler(m_cam, kEdsPropertyEvent_All, m_propertyEventHandler, NULL));

    // set default options
    // save to computer, not memory card
    EdsUInt32 value = kEdsSaveTo_Host;
    checkError(EdsSetPropertyData(m_cam, kEdsPropID_SaveTo, 0, sizeof(EdsUInt32), &value));

    if (c_forceJpeg) {
        // enforce JPEG format
        EdsUInt32 qs;
        checkError(EdsGetPropertyData(m_cam, kEdsPropID_ImageQuality, 0, sizeof(qs), &qs));
        // clear the old image type setting and set the new one
        qs = qs & 0xhff0fffff | (kEdsImageType_Jpeg << 20);
        checkError(EdsSetPropertyData(m_cam, kEdsPropID_ImageQuality, 0, sizeof(qs), &qs));
    }

    m_haveSession = true;

    return true;
}

void Camera::releaseSession()
{
    /*
    assert(m_fastPictures == false);
    EdsCloseSession(m_cam);
    EdsRelease(m_cam);
    m_haveSession = false;
    */
}

void Camera::checkError(EdsError err)
{
    cerr << "ERROR: " << err << endl;
}

EdsError Camera::staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    // transfer from static to member
    s_instance->objectEventHandler(inEvent, inRef, inContext);
    return 0;
}

EdsError Camera::staticStateEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    // transfer from static to member
    s_instance->stateEventHandler(inEvent, inRef, inContext);
    return 0;
}

EdsError Camera::staticPropertyEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    // transfer from static to member
    s_instance->propertyEventHandler(inEvent, inRef, inContext);
    return 0;
}

EdsError Camera::objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    if (inEvent == kEdsObjectEvent_DirItemRequestTransfer) {
        if (m_fastPictures) {
            // queue up the transfer request
            TransferItem transfer;
            transfer.sdkRef = inRef;
            transfer.outFile = m_picOutFolder;
            m_transferQueue.push_back(transfer);
        } else {
            transferOneItem(inRef, m_picOutFolder);
        }

        // allow other thread to continue
        m_waitingOnPic = false;
    } else {
        cerr << "DEBUG: objectEventHandler: event " << inEvent << endl;
    }
}

EdsError Camera::stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext)
{
    cerr << "DEBUG: stateEventHandler: event " << inEvent << ", parameter " << inEventData << endl;
}

EdsError Camera::propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext)
{
    if (inPropertyID == kEdsPropID_Evf_OutputDevice) {
        if (m_waitingToStartLiveView) {
            // start live view thread
            // TODO: AUGH THREADS!

            // save state
            m_waitingToStartLiveView = false;
            m_liveViewOn = true;
        }
    } else {
        cerr << "DEBUG: propertyEventHandler: propid " << inPropertyID << endl;
    }
}

string Camera::ensureDoesNotExist(string outfile)
{
    // TODO
    return outfile;
}

void Camera::transferOneItem(EdsBaseRef inRef, string outFolder)
{
    // transfer the image in memory to disk
    EdsDirectoryItemInfo dirItemInfo;
    EdsStreamRef outStream;

    checkError(EdsGetDirectoryItemInfo(inRef, dirItemInfo));

    string outfile = outFolder + "\\";
    outfile += dirItemInfo.szFileName;

    // make sure we don't overwrite files
    outfile = ensureDoesNotExist(outfile);

    // get a temp file to write to
    string tmpfile = GET TEMP FILE; // TODO

    // this creates the outStream that is used by EdsDownload to actually
    // grab and write out the file
    checkError(EdsCreateFileStream(tmpfile, kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, outStream));

    // do the transfer
    checkError(EdsDownload(inRef, dirItemInfo.size, outStream));
    checkError(EdsDownloadComplete(inRef));

    // clean up
    checkError(EdsRelease(outStream));

    FILE MOVE tmpfile -> outfile; // TODO
}

void Camera::beginFastPictures()
{
    checkBusy();

    m_fastPicturesInteruptingLiveView = m_liveViewOn;
    if (m_fastPicturesInteruptingLiveView)
        stopLiveView();

    establishSession();
    m_fastPictures = true;
}

void Camera::endFastPictures()
{
    if (! m_fastPictures)
        return;

    while (m_transferQueue.size() > 0) {
        TransferItem transfer = m_transferQueue.front();
        transferOneItem(transfer.sdkRef, transfer.outFile);
    }

    m_fastPictures = false;

    releaseSession();

    if (m_fastPicturesInteruptingLiveView) {
        m_fastPicturesInteruptingLiveView = false;
        // TODO: start live view
    }
}

void takeFastPicture(string outFolder)
{
    if (! m_fastPictures) {
        cerr << "FATAL: must be in fast picture mode to take a fast picture." << endl;
        exit(-1);
    }

    checkDirectory(outFolder);

    // set flag indicating we are waiting on a callback call
    m_waitingOnPic = true;
    m_picOutFolder = outFolder;

    bool tryAgain = true;
    while (tryAgain) {
        tryAgain = false;
        if (! takePicture(outFolder)) {
            // TODO: sleep sleepAmount
            tryAgain = true;
        }
    }
}

void Camera::checkBusy()
{
    if (m_waitingOnPic || m_waitingToStartLiveView) {
        // bad programmer. should have disabled user controls.
        cerr << "FATAL: camera is busy." << endl;
        exit(-1);
    }
}

void Camera::lieToTheCameraAboutHowMuchSpaceWeHaveOnTheComputer()
{
    // tell the camera how much disk space we have left
    EdsCapacity caps;

    caps.reset = True;
    caps.bytesPerSector = 512;
    caps.numberOfFreeClusters = 2147483647; // arbitrary large number
    checkError(EdsSetCapacity(m_cam, caps));
}

bool Camera::takePicture(string outFile)
{
    lieToTheCameraAboutHowMuchSpaceWeHaveOnTheComputer();

    // take a picture with the camera and save it to outfile
    EdsError err = EdsSendCommand(m_cam, kEdsCameraCommand_TakePicture, 0);

    if (err != EDS_ERR_OK) {
        m_waitingOnPic = false;
        checkError(err);
    }

    for (int i = 0; i < c_sleepTimeout / c_sleepAmount; ++i) {
        // TODO:
        // thread.sleep(sleepAmount);
        // doEvents();

        if (! m_waitingOnPic)
            return true;
    }

    return false;
}

void Camera::updateLiveView()
{
    // TODO
}

void Camera::showLiveViewFrame()
{
    // TODO
}

void Camera::takeSinglePicture(string outFolder)
{
    bool interuptingLiveView = m_liveViewOn;
    // TODO: picture box code
    bool haveSession = m_haveSession;

    checkDirectory(outFolder);

    establishSession();
    checkBusy();

    if (interuptingLiveView)
        stopLiveView();

    // set flag indicating we are waiting on a callback call
    m_waitingOnPic = true;
    m_picOutFolder = outFolder;

    if (takePicture(outFolder)) {
        if (interuptingLiveView) {
            // TODO: start live view
        }
    } else {
        // we never got a callback. throw an error
        if (interuptingLiveView) {
            // TODO: start live view
        } else {
            if (! haveSession)
                releaseSession();
        }

        m_waitingOnPic = false;
        cerr << "ERROR: Take picture failed." << endl;
    }
}

void Camera::startLiveView(liveViewFrameCallback callback)
{
    // TODO
}

void Camera::stopLiveView()
{
    // TODO
}

EdsSize Camera::liveViewImageSize() const
{
    return m_liveViewImageSize;
}

EdsPoint Camera::zoomPosition() const
{
    return m_zoomPosition;
}

void Camera::setZoomPosition(EdsPoint position)
{
    m_pendingZoomPoint = position;
    m_pendingZoomPosition = true;
}

int Camera::zoomRatio() const
{
    return m_zoomRatio;
}

void Camera::setZoomRatio(int zoomRatio)
{
    m_zoomRatio = zoomRatio;
    m_pendingZoomRatio = true;
}

EdsWhiteBalance Camera::whiteBalance() const
{
    bool haveSession = m_haveSession;
    establishSession();
    checkError(EdsGetPropertyData(m_cam, kEdsPropID_WhiteBalance, 0, sizeof(m_whiteBalance), &m_whiteBalance));
    if (! haveSession)
        releaseSession();
    return m_whiteBalance;
}
void Camera::setWhiteBalance(EdsWhiteBalance whiteBalance)
{
    m_whiteBalance = whiteBalance;
    m_pendingWhiteBalance = true;
}

string Camera::name() const
{
    EdsDeviceInfo deviceInfo;
    checkError(EdsGetDeviceInfo(m_cam, deviceInfo));
    return deviceInfo.szDeviceDescription;
}

