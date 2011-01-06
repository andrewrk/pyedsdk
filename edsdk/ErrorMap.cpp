#include "ErrorMap.h"

#include <cassert>

bool ErrorMap::s_initialized = false;
map<EdsError, string> ErrorMap::s_messages;

void ErrorMap::initialize()
{
    if (s_initialized)
        return;
    s_initialized = true;


    // vim command to convert defines into error messages :)
    // %s/#define \(\w\+\)\s\+\(.\+\)$/\ts_messages[\2] = "\1";/g
/*-----------------------------------------------------------------------
   ED-SDK Error Code Masks
------------------------------------------------------------------------*/
	s_messages[0x80000000L] = "EDS_ISSPECIFIC_MASK";
	s_messages[0x7F000000L] = "EDS_COMPONENTID_MASK";
	s_messages[0x00FF0000L] = "EDS_RESERVED_MASK";
	s_messages[0x0000FFFFL] = "EDS_ERRORID_MASK";

/*-----------------------------------------------------------------------
   ED-SDK Base Component IDs
------------------------------------------------------------------------*/
	s_messages[0x01000000L] = "EDS_CMP_ID_CLIENT_COMPONENTID";
	s_messages[0x02000000L] = "EDS_CMP_ID_LLSDK_COMPONENTID";
	s_messages[0x03000000L] = "EDS_CMP_ID_HLSDK_COMPONENTID";

/*-----------------------------------------------------------------------
   ED-SDK Functin Success Code
------------------------------------------------------------------------*/
	s_messages[0x00000000L] = "EDS_ERR_OK";

/*-----------------------------------------------------------------------
   ED-SDK Generic Error IDs
------------------------------------------------------------------------*/
/* Miscellaneous errors */
	s_messages[0x00000001L] = "EDS_ERR_UNIMPLEMENTED";
	s_messages[0x00000002L] = "EDS_ERR_INTERNAL_ERROR";
	s_messages[0x00000003L] = "EDS_ERR_MEM_ALLOC_FAILED";
	s_messages[0x00000004L] = "EDS_ERR_MEM_FREE_FAILED";
	s_messages[0x00000005L] = "EDS_ERR_OPERATION_CANCELLED";
	s_messages[0x00000006L] = "EDS_ERR_INCOMPATIBLE_VERSION";
	s_messages[0x00000007L] = "EDS_ERR_NOT_SUPPORTED";
	s_messages[0x00000008L] = "EDS_ERR_UNEXPECTED_EXCEPTION";
	s_messages[0x00000009L] = "EDS_ERR_PROTECTION_VIOLATION";
	s_messages[0x0000000AL] = "EDS_ERR_MISSING_SUBCOMPONENT";
	s_messages[0x0000000BL] = "EDS_ERR_SELECTION_UNAVAILABLE";

/* File errors */
	s_messages[0x00000020L] = "EDS_ERR_FILE_IO_ERROR";
	s_messages[0x00000021L] = "EDS_ERR_FILE_TOO_MANY_OPEN";
	s_messages[0x00000022L] = "EDS_ERR_FILE_NOT_FOUND";
	s_messages[0x00000023L] = "EDS_ERR_FILE_OPEN_ERROR";
	s_messages[0x00000024L] = "EDS_ERR_FILE_CLOSE_ERROR";
	s_messages[0x00000025L] = "EDS_ERR_FILE_SEEK_ERROR";
	s_messages[0x00000026L] = "EDS_ERR_FILE_TELL_ERROR";
	s_messages[0x00000027L] = "EDS_ERR_FILE_READ_ERROR";
	s_messages[0x00000028L] = "EDS_ERR_FILE_WRITE_ERROR";
	s_messages[0x00000029L] = "EDS_ERR_FILE_PERMISSION_ERROR";
	s_messages[0x0000002AL] = "EDS_ERR_FILE_DISK_FULL_ERROR";
	s_messages[0x0000002BL] = "EDS_ERR_FILE_ALREADY_EXISTS";
	s_messages[0x0000002CL] = "EDS_ERR_FILE_FORMAT_UNRECOGNIZED";
	s_messages[0x0000002DL] = "EDS_ERR_FILE_DATA_CORRUPT";
	s_messages[0x0000002EL] = "EDS_ERR_FILE_NAMING_NA";

/* Directory errors */          
	s_messages[0x00000040L] = "EDS_ERR_DIR_NOT_FOUND";
	s_messages[0x00000041L] = "EDS_ERR_DIR_IO_ERROR";
	s_messages[0x00000042L] = "EDS_ERR_DIR_ENTRY_NOT_FOUND";
	s_messages[0x00000043L] = "EDS_ERR_DIR_ENTRY_EXISTS";
	s_messages[0x00000044L] = "EDS_ERR_DIR_NOT_EMPTY";

/* Property errors */
	s_messages[0x00000050L] = "EDS_ERR_PROPERTIES_UNAVAILABLE";
	s_messages[0x00000051L] = "EDS_ERR_PROPERTIES_MISMATCH";
	s_messages[0x00000053L] = "EDS_ERR_PROPERTIES_NOT_LOADED";

/* Function Parameter errors */     
	s_messages[0x00000060L] = "EDS_ERR_INVALID_PARAMETER";
	s_messages[0x00000061L] = "EDS_ERR_INVALID_HANDLE";
	s_messages[0x00000062L] = "EDS_ERR_INVALID_POINTER";
	s_messages[0x00000063L] = "EDS_ERR_INVALID_INDEX";
	s_messages[0x00000064L] = "EDS_ERR_INVALID_LENGTH";
	s_messages[0x00000065L] = "EDS_ERR_INVALID_FN_POINTER";
	s_messages[0x00000066L] = "EDS_ERR_INVALID_SORT_FN";

/* Device errors */
	s_messages[0x00000080L] = "EDS_ERR_DEVICE_NOT_FOUND";
	s_messages[0x00000081L] = "EDS_ERR_DEVICE_BUSY";
	s_messages[0x00000082L] = "EDS_ERR_DEVICE_INVALID";
	s_messages[0x00000083L] = "EDS_ERR_DEVICE_EMERGENCY";
	s_messages[0x00000084L] = "EDS_ERR_DEVICE_MEMORY_FULL";
	s_messages[0x00000085L] = "EDS_ERR_DEVICE_INTERNAL_ERROR";
	s_messages[0x00000086L] = "EDS_ERR_DEVICE_INVALID_PARAMETER";
	s_messages[0x00000087L] = "EDS_ERR_DEVICE_NO_DISK";
	s_messages[0x00000088L] = "EDS_ERR_DEVICE_DISK_ERROR";
	s_messages[0x00000089L] = "EDS_ERR_DEVICE_CF_GATE_CHANGED";
	s_messages[0x0000008AL] = "EDS_ERR_DEVICE_DIAL_CHANGED";
	s_messages[0x0000008BL] = "EDS_ERR_DEVICE_NOT_INSTALLED";
	s_messages[0x0000008CL] = "EDS_ERR_DEVICE_STAY_AWAKE";
	s_messages[0x0000008DL] = "EDS_ERR_DEVICE_NOT_RELEASED";


/* Stream errors */
	s_messages[0x000000A0L] = "EDS_ERR_STREAM_IO_ERROR";
	s_messages[0x000000A1L] = "EDS_ERR_STREAM_NOT_OPEN";
	s_messages[0x000000A2L] = "EDS_ERR_STREAM_ALREADY_OPEN";
	s_messages[0x000000A3L] = "EDS_ERR_STREAM_OPEN_ERROR";
	s_messages[0x000000A4L] = "EDS_ERR_STREAM_CLOSE_ERROR";
	s_messages[0x000000A5L] = "EDS_ERR_STREAM_SEEK_ERROR";
	s_messages[0x000000A6L] = "EDS_ERR_STREAM_TELL_ERROR";
	s_messages[0x000000A7L] = "EDS_ERR_STREAM_READ_ERROR";
	s_messages[0x000000A8L] = "EDS_ERR_STREAM_WRITE_ERROR";
	s_messages[0x000000A9L] = "EDS_ERR_STREAM_PERMISSION_ERROR";
	s_messages[0x000000AAL] = "EDS_ERR_STREAM_COULDNT_BEGIN_THREAD";
	s_messages[0x000000ABL] = "EDS_ERR_STREAM_BAD_OPTIONS";
	s_messages[0x000000ACL] = "EDS_ERR_STREAM_END_OF_STREAM";

/* Communications errors */
	s_messages[0x000000C0L] = "EDS_ERR_COMM_PORT_IS_IN_USE";
	s_messages[0x000000C1L] = "EDS_ERR_COMM_DISCONNECTED";
	s_messages[0x000000C2L] = "EDS_ERR_COMM_DEVICE_INCOMPATIBLE";
	s_messages[0x000000C3L] = "EDS_ERR_COMM_BUFFER_FULL";
	s_messages[0x000000C4L] = "EDS_ERR_COMM_USB_BUS_ERR";

/* Lock/Unlock */
	s_messages[0x000000D0L] = "EDS_ERR_USB_DEVICE_LOCK_ERROR";
	s_messages[0x000000D1L] = "EDS_ERR_USB_DEVICE_UNLOCK_ERROR";

/* STI/WIA */
	s_messages[0x000000E0L] = "EDS_ERR_STI_UNKNOWN_ERROR";
	s_messages[0x000000E1L] = "EDS_ERR_STI_INTERNAL_ERROR";
	s_messages[0x000000E2L] = "EDS_ERR_STI_DEVICE_CREATE_ERROR";
	s_messages[0x000000E3L] = "EDS_ERR_STI_DEVICE_RELEASE_ERROR";
	s_messages[0x000000E4L] = "EDS_ERR_DEVICE_NOT_LAUNCHED";
    
	s_messages[0x000000F0L] = "EDS_ERR_ENUM_NA";
	s_messages[0x000000F1L] = "EDS_ERR_INVALID_FN_CALL";
	s_messages[0x000000F2L] = "EDS_ERR_HANDLE_NOT_FOUND";
	s_messages[0x000000F3L] = "EDS_ERR_INVALID_ID";
	s_messages[0x000000F4L] = "EDS_ERR_WAIT_TIMEOUT_ERROR";

/* PTP */
	s_messages[0x00002003] = "EDS_ERR_SESSION_NOT_OPEN";
	s_messages[0x00002004] = "EDS_ERR_INVALID_TRANSACTIONID";
	s_messages[0x00002007] = "EDS_ERR_INCOMPLETE_TRANSFER";
	s_messages[0x00002008] = "EDS_ERR_INVALID_STRAGEID";
	s_messages[0x0000200A] = "EDS_ERR_DEVICEPROP_NOT_SUPPORTED";
	s_messages[0x0000200B] = "EDS_ERR_INVALID_OBJECTFORMATCODE";
	s_messages[0x00002011] = "EDS_ERR_SELF_TEST_FAILED";
	s_messages[0x00002012] = "EDS_ERR_PARTIAL_DELETION";
	s_messages[0x00002014] = "EDS_ERR_SPECIFICATION_BY_FORMAT_UNSUPPORTED";
	s_messages[0x00002015] = "EDS_ERR_NO_VALID_OBJECTINFO";
	s_messages[0x00002016] = "EDS_ERR_INVALID_CODE_FORMAT";
	s_messages[0x00002017] = "EDS_ERR_UNKNOWN_VENDOR_CODE";
	s_messages[0x00002018] = "EDS_ERR_CAPTURE_ALREADY_TERMINATED";
	s_messages[0x0000201A] = "EDS_ERR_INVALID_PARENTOBJECT";
	s_messages[0x0000201B] = "EDS_ERR_INVALID_DEVICEPROP_FORMAT";
	s_messages[0x0000201C] = "EDS_ERR_INVALID_DEVICEPROP_VALUE";
	s_messages[0x0000201E] = "EDS_ERR_SESSION_ALREADY_OPEN";
	s_messages[0x0000201F] = "EDS_ERR_TRANSACTION_CANCELLED";
	s_messages[0x00002020] = "EDS_ERR_SPECIFICATION_OF_DESTINATION_UNSUPPORTED";

/* PTP Vendor */
	s_messages[0x0000A001] = "EDS_ERR_UNKNOWN_COMMAND";
	s_messages[0x0000A005] = "EDS_ERR_OPERATION_REFUSED";
	s_messages[0x0000A006] = "EDS_ERR_LENS_COVER_CLOSE";
	s_messages[0x0000A101] = "EDS_ERR_LOW_BATTERY";
	s_messages[0x0000A102] = "EDS_ERR_OBJECT_NOTREADY";




	s_messages[0x00008D01L] = "EDS_ERR_TAKE_PICTURE_AF_NG";
	s_messages[0x00008D02L] = "EDS_ERR_TAKE_PICTURE_RESERVED";
	s_messages[0x00008D03L] = "EDS_ERR_TAKE_PICTURE_MIRROR_UP_NG";
	s_messages[0x00008D04L] = "EDS_ERR_TAKE_PICTURE_SENSOR_CLEANING_NG";
	s_messages[0x00008D05L] = "EDS_ERR_TAKE_PICTURE_SILENCE_NG";
	s_messages[0x00008D06L] = "EDS_ERR_TAKE_PICTURE_NO_CARD_NG";
	s_messages[0x00008D07L] = "EDS_ERR_TAKE_PICTURE_CARD_NG";
	s_messages[0x00008D08L] = "EDS_ERR_TAKE_PICTURE_CARD_PROTECT_NG";


	s_messages[0x000000F5L] = "EDS_ERR_LAST_GENERIC_ERROR_PLUS_ONE";
}

string ErrorMap::errorMsg(EdsError err)
{
    initialize();
    
    map<EdsError, string>::const_iterator it = s_messages.find(err);
    bool atEnd = (it == s_messages.end());
    assert(! atEnd);
    if (atEnd)
        return string();

    return it->second;
}
