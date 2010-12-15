#include "Camera.h"
#include <windows.h>

#include "Filesystem.h"
using namespace Filesystem;

#include <cstdio>
#include <cassert>
using namespace std;

const string Camera::c_cameraName_5D = "Canon EOS 5D Mark II";
const string Camera::c_cameraName_40D = "Canon EOS 40D";
const string Camera::c_cameraName_7D = "Canon EOS 7D";

const int Camera::LiveView::c_delay = 200;
const int Camera::LiveView::c_frameBufferSize = 0x800000;

const int Camera::c_sleepTimeout = 10000;
const int Camera::c_sleepAmount = 50;

const bool Camera::c_forceJpeg = true;

bool Camera::s_initialized = false;

map<string, Camera::CameraModelData> Camera::s_modelData;

void Camera::initialize()
{
    if (s_initialized)
        return;

    s_initialized = true;

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

    s_modelData[c_cameraName_40D] = data40D;
    s_modelData[c_cameraName_5D] = data5D;
    s_modelData[c_cameraName_7D] = data7D;

    EdsInitializeSDK();
}

Camera::LiveView::LiveView() :
    m_state(Off),
    m_streamPtr(NULL)
{
    m_imageSize.width = 0;
    m_imageSize.height = 0;
    
    // set up buffer
    m_frameBuffer = new unsigned char[c_frameBufferSize];
    EdsError err = EDS_ERR_OK;
    err = err || EdsCreateMemoryStreamFromPointer(m_frameBuffer, c_frameBufferSize, &m_streamPtr);
}

Camera::LiveView::~LiveView()
{
    EdsRelease(m_streamPtr);
    delete[] m_frameBuffer;
}

Camera::Camera(EdsCameraRef cam) :
    m_liveView(),
    m_cam(cam),
    m_pendingZoomPosition(false),
    m_zoomRatio(1),
    m_pendingZoomRatio(false),
    m_whiteBalance(kEdsWhiteBalance_Auto),
    m_pendingWhiteBalance(false),
    m_good(false),
    m_pictureCompleteCallback(NULL)
{
    // call static initializer
    initialize();

    m_zoomPosition.x = 0;
    m_zoomPosition.y = 0;
    m_pendingZoomPoint.x = 0;
    m_pendingZoomPoint.y = 0;

    establishSession();
}

Camera::~Camera()
{
    // release session
    EdsCloseSession(m_cam);
    EdsRelease(m_cam);
}

Camera * Camera::getFirstCamera()
{
    initialize();

    EdsCameraListRef camList = NULL;
    EdsError err = EDS_ERR_OK;

    err = err || EdsGetCameraList(&camList);

    EdsUInt32 camCount = 0;
    err = err || EdsGetChildCount(camList, &camCount);

    if (camCount == 0) {
        fprintf(stderr, "ERROR: No camera connected.\n");
        if (camList)
            EdsRelease(camList);
        return NULL;
    }

    // get the first camera
    EdsCameraRef camHandle;
    err = err || EdsGetChildAtIndex(camList, 0, &camHandle);

    Camera * cam = NULL;
    
    if (! err)
        cam = new Camera(camHandle);

    // release the camera list data
    err = err || EdsRelease(camList);

    if (err)
        fprintf(stderr, "ERROR: Error occurred when getting first camera: %u\n", err);

    return cam;
}

Camera::CameraModelData Camera::cameraSpecificData() const
{
    string myName = name();

    if (s_modelData.count(myName) > 0)
        return s_modelData[myName];

    // default to 40D
    return s_modelData[c_cameraName_40D];
}

void Camera::establishSession()
{
    EdsError err = EDS_ERR_OK;

    // open a session
    err = err || EdsOpenSession(m_cam);

    // handlers
    err = err || EdsSetCameraStateEventHandler(m_cam, kEdsStateEvent_All, &staticStateEventHandler, this);
    err = err || EdsSetObjectEventHandler(m_cam, kEdsObjectEvent_All, &staticObjectEventHandler, this);
    err = err || EdsSetPropertyEventHandler(m_cam, kEdsPropertyEvent_All, &staticPropertyEventHandler, this);

    // set default options
    // save to computer, not memory card
    EdsUInt32 value = kEdsSaveTo_Host;
    err = err || EdsSetPropertyData(m_cam, kEdsPropID_SaveTo, 0, sizeof(EdsUInt32), &value);

    if (c_forceJpeg) {
        // enforce JPEG format
        EdsUInt32 qs;
        err = err || EdsGetPropertyData(m_cam, kEdsPropID_ImageQuality, 0, sizeof(qs), &qs);
        // clear the old image type setting and set the new one
        qs = qs & 0xff0fffff | (kEdsImageType_Jpeg << 20);
        err = err || EdsSetPropertyData(m_cam, kEdsPropID_ImageQuality, 0, sizeof(qs), &qs);
    }

    if (err)
        fprintf(stderr, "ERROR: When establishing session: %u\n", err);

    m_good = (err == 0);
}

