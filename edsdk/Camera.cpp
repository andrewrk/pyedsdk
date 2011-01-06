#include "Camera.h"
#include <windows.h>

#include "ErrorMap.h"

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

bool Camera::s_initialized = false;
bool Camera::s_staticDataInitialized = false;

map<string, Camera::CameraModelData> Camera::s_modelData;

void Camera::initialize()
{
    if (s_initialized)
        return;
    s_initialized = true;

    initStaticData();
    EdsInitializeSDK();
}

void Camera::initStaticData()
{
    if (s_staticDataInitialized)
        return;
    s_staticDataInitialized = true;

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
}

Camera::LiveView::LiveView(Camera * camera) :
    m_state(Off),
    m_streamPtr(NULL)
{
    m_imageSize.width = 0;
    m_imageSize.height = 0;
    
    // set up buffer
    m_frameBuffer = new unsigned char[c_frameBufferSize];
    EdsError err = EdsCreateMemoryStreamFromPointer(m_frameBuffer, c_frameBufferSize, &m_streamPtr);
    
    if (err) {
        *(camera->m_err) << "Unable to create memory stream for live view: " << ErrorMap::errorMsg(err);
        camera->pushErrMsg(Camera::Error);
    }
}

Camera::LiveView::~LiveView()
{
    EdsRelease(m_streamPtr);
    delete[] m_frameBuffer;
}

Camera::Camera() :
    m_liveView(NULL),
    m_pendingZoomPosition(false),
    m_zoomRatio(1),
    m_pendingZoomRatio(false),
    m_whiteBalance(kEdsWhiteBalance_Auto),
    m_pendingWhiteBalance(false),
    m_pictureCompleteCallback(NULL),
    m_connected(false),
    m_errorLevel(None)
{
    m_zoomPosition.x = 0;
    m_zoomPosition.y = 0;
    m_pendingZoomPoint.x = 0;
    m_pendingZoomPoint.y = 0;
}

Camera::~Camera()
{
    disconnect();
}

