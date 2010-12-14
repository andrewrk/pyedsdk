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
    
    static PyObject * Camera_good(CameraObject * self, PyObject * args);
    static PyObject * Camera_name(CameraObject * self, PyObject * args);
    static PyObject * Camera_takeSinglePicture(CameraObject * self, PyObject * args);
    static PyObject * Camera_startLiveView(CameraObject * self, PyObject * args);
    static PyObject * Camera_stopLiveView(CameraObject * self, PyObject * args);
    static PyObject * Camera_liveViewImageSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_zoomPosition(CameraObject * self, PyObject * args);
    static PyObject * Camera_setZoomPosition(CameraObject * self, PyObject * args);
    static PyObject * Camera_zoomRatio(CameraObject * self, PyObject * args);
    static PyObject * Camera_setZoomRatio(CameraObject * self, PyObject * args);
    static PyObject * Camera_whiteBalance(CameraObject * self, PyObject * args);
    static PyObject * Camera_setWhiteBalance(CameraObject * self, PyObject * args);
    static PyObject * Camera_popPictureDoneQueue(CameraObject * self, PyObject * args);
    static PyObject * Camera_pictureDoneQueueSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_pictureDoneQueueSize(CameraObject * self, PyObject * args);
    static PyObject * Camera_grabLiveViewFrame(CameraObject * self, PyObject * args);
    static PyObject * Camera_autoFocus(CameraObject * self, PyObject * args);
    static PyObject * Camera_pressShutterHalfway(CameraObject * self, PyObject * args);
    static PyMethodDef CameraMethods[] = {
        {"good",                (PyCFunction)Camera_good,                METH_VARARGS, "Return whether the Camera is working"},
        {"name",                (PyCFunction)Camera_name,                METH_VARARGS, "Return the model name of the camera"},
        {"takeSinglePicture",   (PyCFunction)Camera_takeSinglePicture,   METH_VARARGS, "takes one picture to file specified."},
        {"startLiveView",       (PyCFunction)Camera_startLiveView,       METH_VARARGS, "tells the camera to go into live view mode"},
        {"stopLiveView",        (PyCFunction)Camera_stopLiveView,        METH_VARARGS, "tells the camera to come out of live view mode"},
        {"liveViewImageSize",   (PyCFunction)Camera_liveViewImageSize,   METH_VARARGS, "returns (width, height) of the live view image size."
                                                                                       " only valid after live view has been on."},
        {"zoomPosition",        (PyCFunction)Camera_zoomPosition,        METH_VARARGS, "returns (x, y) of the zoom position when zoomed in in live view."},
        {"setZoomPosition",     (PyCFunction)Camera_setZoomPosition,     METH_VARARGS, "sets the zoom position when zoomed in in live view."},
        {"zoomRatio",           (PyCFunction)Camera_zoomRatio,           METH_VARARGS, "returns the zoom factor."},
        {"setZoomRatio",        (PyCFunction)Camera_setZoomRatio,        METH_VARARGS, "sets the zoom factor"},
        {"whiteBalance",        (PyCFunction)Camera_whiteBalance,        METH_VARARGS, "returns the white balance setting. this is an enum defined in edsdk."},
        {"setWhiteBalance",     (PyCFunction)Camera_setWhiteBalance,     METH_VARARGS, "sets the while balance setting"},
        {"popPictureDoneQueue", (PyCFunction)Camera_popPictureDoneQueue, METH_VARARGS, "pops the oldest picture that is completed."},
        {"pictureDoneQueueSize",(PyCFunction)Camera_pictureDoneQueueSize,METH_VARARGS, "checks how many pictures are in the completed queue."},
        {"grabLiveViewFrame",   (PyCFunction)Camera_grabLiveViewFrame,   METH_VARARGS, "refresh the frame buffer with a new frame from the camera."},

        {"autoFocus",           (PyCFunction)Camera_autoFocus,           METH_VARARGS, "performs an auto focus once right now"},
        {"pressShutterHalfway", (PyCFunction)Camera_pressShutterHalfway, METH_VARARGS, "presses the shutter button halfway once right now"},

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

    static PyObject * Camera_good(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        if (self->camera->good())
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

        self->camera->takeSinglePicture(outFile);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_startLiveView(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->startLiveView();

        Py_RETURN_NONE;
    }

    static PyObject * Camera_stopLiveView(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->stopLiveView();

        Py_RETURN_NONE;
    }

    static PyObject * Camera_liveViewImageSize(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        EdsSize size = self->camera->liveViewImageSize();

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
        
        self->camera->grabLiveViewFrame();

        Py_RETURN_NONE;
    }

    static PyObject * Camera_autoFocus(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->autoFocus();

        Py_RETURN_NONE;
    }

    static PyObject * Camera_pressShutterHalfway(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->pressShutterHalfway();

        Py_RETURN_NONE;
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
            // raise python error
            Camera_dealloc(self);
            PyErr_SetString(CameraError, "Unable to connect to camera.");
            return NULL;
        }

        return (PyObject *) self;
    }

    /* List of functions defined in the module */
    static PyMethodDef cameraMethods[] = {
        {"getFirstCamera", camera_getFirstCamera, METH_VARARGS, "return a Camera object using the first camera we can find"},

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