EdsError EDSCALLBACK Camera::staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    // transfer from static to member
    assert(inContext);
    ((Camera *) inContext)->objectEventHandler(inEvent, inRef);
    if (inRef)
        EdsRelease(inRef);
    return 0;
}

EdsError EDSCALLBACK Camera::staticStateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext)
{
    // transfer from static to member
    assert(inContext);
    ((Camera *) inContext)->stateEventHandler(inEvent, inEventData);
    return 0;
}

EdsError EDSCALLBACK Camera::staticPropertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext)
{
    // transfer from static to member
    assert(inContext);
    ((Camera *) inContext)->propertyEventHandler(inEvent, inPropertyID, inParam);
    return 0;
}

void Camera::objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef)
{
    if (inEvent == kEdsObjectEvent_DirItemRequestTransfer) {
        transferOneItem(inRef, m_picOutFile);
    } else {
        fprintf(stderr, "DEBUG: objectEventHandler: event %u\n", inEvent);
    }
}

void Camera::stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData)
{
    fprintf(stderr, "DEBUG: stateEventHandler: event %u, parameter %u\n", inEvent, inEventData);
}

void Camera::propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam)
{
    if (inPropertyID == kEdsPropID_Evf_OutputDevice) {
        // TODO: actually check the property value instead of assuming it's going to do what we want.
        if (m_liveView.m_state == LiveView::WaitingToStart) {
            m_liveView.m_state = LiveView::On;
        } else if (m_liveView.m_state == LiveView::WaitingToStop) {
            m_liveView.m_state = LiveView::Off;
        }
    } else {
        fprintf(stderr, "DEBUG: propertyEventHandler: propid %u\n", inPropertyID);
    }
}