bool Camera::disconnect()
{
    delete m_liveView;
    m_liveView = NULL;

    if (m_connected) {
        // release session
        EdsError err;
        err = EdsCloseSession(m_cam);

        bool success = true;
        if (err) {
            *m_err << "Unable to close session: " << ErrorMap::errorMsg(err);
            pushErrMsg();
            success = false;
        }

        err = EdsRelease(m_cam);

        if (err) {
            *m_err << "Unable to deallocate session: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        }

        return success;
    }

    return true;
}

Camera * Camera::getFirstCamera()
{
    initialize();

    Camera * cam = new Camera();

    EdsCameraListRef camList = NULL;
    EdsError err;

    err = EdsGetCameraList(&camList);
    if (err) {
        *(cam->m_err) << "Unable to get camera list: " << ErrorMap::errorMsg(err);
        cam->pushErrMsg();
        delete cam;
        return NULL;
    }
    if (! camList) {
        *(cam->m_err) << "EDSDK didn't give us a camera list ref.";
        cam->pushErrMsg();
        delete cam;
        return NULL;
    }

    EdsUInt32 camCount = 0;
    err = EdsGetChildCount(camList, &camCount);

    if (err) {
        *(cam->m_err) << "Unable to get camera count: " << ErrorMap::errorMsg(err);
        cam->pushErrMsg();
        EdsRelease(camList);
        delete cam;
        return NULL;
    }

    if (camCount == 0) {
        *(cam->m_err) << "No camera connected.";
        cam->pushErrMsg(Warning);
        EdsRelease(camList);
        delete cam;
        return NULL;
    }

    // get the first camera
    EdsCameraRef camHandle;
    err = EdsGetChildAtIndex(camList, 0, &camHandle);

    if (err) {
        *(cam->m_err) << "Unable to get connected camera handle: " << ErrorMap::errorMsg(err);
        cam->pushErrMsg(Error);
        EdsRelease(camList);
        delete cam;
        return NULL;
    }

    cam->m_cam = camHandle;

    err = EdsRelease(camList);

    if (err) {
        *(cam->m_err) << "Unable to release camera list handle: " << ErrorMap::errorMsg(err);
        cam->pushErrMsg(Warning);
    }

    return cam;
}

Camera::CameraModelData Camera::cameraSpecificData()
{
    string myName = name();

    if (s_modelData.count(myName) > 0)
        return s_modelData[myName];

    // default to 40D
    return s_modelData[c_cameraName_40D];
}

bool Camera::connect()
{
    delete m_liveView;
    m_liveView = new LiveView(this);

    EdsError err;

    // open a session
    err = EdsOpenSession(m_cam);

    if (err) {
        *m_err << "Unable to open session: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    // handlers
    err = EdsSetCameraStateEventHandler(m_cam, kEdsStateEvent_All, &staticStateEventHandler, this);

    if (err) {
        *m_err << "Unable to set camera state event handler: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    err = EdsSetObjectEventHandler(m_cam, kEdsObjectEvent_All, &staticObjectEventHandler, this);

    if (err) {
        *m_err << "Unable to set object event handler: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }
    
    err = EdsSetPropertyEventHandler(m_cam, kEdsPropertyEvent_All, &staticPropertyEventHandler, this);

    if (err) {
        *m_err << "Unable to set property event handler: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    // set default options
    // save to computer, not memory card
    EdsUInt32 value = kEdsSaveTo_Host;
    err = EdsSetPropertyData(m_cam, kEdsPropID_SaveTo, 0, sizeof(EdsUInt32), &value);

    if (err) {
        *m_err << "Unable to set property SaveTo device to computer: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    return true;
}

EdsError EDSCALLBACK Camera::staticObjectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef, EdsVoid * inContext)
{
    // transfer from static to member
    if (! inContext)
        return 0;

    ((Camera *) inContext)->objectEventHandler(inEvent, inRef);

    if (inRef)
        EdsRelease(inRef);

    return 0;
}

EdsError EDSCALLBACK Camera::staticStateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData, EdsVoid * inContext)
{
    // transfer from static to member
    if (! inContext)
        return 0;

    ((Camera *) inContext)->stateEventHandler(inEvent, inEventData);

    return 0;
}

EdsError EDSCALLBACK Camera::staticPropertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid * inContext)
{
    // transfer from static to member
    if (! inContext)
        return 0;

    ((Camera *) inContext)->propertyEventHandler(inEvent, inPropertyID, inParam);

    return 0;
}

void Camera::objectEventHandler(EdsObjectEvent inEvent, EdsBaseRef inRef)
{
    if (inEvent == kEdsObjectEvent_DirItemRequestTransfer) {
        transferOneItem(inRef, m_picOutFile);
    } else {
        *m_err << "objectEventHandler: event " << inEvent;
        pushErrMsg(Debug);
    }
}

void Camera::stateEventHandler(EdsStateEvent inEvent, EdsUInt32 inEventData)
{
    *m_err << "stateEventHandler: event " << inEvent << ", parameter " << inEventData;
    pushErrMsg(Debug);
}

void Camera::propertyEventHandler(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam)
{
    switch (inPropertyID) {
		case kEdsPropID_Unknown:
            *m_err << "Incoming property event: Unknown";
            pushErrMsg(Warning);
			break;
		case kEdsPropID_ProductName:
            *m_err << "Incoming property event: ProductName";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_BodyID:
            *m_err << "Incoming property event: BodyID";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_OwnerName:
            *m_err << "Incoming property event: OwnerName";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_MakerName:
            *m_err << "Incoming property event: MakerName";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_DateTime:
            *m_err << "Incoming property event: DateTime";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FirmwareVersion:
            *m_err << "Incoming property event: FirmwareVersion";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_BatteryLevel:
            *m_err << "Incoming property event: BatteryLevel";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_CFn:
            *m_err << "Incoming property event: CFn";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_SaveTo:
            *m_err << "Incoming property event: SaveTo";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_CurrentStorage:
            *m_err << "Incoming property event: CurrentStorage";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_CurrentFolder:
            *m_err << "Incoming property event: CurrentFolder";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_MyMenu:
            *m_err << "Incoming property event: MyMenu";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_BatteryQuality:
            *m_err << "Incoming property event: BatteryQuality";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_HDDirectoryStructure:
            *m_err << "Incoming property event: HDDirectoryStructure";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ImageQuality:
            *m_err << "Incoming property event: ImageQuality";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_JpegQuality:
            *m_err << "Incoming property event: JpegQuality";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Orientation:
            *m_err << "Incoming property event: Orientation";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ICCProfile:
            *m_err << "Incoming property event: ICCProfile";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FocusInfo:
            *m_err << "Incoming property event: FocusInfo";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_DigitalExposure:
            *m_err << "Incoming property event: DigitalExposure";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_WhiteBalance:
            *m_err << "Incoming property event: WhiteBalance";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ColorTemperature:
            *m_err << "Incoming property event: ColorTemperature";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_WhiteBalanceShift:
            *m_err << "Incoming property event: WhiteBalanceShift";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Contrast:
            *m_err << "Incoming property event: Contrast";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ColorSaturation:
            *m_err << "Incoming property event: ColorSaturation";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ColorTone:
            *m_err << "Incoming property event: ColorTone";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Sharpness:
            *m_err << "Incoming property event: Sharpness";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ColorSpace:
            *m_err << "Incoming property event: ColorSpace";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ToneCurve:
            *m_err << "Incoming property event: ToneCurve";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_PhotoEffect:
            *m_err << "Incoming property event: PhotoEffect";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FilterEffect:
            *m_err << "Incoming property event: FilterEffect";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ToningEffect:
            *m_err << "Incoming property event: ToningEffect";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ParameterSet:
            *m_err << "Incoming property event: ParameterSet";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ColorMatrix:
            *m_err << "Incoming property event: ColorMatrix";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_PictureStyle:
            *m_err << "Incoming property event: PictureStyle";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_PictureStyleDesc:
            *m_err << "Incoming property event: PictureStyleDesc";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ETTL2Mode:
            *m_err << "Incoming property event: ETTL2Mode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_PictureStyleCaption:
            *m_err << "Incoming property event: PictureStyleCaption";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Linear:
            *m_err << "Incoming property event: Linear";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ClickWBPoint:
            *m_err << "Incoming property event: ClickWBPoint";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_WBCoeffs:
            *m_err << "Incoming property event: WBCoeffs";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_GPSVersionID:
            *m_err << "Incoming property event: GPSVersionID";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSLatitudeRef:
            *m_err << "Incoming property event: GPSLatitudeRef";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSLatitude:
            *m_err << "Incoming property event: GPSLatitude";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSLongitudeRef:
            *m_err << "Incoming property event: GPSLongitudeRef";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSLongitude:
            *m_err << "Incoming property event: GPSLongitude";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSAltitudeRef:
            *m_err << "Incoming property event: GPSAltitudeRef";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSAltitude:
            *m_err << "Incoming property event: GPSAltitude";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSTimeStamp:
            *m_err << "Incoming property event: GPSTimeStamp";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSSatellites:
            *m_err << "Incoming property event: GPSSatellites";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSStatus:
            *m_err << "Incoming property event: GPSStatus";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSMapDatum:
            *m_err << "Incoming property event: GPSMapDatum";
            pushErrMsg(Debug);
			break;
		case    kEdsPropID_GPSDateStamp:
            *m_err << "Incoming property event: GPSDateStamp";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_AtCapture_Flag:
            *m_err << "Incoming property event: AtCapture_Flag";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_AEMode:
            *m_err << "Incoming property event: AEMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_DriveMode:
            *m_err << "Incoming property event: DriveMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ISOSpeed:
            *m_err << "Incoming property event: ISOSpeed";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_MeteringMode:
            *m_err << "Incoming property event: MeteringMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_AFMode:
            *m_err << "Incoming property event: AFMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Av:
            *m_err << "Incoming property event: Av";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Tv:
            *m_err << "Incoming property event: Tv";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ExposureCompensation:
            *m_err << "Incoming property event: ExposureCompensation";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FlashCompensation:
            *m_err << "Incoming property event: FlashCompensation";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FocalLength:
            *m_err << "Incoming property event: FocalLength";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_AvailableShots:
            *m_err << "Incoming property event: AvailableShots";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Bracket:
            *m_err << "Incoming property event: Bracket";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_WhiteBalanceBracket:
            *m_err << "Incoming property event: WhiteBalancingBracket";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_LensName:
            *m_err << "Incoming property event: LensName";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_AEBracket:
            *m_err << "Incoming property event: AEBracket";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FEBracket:
            *m_err << "Incoming property event: FEBracket";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_ISOBracket:
            *m_err << "Incoming property event: ISOBracket";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_NoiseReduction:
            *m_err << "Incoming property event: NoiseReduction";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FlashOn:
            *m_err << "Incoming property event: FlashOn";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_RedEye:
            *m_err << "Incoming property event: RedEye";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_FlashMode:
            *m_err << "Incoming property event: FlashMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_LensStatus:
            *m_err << "Incoming property event: LensStatus";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Artist:
            *m_err << "Incoming property event: Artist";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Copyright:
            *m_err << "Incoming property event: Copyright";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_DepthOfField:
            *m_err << "Incoming property event: DepthOfField";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_EFCompensation:
            *m_err << "Incoming property event: EFCompensation";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_OutputDevice:
            if (inParam == kEdsEvfOutputDevice_PC) {
                *m_err << "notice from camera: we are now in live view mode.";
                pushErrMsg(Debug);
            } else if (inParam == kEdsEvfOutputDevice_TFT) {
                *m_err << "notice from camera: we are no longer in live view mode.";
                pushErrMsg(Debug);
            } else {
                // should not get here
                *m_err << "notice from camera: we are now in WTF mode.";
                pushErrMsg(Warning);
            }
            switch (m_liveView->m_state) {
                case LiveView::Off:
                    *m_err << "live view changed but we didn't expect it to. live view should be off";
                    pushErrMsg(Warning);
                    break;
                case LiveView::On:
                    *m_err << "live view changed but we didn't expect it to. live view should be on";
                    pushErrMsg(Warning);
                    break;
                case LiveView::Paused:
                    *m_err << "live view changed but we didn't expect it to. live view should be paused";
                    pushErrMsg(Warning);
                    break;
                case LiveView::WaitingToStart:
                    m_liveView->m_state = LiveView::On;
                    switch (m_liveView->m_desiredNewState) {
                        case LiveView::Off:
                            stopLiveView();
                            break;
                        case LiveView::On:
                            break;
                        case LiveView::Paused:
                            pauseLiveView();
                            break;
                        default:
                            assert(false);
                    }
                    break;
                case LiveView::WaitingToStop:
                    m_liveView->m_state = LiveView::Off;
                    switch (m_liveView->m_desiredNewState) {
                        case LiveView::Off:
                            break;
                        case LiveView::On:
                            startLiveView();
                            break;
                        case LiveView::Paused:
                            m_liveView->m_state = LiveView::Paused;
                            break;
                        default:
                            assert(false);
                    }
                    break;
            }
            break;
		case kEdsPropID_Evf_Mode:
            *m_err << "Incoming property event: EvfMode";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_WhiteBalance:
            *m_err << "Incoming property event: WhiteBalance";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_ColorTemperature:
            *m_err << "Incoming property event: ColorTemperature";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_DepthOfFieldPreview:
            *m_err << "Incoming property event: DepthOfFieldPreview";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_Zoom:
            *m_err << "Incoming property event: Zoom";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_ZoomPosition:
            *m_err << "Incoming property event: ZoomPosition";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_FocusAid:
            *m_err << "Incoming property event: FocusAid";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_Histogram:
            *m_err << "Incoming property event: Histogram";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_ImagePosition:
            *m_err << "Incoming property event: ImagePosition";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_HistogramStatus:
            *m_err << "Incoming property event: HistogramStatus";
            pushErrMsg(Debug);
			break;
		case kEdsPropID_Evf_AFMode:
            *m_err << "Incoming property event: AFMode";
            pushErrMsg(Debug);
			break;
        default:
            *m_err << "Unrecognized prop id: " << inPropertyID;
            pushErrMsg(Warning);
            assert(false);
    }
}

bool Camera::transferOneItem(EdsBaseRef inRef, string outfile)
{
    // transfer the image in memory to disk
    EdsDirectoryItemInfo dirItemInfo;
    EdsStreamRef outStream = NULL;

    EdsError err;

    err = EdsGetDirectoryItemInfo(inRef, &dirItemInfo);

    if (err) {
        *m_err << "Unable to get directory item info: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    // get a temp file to write to
    string tmpfile = tmpnam(NULL);

    // this creates the outStream that is used by EdsDownload to actually
    // grab and write out the file
    err = EdsCreateFileStream(tmpfile.c_str(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &outStream);

    if (err) {
        *m_err << "Unable to create file stream: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    if (! outStream) {
        *m_err << "Create file stream didn't allocate a stream for us.";
        pushErrMsg();
        return false;
    }

    // do the transfer
    err = EdsDownload(inRef, dirItemInfo.size, outStream);
 
    if (err) {
        *m_err << "Unable to download picture: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        EdsRelease(outStream);
        return false;
    }

    err = EdsDownloadComplete(inRef);

    if (err) {
        *m_err << "Unable to finish downloading picture: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        EdsRelease(outStream);
        return false;
    }

    // clean up
    err = EdsRelease(outStream);

    if (err) {
        *m_err << "Unable to release out stream after downloading: " << ErrorMap::errorMsg(err);
        pushErrMsg(Warning);
    }

    // make sure we don't overwrite files
    ensurePathExists(getDirectoryName(outfile));
    outfile = makeUnique(outfile);
    moveFile(tmpfile, outfile);

    resumeLiveView();

    if (m_pictureCompleteCallback)
        m_pictureCompleteCallback(outfile);

    m_pictureDoneQueue.push(outfile);

    return true;
}

bool Camera::pauseLiveView()
{
    switch (m_liveView->m_state) {
        case LiveView::Paused:
        case LiveView::Off:
            // nothing to do
            return true;
        case LiveView::On:
            if (stopLiveView()) {
                m_liveView->m_state = LiveView::Paused;
                return true;
            } else {
                return false;
            }
        case LiveView::WaitingToStart:
        case LiveView::WaitingToStop:
            m_liveView->m_desiredNewState = LiveView::Paused;
            return true;
    }
    assert(false);
    return false;
}

bool Camera::resumeLiveView()
{
    switch (m_liveView->m_state) {
        case LiveView::Off:
        case LiveView::On:
            // we don't need to do anything
            return true;
        case LiveView::Paused:
            // this is the normal way to resume
            return startLiveView();
        case LiveView::WaitingToStop:
        case LiveView::WaitingToStart:
            m_liveView->m_desiredNewState = LiveView::On;
            return true;
    }
    assert(false);
    return false;
}

bool Camera::setComputerCapabilities()
{
    // tell the camera how much disk space we have left
    EdsCapacity caps;

    caps.reset = true;
    caps.bytesPerSector = 512;
    caps.numberOfFreeClusters = 2147483647; // arbitrary large number
    EdsError err = EdsSetCapacity(m_cam, caps);

    if (err) {
        *m_err << "Unable to set computer capabilities: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    return true;
}

bool Camera::takeSinglePicture(string outFile)
{
    pauseLiveView();
    m_picOutFile = outFile;

    if (! setComputerCapabilities())
        return false;

    // take a picture with the camera and save it to outfile
    EdsError err = EdsSendCommand(m_cam, kEdsCameraCommand_TakePicture, 0);

    if (err == EDS_ERR_OBJECT_NOTREADY) {
        *m_err << "unable to take picture, camera not ready";
        pushErrMsg(Warning);
        return false;
    } else if (err) {
        *m_err << "unable to take picture: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    return true;
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

EdsWhiteBalance Camera::whiteBalance()
{
    EdsError err = EdsGetPropertyData(m_cam, kEdsPropID_WhiteBalance, 0, sizeof(m_whiteBalance), (EdsVoid *) &m_whiteBalance);
    if (err) {
        *m_err << "Unable to get white balance: " << ErrorMap::errorMsg(err);
        pushErrMsg();
    }
    return m_whiteBalance;
}
void Camera::setWhiteBalance(EdsWhiteBalance whiteBalance)
{
    m_whiteBalance = whiteBalance;
    m_pendingWhiteBalance = true;
}

string Camera::name()
{
    EdsDeviceInfo deviceInfo;
    EdsError err = EdsGetDeviceInfo(m_cam, &deviceInfo);

    if (err) {
        *m_err << "Unable to get device info: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return string();
    }

    return deviceInfo.szDeviceDescription;
}

void Camera::setPictureCompleteCallback(takePictureCompleteCallback callback)
{
    m_pictureCompleteCallback = callback;
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
    return m_liveView->m_frameBuffer;
}

int Camera::liveViewFrameBufferSize() const
{
    return m_liveView->c_frameBufferSize;
}

bool Camera::grabLiveViewFrame()
{
    // skip frames if the camera isn't ready yet.
    if (m_liveView->m_state != LiveView::On) {
        *m_err << "Skipping live view frame because camera is not in live view mode";
        if (m_liveView->m_state == LiveView::WaitingToStart)
            *m_err << " yet";
        pushErrMsg(Warning);
        return false;
    }

    EdsError err;
    EdsImageRef img = NULL;

    // create image
    err = EdsCreateEvfImageRef(m_liveView->m_streamPtr, &img);
    if (err) {
        *m_err << "Unable to create live view frame on the camera: " << ErrorMap::errorMsg(err);
        pushErrMsg(Error);
        return false;
    }
    if (! img) {
        *m_err << "EDSDK didn't give us an image ref for live view frame";
        pushErrMsg(Error);
        return false;
    }

    // download the frame
    err = EdsDownloadEvfImage(m_cam, img);

    if (err == EDS_ERR_OBJECT_NOTREADY) {
        // skip the frame if the camera isn't ready
        *m_err << "skipping live view frame because camera isn't ready";
        pushErrMsg(Warning);
        EdsRelease(img);
        return false;
    } else if (err) {
        // skip the frame. unknown error
        *m_err << "skipping live view frame: " << ErrorMap::errorMsg(err);
        pushErrMsg(Error);
        EdsRelease(img);
        return false;
    }

    // get/set zoom ratio
    if (m_pendingZoomRatio) {
        err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &m_zoomRatio);
        if (err) {
            *m_err << "Unable to set zoom ratio: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        } else {
            m_pendingZoomRatio = false;
        }
    } else {
        err = EdsGetPropertyData(img, kEdsPropID_Evf_Zoom, 0, sizeof(EdsUInt32), &m_zoomRatio);
        if (err) {
            *m_err << "Unable to get zoom ratio: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        }
    }

    // get/set zoom position
    if (m_pendingZoomPosition) {
        err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &m_pendingZoomPoint);
        if (err) {
            *m_err << "Unable to set zoom position: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        } else {
            m_pendingZoomPosition = false;
        }
    } else {
        err = EdsGetPropertyData(img, kEdsPropID_Evf_ZoomPosition, 0, sizeof(EdsPoint), &m_zoomPosition);
        if (err) {
            *m_err << "Unable to get zoom position: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        }
    }

    // set white balance
    if (m_pendingWhiteBalance) {
        err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_WhiteBalance, 0, sizeof(EdsWhiteBalance), &m_whiteBalance);
        if (err) {
            *m_err << "Unable to set white balance: " << ErrorMap::errorMsg(err);
            pushErrMsg(Warning);
        } else {
            m_pendingWhiteBalance = false;
        }
    }

    err = EdsRelease(img);
    if (err) {
        *m_err << "Unable to release live view frame: " << ErrorMap::errorMsg(err);
        pushErrMsg(Warning);
    }

    return true;
}

bool Camera::startLiveView()
{
    switch (m_liveView->m_state) {
        case LiveView::Paused:
            *m_err << "startLiveView(): Live view paused, will resume in a moment.";
            pushErrMsg(Warning);
            return true;
        case LiveView::Off:
            // tell the computer to send live data to the computer
            EdsError err;
            EdsUInt32 device;
            err = EdsGetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

            if (err) {
                *m_err << "Unable to get live view output device: " << ErrorMap::errorMsg(err);
                pushErrMsg();
                return false;
            }

            device |= kEdsEvfOutputDevice_PC;
            err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

            if (err) {
                *m_err << "Unable to turn on live view: " << ErrorMap::errorMsg(err);
                pushErrMsg();
            }

            *m_err << "Requested live view to turn on.";
            pushErrMsg(Debug);
            m_liveView->m_state = LiveView::WaitingToStart;
            m_liveView->m_desiredNewState = LiveView::On;
            return true;
        case LiveView::WaitingToStop:
            *m_err << "Waiting for live view to end so we can start it again.";
            pushErrMsg(Debug);
            m_liveView->m_desiredNewState = LiveView::On;
            return true;
        case LiveView::WaitingToStart:
            *m_err << "startLiveView(): already waiting to start live view.";
            pushErrMsg(Warning);
            m_liveView->m_desiredNewState = LiveView::On;
            return true;
        case LiveView::On:
            *m_err << "startLiveView(): Live view already on";
            pushErrMsg(Warning);
            return true;
    }
    assert(false);
    return false;
}

bool Camera::stopLiveView()
{
    switch (m_liveView->m_state) {
        case LiveView::Paused:
            *m_err << "stopLiveView(): Live view already paused";
            pushErrMsg(Debug);
            m_liveView->m_state = LiveView::Off;
            return true;
        case LiveView::Off:
            *m_err << "stopLiveView(): Live view already off";
            pushErrMsg(Debug);
            return true;
        case LiveView::WaitingToStop:
        case LiveView::WaitingToStart:
            m_liveView->m_desiredNewState = LiveView::Off;
            return true;
        case LiveView::On:
            // tell the camera to stop sending live data to the computer
            EdsError err = EDS_ERR_OK;
            EdsUInt32 device;
            err = EdsGetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

            if (err) {
                *m_err << "Unable to get live view output device: " << ErrorMap::errorMsg(err);
                pushErrMsg();
                return false;
            }

            device &= ~kEdsEvfOutputDevice_PC;
            err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_OutputDevice, 0, sizeof(EdsUInt32), &device);

            if (err) {
                *m_err << "Unable to turn off live view: " << ErrorMap::errorMsg(err);
                pushErrMsg();
                return false;
            }

            m_liveView->m_state = LiveView::WaitingToStop;
            m_liveView->m_desiredNewState = LiveView::Off;
            return true;
    }
    assert(false);
    return false;
}

EdsSize Camera::liveViewImageSize() const
{
    return m_liveView->m_imageSize;
}

bool Camera::autoFocus()
{
    EdsUInt32 off = (EdsUInt32) kEdsEvfDepthOfFieldPreview_OFF;
    EdsError err;

    EdsUInt32 currentDepth;
    err = EdsGetPropertyData(m_cam, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(EdsUInt32), &currentDepth);
    if (err) {
        *m_err << "Unable to get depth of field preview: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }
    if (currentDepth != kEdsEvfDepthOfFieldPreview_OFF) {
        // turn OFF depth of field preview
        err = EdsSetPropertyData(m_cam, kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof(EdsUInt32), &off);

        if (err) {
            *m_err << "Unable to set depth of field preview: " << ErrorMap::errorMsg(err);
            pushErrMsg();
            return false;
        }
    }

    err = EdsSendCommand(m_cam, (EdsUInt32)kEdsCameraCommand_DoEvfAf, (EdsUInt32)Evf_AFMode_Quick);
    if (err) {
        *m_err << "ERROR: Unable to set camera to AF mode quick: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    err = EdsSendCommand(m_cam, (EdsUInt32)kEdsCameraCommand_DoEvfAf, (EdsUInt32)Evf_AFMode_Live);
    if (err) {
        *m_err << "ERROR: Unable to set camera to AF mode live: " << ErrorMap::errorMsg(err);
        pushErrMsg();
        return false;
    }

    return true;
}

void Camera::terminate()
{
    EdsTerminateSDK();
    s_initialized = false;
}

void Camera::pushErrMsg(ErrorLevel level)
{
    if (level >= m_errorLevel) {
        ErrorMessage msg;
        msg.level = level;
        msg.msg = m_err->str();
        m_errMsgQueue.push(msg);
    }

    delete m_err;
    m_err = new stringstream;
}

int Camera::errMsgQueueSize() const
{
    return m_errMsgQueue.size();
}

Camera::ErrorMessage Camera::popErrMsg()
{
    if (errMsgQueueSize() == 0) {
        return ErrorMessage();
    } else {
        ErrorMessage value = m_errMsgQueue.front();
        m_errMsgQueue.pop();
        return value;
    }
}

void Camera::setErrorLevel(ErrorLevel level)
{
    m_errorLevel = level;
}
