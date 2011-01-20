#include <Python.h>

#include "Camera.h"

extern "C" {
    static PyObject * CameraError;

    typedef struct {
        PyObject_HEAD
        Camera * camera; // C++ object
    } CameraObject;

    static void Camera_dealloc(CameraObject * self);
    static int Camera_getbuffer(CameraObject * self, PyObject * view, int flags);
    static void Camera_releasebuffer(CameraObject * self, PyObject * view);

    static int Camera_getbuffer(CameraObject * self, PyObject * _view, int flags);
    static void Camera_releasebuffer(CameraObject * self, PyObject * view);
    static PyBufferProcs Camera_bufferProcs = {(getbufferproc)Camera_getbuffer, (releasebufferproc)Camera_releasebuffer};

    static PyObject * Camera_connect(CameraObject * self, PyObject * args);
    static PyObject * Camera_disconnect(CameraObject * self, PyObject * args);
    
    static PyObject * Camera_name(CameraObject * self, PyObject * args);
    static PyObject * Camera_takeSinglePicture(CameraObject * self, PyObject * args);
    static PyObject * Camera_startLiveView(CameraObject * self, PyObject * args);
    static PyObject * Camera_stopLiveView(CameraObject * self, PyObject * args);
    static PyObject * Camera_zoomRatio(CameraObject * self, PyObject * args);
    static PyObject * Camera_setZoomRatio(CameraObject * self, PyObject * args);
    static PyObject * Camera_whiteBalance(CameraObject * self, PyObject * args);
    static PyObject * Camera_setWhiteBalance(CameraObject * self, PyObject * args);
    static PyObject * Camera_meteringMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_setMeteringMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_driveMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_setDriveMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_afMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_setAFMode(CameraObject * self, PyObject * args);
    static PyObject * Camera_exposureCompensation(CameraObject * self, PyObject * args);
    static PyObject * Camera_setExposureCompensation(CameraObject * self, PyObject * args);

    static PyObject * Camera_popPictureDoneQueue(CameraObject * self, PyObject * args);
    static PyObject * Camera_pictureDoneQueueSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_grabLiveViewFrame(CameraObject * self, PyObject * args);
    static PyObject * Camera_autoFocus(CameraObject * self, PyObject * args);

    static PyObject * Camera_liveViewImageSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_maxZoomPosition(CameraObject * self, PyObject * args);
    static PyObject * Camera_zoomBoxSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_zoomPosition(CameraObject * self, PyObject * args);
    static PyObject * Camera_setZoomPosition(CameraObject * self, PyObject * args);
    static PyMethodDef CameraMethods[] = {
        {"connect",             (PyCFunction)Camera_connect,             METH_VARARGS, "establish a session on the camera"},
        {"disconnect",          (PyCFunction)Camera_disconnect,          METH_VARARGS, "release the session with the camera"},

        {"name",                (PyCFunction)Camera_name,                METH_VARARGS, "Return the model name of the camera"},
        {"takeSinglePicture",   (PyCFunction)Camera_takeSinglePicture,   METH_VARARGS, "takes one picture to file specified."},
        {"startLiveView",       (PyCFunction)Camera_startLiveView,       METH_VARARGS, "tells the camera to go into live view mode"},
        {"stopLiveView",        (PyCFunction)Camera_stopLiveView,        METH_VARARGS, "tells the camera to come out of live view mode"},
        {"autoFocus",           (PyCFunction)Camera_autoFocus,           METH_VARARGS, "performs an auto focus once right now"},

        {"liveViewImageSize",   (PyCFunction)Camera_liveViewImageSize,   METH_VARARGS, "returns (w, h) of the image data coming from live view."},
        {"maxZoomPosition",     (PyCFunction)Camera_maxZoomPosition,     METH_VARARGS, "returns (x, y) of what you can set zoom position to."},
        {"zoomBoxSize",         (PyCFunction)Camera_zoomBoxSize,         METH_VARARGS, "returns (w, h) of the zoom rectangle that represents zoom position."},
        {"zoomPosition",        (PyCFunction)Camera_zoomPosition,        METH_VARARGS, "returns (x, y) of the zoom position when zoomed in in live view."},
        {"setZoomPosition",     (PyCFunction)Camera_setZoomPosition,     METH_VARARGS, "sets the zoom position when zoomed in in live view."},

        {"zoomRatio",           (PyCFunction)Camera_zoomRatio,           METH_VARARGS, "returns the zoom factor."},
        {"setZoomRatio",        (PyCFunction)Camera_setZoomRatio,        METH_VARARGS, "sets the zoom factor"},
        {"whiteBalance",        (PyCFunction)Camera_whiteBalance,        METH_VARARGS, "returns the white balance property. this is an enum defined in edsdk."},
        {"setWhiteBalance",     (PyCFunction)Camera_setWhiteBalance,     METH_VARARGS, "sets the white balance property"},
        {"meteringMode",        (PyCFunction)Camera_meteringMode,        METH_VARARGS, "returns the metering mode property"},
        {"setMeteringMode",     (PyCFunction)Camera_setMeteringMode,     METH_VARARGS, "sets the metering mode property"},
        {"driveMode",           (PyCFunction)Camera_driveMode,           METH_VARARGS, "returns the drive mode property"},
        {"setDriveMode",        (PyCFunction)Camera_setDriveMode,        METH_VARARGS, "sets the drive mode property"},
        {"afMode",              (PyCFunction)Camera_afMode,              METH_VARARGS, "returns the AF mode property"},
        {"setAFMode",           (PyCFunction)Camera_setAFMode,           METH_VARARGS, "sets the AF mode property"},
        {"exposureCompensation",(PyCFunction)Camera_exposureCompensation,METH_VARARGS, "returns the exposure compensation property"},
        {"setExposureCompensation",(PyCFunction)Camera_setExposureCompensation,METH_VARARGS, "sets the exposure compensation property"},

        {"popPictureDoneQueue", (PyCFunction)Camera_popPictureDoneQueue, METH_VARARGS, "pops the oldest picture that is completed."},
        {"pictureDoneQueueSize",(PyCFunction)Camera_pictureDoneQueueSize,METH_VARARGS, "checks how many pictures are in the completed queue."},
        {"grabLiveViewFrame",   (PyCFunction)Camera_grabLiveViewFrame,   METH_VARARGS, "refresh the frame buffer with a new frame from the camera."},

        {NULL, NULL, 0, NULL} // sentinel
    };


    static PyTypeObject Camera_Type = {
        /* The ob_type field must be initialized in the module init function
         * to be portable to Windows without using C++. */
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "Camera",                       /*tp_name*/
        sizeof(CameraObject),           /*tp_basicsize*/
        0,                              /*tp_itemsize*/
        // methods to implement standard operations
        (destructor)Camera_dealloc,     /*tp_dealloc*/
        0,                              /*tp_print*/
        0,                              /*tp_getattr*/
        0,                              /*tp_setattr*/
        0,                              /*tp_reserved*/
        0,                              /*tp_repr*/
        // method suites for standard classes
        0,                              /*tp_as_number*/
        0,                              /*tp_as_sequence*/
        0,                              /*tp_as_mapping*/
        // more standard operations (here for binary compatibility)
        0,                              /*tp_hash*/
        0,                              // tp_call
        0,                              // tp_str
        PyObject_GenericGetAttr,        // tp_getattro
        PyObject_GenericSetAttr,        // tp_setattro
        // functions to access object as input/output buffer
        &Camera_bufferProcs,            // tp_as_buffer
        0,                              // tp_flags
        "A camera class which you can use to control the actual camera.", // tp_doc
        0,                              // tp_traverse
        0,                              // tp_clear
        0,                              // tp_richcompare
        0,                              // tp_weaklistoffset
        0,                              // tp_iter
        0,                              // tp_iternext
        CameraMethods,                  // tp_methods
    };

#define CameraObject_Check(v) ((v)->ob_type == &Camera_Type)

    // Camera methods

    static void Camera_dealloc(CameraObject * self)
    {
        delete self->camera;
        PyObject_FREE(self);
    }

    static PyObject * Camera_connect(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->connect())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_disconnect(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->disconnect())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_name(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        return Py_BuildValue("s", self->camera->name().c_str());
    }

    static PyObject * Camera_takeSinglePicture(CameraObject * self, PyObject * args)
    {
        char * outFile;
        if (! PyArg_ParseTuple(args, "s", &outFile))
            return NULL;

        if (self->camera->takeSinglePicture(outFile))
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_startLiveView(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->startLiveView())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_stopLiveView(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->stopLiveView())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_liveViewImageSize(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        EdsSize size = self->camera->cameraSpecificData()->zoom100ImageSize;

        return Py_BuildValue("ii", size.width, size.height);
    }

    static PyObject * Camera_maxZoomPosition(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        EdsPoint pt = self->camera->cameraSpecificData()->zoom100MaxPosition;

        return Py_BuildValue("ii", pt.x, pt.y);
    }

    static PyObject * Camera_zoomBoxSize(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        EdsSize size = self->camera->cameraSpecificData()->zoomBoxSize;

        return Py_BuildValue("ii", size.width, size.height);
    }

    static PyObject * Camera_zoomPosition(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        EdsPoint point = self->camera->zoomPosition();

        PyObject * ret = Py_BuildValue("ii", point.x, point.y);
        return ret;
    }

    static PyObject * Camera_setZoomPosition(CameraObject * self, PyObject * args)
    {
        int x, y;
        if (! PyArg_ParseTuple(args, "ii", &x, &y))
            return NULL;

        EdsPoint point;
        point.x = x;
        point.y = y;
        self->camera->setZoomPosition(point);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_zoomRatio(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int zoomRatio = self->camera->zoomRatio();

        return Py_BuildValue("i", zoomRatio);
    }

    static PyObject * Camera_setZoomRatio(CameraObject * self, PyObject * args)
    {
        int zoomRatio;
        if (! PyArg_ParseTuple(args, "i", &zoomRatio))
            return NULL;

        self->camera->setZoomRatio(zoomRatio);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_whiteBalance(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int whiteBalance = self->camera->whiteBalance();

        return Py_BuildValue("i", whiteBalance);
    }

    static PyObject * Camera_setWhiteBalance(CameraObject * self, PyObject * args)
    {
        int whiteBalance;
        if (! PyArg_ParseTuple(args, "i", &whiteBalance))
            return NULL;

        self->camera->setWhiteBalance((EdsWhiteBalance)whiteBalance);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_meteringMode(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int mode = self->camera->meteringMode();

        return Py_BuildValue("i", mode);
    }

    static PyObject * Camera_setMeteringMode(CameraObject * self, PyObject * args)
    {
        int mode;
        if (! PyArg_ParseTuple(args, "i", &mode))
            return NULL;

        self->camera->setMeteringMode((Camera::MeteringMode)mode);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_driveMode(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int mode = self->camera->driveMode();

        return Py_BuildValue("i", mode);
    }

    static PyObject * Camera_setDriveMode(CameraObject * self, PyObject * args)
    {
        int mode;
        if (! PyArg_ParseTuple(args, "i", &mode))
            return NULL;

        self->camera->setDriveMode((Camera::DriveMode)mode);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_afMode(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int mode = self->camera->afMode();

        return Py_BuildValue("i", mode);
    }

    static PyObject * Camera_setAFMode(CameraObject * self, PyObject * args)
    {
        int mode;
        if (! PyArg_ParseTuple(args, "i", &mode))
            return NULL;

        self->camera->setAFMode((Camera::AFMode)mode);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_exposureCompensation(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        float value = self->camera->exposureCompensation();

        return Py_BuildValue("f", value);
    }

    static PyObject * Camera_setExposureCompensation(CameraObject * self, PyObject * args)
    {
        float value;
        if (! PyArg_ParseTuple(args, "f", &value))
            return NULL;

        self->camera->setExposureCompensation(value);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_popPictureDoneQueue(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        return Py_BuildValue("s", self->camera->popPictureDoneQueue().c_str());
    }

    static PyObject * Camera_pictureDoneQueueSize(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int count = self->camera->pictureDoneQueueSize();

        return Py_BuildValue("i", count);
    }

    static PyObject * Camera_grabLiveViewFrame(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;
        
        if (self->camera->grabLiveViewFrame())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    static PyObject * Camera_autoFocus(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->autoFocus())
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }


    // -----

    static int Camera_getbuffer(CameraObject * self, PyObject * _view, int flags)
    {

        Py_buffer * view = (Py_buffer *) _view;
        view->buf = (void *) self->camera->liveViewFrameBuffer();
        view->len = self->camera->liveViewFrameBufferSize();
        view->readonly = 1;
        view->format = "B";
        view->ndim = 1;
        view->shape = new Py_ssize_t[1];
        view->shape[0] = view->len;
        view->strides = new Py_ssize_t[1];
        view->strides[0] = 1;
        view->suboffsets = new Py_ssize_t[1];
        view->suboffsets[0] = -1;
        view->itemsize = 1;

        // wtf do we return?
        return 0;
    }

    static void Camera_releasebuffer(CameraObject * self, PyObject * _view)
    {
        Py_buffer * view = (Py_buffer *) _view;
        delete[] view->shape;
        delete[] view->strides;
        delete[] view->suboffsets;
    }
    /* --------------------------------------------------------------------- */

    /* Function of no arguments returning new Camera object */
    static PyObject * camera_getFirstCamera(PyObject * , PyObject * )
    {
        CameraObject * self;
        self = PyObject_NEW(CameraObject, &Camera_Type);
        if (self == NULL)
            return NULL;

        self->camera = Camera::getFirstCamera();

        if (self->camera == NULL) {
            Camera_dealloc(self);
            Py_RETURN_NONE;
        }

        return (PyObject *) self;
    }

    static PyObject * camera_terminate(PyObject * , PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;
        
        Camera::terminate();

        Py_RETURN_NONE;
    }

    static PyObject * camera_setErrorLevel(CameraObject * self, PyObject * args)
    {
        int level;

        if (! PyArg_ParseTuple(args, "i", &level))
            return NULL;

        Camera::setErrorLevel((Camera::ErrorLevel)level);

        Py_RETURN_NONE;
    }

    static PyObject * camera_popErrMsg(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        Camera::ErrorMessage msg = Camera::popErrMsg();

        return Py_BuildValue("(i,s)", msg.level, msg.msg.c_str());
    }

    static PyObject * camera_errMsgQueueSize(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        int count = Camera::errMsgQueueSize();

        return Py_BuildValue("i", count);
    }


    /* List of functions defined in the module */
    static PyMethodDef cameraMethods[] = {
        {"getFirstCamera",      (PyCFunction)camera_getFirstCamera,      METH_VARARGS, "return a Camera object using the first camera we can find"},
        {"terminate",           (PyCFunction)camera_terminate,           METH_VARARGS, "call EdsTerminateSDK and start over"},
        {"setErrorLevel",       (PyCFunction)camera_setErrorLevel,       METH_VARARGS, "set which error messages will be added to the queue"},
        {"popErrMsg",           (PyCFunction)camera_popErrMsg,           METH_VARARGS, "pops the oldest error message that was generated"},
        {"errMsgQueueSize",     (PyCFunction)camera_errMsgQueueSize,     METH_VARARGS, "returns the amount of error messages in the queue"},

        {NULL, NULL, 0, NULL}       // sentinel
    };

    static PyModuleDef cameraModule = {
        PyModuleDef_HEAD_INIT,
        "Camera",                                      // name of module
        "Python library to control cameras via EDSDK", // module documentation
        // size of per-interpreter state of the module, or -1 if
        // the module keeps state in global variables.
        -1,
        cameraMethods
    };

    // initialization function for the module.
    // the name that gets exported here matters.
    PyMODINIT_FUNC
    PyInit_Camera()
    {
        PyObject * m;

        m = PyModule_Create(&cameraModule);
        if (m == NULL)
            return NULL;

        // create the custom error
        CameraError = PyErr_NewException("Camera.error", NULL, NULL);
        Py_INCREF(CameraError);
        PyModule_AddObject(m, "error", CameraError);

        return m;
    }
}