void Camera::transferOneItem(EdsBaseRef inRef, string outfile)
{
    // transfer the image in memory to disk
    EdsDirectoryItemInfo dirItemInfo;
    EdsStreamRef outStream;

    EdsError err = EDS_ERR_OK;

    err = err || EdsGetDirectoryItemInfo(inRef, &dirItemInfo);

    if (err) {
        fprintf(stderr, "ERROR: unable to get directory item info: %u\n", err);
        return;
    }

    // make sure we don't overwrite files
    ensurePathExists(getDirectoryName(outfile));
    outfile = makeUnique(outfile);

    // get a temp file to write to
    string tmpfile = tmpnam(NULL);

    // this creates the outStream that is used by EdsDownload to actually
    // grab and write out the file
    err = err || EdsCreateFileStream(tmpfile.c_str(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &outStream);

    // do the transfer
    err = err || EdsDownload(inRef, dirItemInfo.size, outStream);
    err = err || EdsDownloadComplete(inRef);

    // clean up
    err = err || EdsRelease(outStream);

    if (err) {
        fprintf(stderr, "ERROR: unable to transfer an item: %u\n", err);
        return;
    }

    moveFile(tmpfile, outfile);

    resumeLiveView();

    if (m_pictureCompleteCallback)
        m_pictureCompleteCallback(outfile);

    m_pictureDoneQueue.push(outfile);
}

void Camera::pauseLiveView()
{
    if (m_liveView.m_state == LiveView::On) {
        stopLiveView();
        m_liveView.m_state = LiveView::Paused;
    }
}

void Camera::resumeLiveView()
{
    if (m_liveView.m_state == LiveView::Paused)
        startLiveView();
}

bool Camera::isBusy()
{
    return m_liveView.m_state == LiveView::WaitingToStart || m_liveView.m_state == LiveView::WaitingToStop;
}

void Camera::setComputerCapabilities()
{
    // tell the camera how much disk space we have left
    EdsCapacity caps;

    caps.reset = true;
    caps.bytesPerSector = 512;
    caps.numberOfFreeClusters = 2147483647; // arbitrary large number
    EdsError err = EdsSetCapacity(m_cam, caps);

    if (err)
        fprintf(stderr, "ERROR: unable to set computer capabilities: %u\n", err);
}

void Camera::takePicture()
{
    setComputerCapabilities();

    // take a picture with the camera and save it to outfile
    EdsError err = EdsSendCommand(m_cam, kEdsCameraCommand_TakePicture, 0);

    if (err != EDS_ERR_OK)
        fprintf(stderr, "ERROR: unable to take picture: %u\n", err);
}

void Camera::takeSinglePicture(string outFile)
{
    assert(! isBusy());
    if (isBusy()) {
        fprintf(stderr, "ERROR: can't take picture, camera is busy.\n");
        return;
    }

    pauseLiveView();
    m_picOutFile = outFile;
    takePicture();
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
    EdsError err = EdsGetPropertyData(m_cam, kEdsPropID_WhiteBalance, 0, sizeof(m_whiteBalance), (EdsVoid *) &m_whiteBalance);
    if (err)
        fprintf(stderr, "ERROR: Unable to get white balance: %u\n", err);
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
    EdsError err = EdsGetDeviceInfo(m_cam, &deviceInfo);

    if (err) {
        fprintf(stderr, "ERROR: Unable to get device info: %u\n", err);
        return string();
    }

    return deviceInfo.szDeviceDescription;
}

void Camera::setPictureCompleteCallback(takePictureCompleteCallback callback)
{
    m_pictureCompleteCallback = callback;
}

bool Camera::good() const
{
    return m_good;
}

int Camera::pictureDoneQueueSize() const
{
    return m_pictureDoneQueue.size();
}

string Camera::popPictureDoneQueue()
{
    if (pictureDoneQueueSize() == 0) {
        return string();
    } else {
        string value = m_pictureDoneQueue.front();
        m_pictureDoneQueue.pop();
        return value;
    }
}

const unsigned char * Camera::liveViewFrameBuffer() const
{
    return m_liveView.m_frameBuffer;
}

int Camera::liveViewFrameBufferSize() const
{
    return m_liveView.c_frameBufferSize;
}

void Camera::grabLiveViewFrame()
{
    // skip frames if the camera isn't ready yet.
    if (m_liveView.m_state != LiveView::On) {
        fprintf(stderr, "WARNING: Skipping live view frame because camera is not in live view mode yet.\n");
        return;
    }

    EdsError err = EDS_ERR_OK;
    EdsImageRef img;
    // create image
    err = err || EdsCreateEvfImageRef(m_liveView.m_streamPtr, &img);
    if (err) {
        fprintf(stderr, "ERROR: Unable to create live view frame on the camera.\n");
        return;
    }

    // download the frame
    err = err || EdsDownloadEvfImage(m_cam, img);

    if (err == EDS_ERR_OBJECT_NOTREADY) {
        // skip the frame if the camera isn't ready
        fprintf(stderr, "ERROR: skipping live view frame because camera isn't ready\n");
        EdsRelease(img);
        return;
    } else if (err) {
        // skip the frame. unknown error
        fprintf(stderr, "ERROR: skipping live view frame: %u\n", err);
        EdsRelease(img);
        return;
    }

    // get/set zoom ratio
    if (m_pendingZoomRatio) {
        if (! EdsSetPropertyData(m_cam, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &m_zoomRatio))
            m_pendingZoomRatio = false;
    } else {
        EdsGetPropertyData(img, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &m_zoomRatio);
    }

    // get/set zoom position
    if (m_pendingZoomPosition) {
        if (! EdsSetPropertyData(m_cam, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &m_pendingZoomPoint))
            m_pendingZoomPosition = false;
    } else {
        EdsGetPropertyData(img, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &m_zoomPosition);
    }

    // set white balance
    if (m_pendingWhiteBalance) {
        if (! EdsSetPropertyData(m_cam, kEdsPropID_Evf_WhiteBalance, 0, sizeof(EdsWhiteBalance), &m_whiteBalance))
            m_pendingWhiteBalance = false;
    }

    EdsRelease(img);
}

void Camera::startLiveView()
{
    // it's kind of tricky to handle this, so we put it off for a TODO
    assert(m_liveView.m_state != LiveView::WaitingToStop);

    if (m_liveView.m_state == LiveView::WaitingToStart)
        return;

    // tell the computer to send live data to the computer
    EdsError err = EDS_ERR_OK;
    EdsUInt32 device;
    err = err || EdsGetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);
    device |= kEdsEvfOutputDevice_PC;
    err = err || EdsSetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

    if (err)
        fprintf(stderr, "ERROR: Unable to turn on live view: %u\n", err);
    else
        m_liveView.m_state = LiveView::WaitingToStart;
}

void Camera::stopLiveView()
{
    // it's kind of tricky to handle this, so we put it off for a TODO
    assert(m_liveView.m_state != LiveView::WaitingToStart);

    if (m_liveView.m_state == LiveView::WaitingToStop)
        return;

    // tell the camera to stop sending live data to the computer
    EdsError err = EDS_ERR_OK;
    EdsUInt32 device;
    err = err || EdsGetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);
    device &= ~kEdsEvfOutputDevice_PC;
    err = err || EdsSetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

    if (err)
        fprintf(stderr, "ERROR: Unable to turn off live view: %u\n", err);
    else
        m_liveView.m_state = LiveView::WaitingToStop;
}

EdsSize Camera::liveViewImageSize() const
{
    return m_liveView.m_imageSize;
}

void Camera::autoFocus()
{
    EdsUInt32 off = kEdsEvfDepthOfFieldPreview_OFF;
    EdsUInt32 on = kEdsEvfDepthOfFieldPreview_ON;

    // turn OFF
    EdsSetPropertyData(m_cam, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(kEdsEvfDepthOfFieldPreview_OFF), &off);
    // turn ON
    EdsSetPropertyData(m_cam, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(kEdsEvfDepthOfFieldPreview_ON), &on);
}

