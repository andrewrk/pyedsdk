#include <Python.h>

#include "Camera.h"

extern "C" {
    static PyObject * ErrorObject;

    typedef struct {
        PyObject_HEAD
        PyObject * x_attr; // attributes dictionary
        Camera * camera; // C++ object
    } CameraObject;

    static void Camera_dealloc(CameraObject * self);
    static PyObject * Camera_getattr(CameraObject * self, char * name);
    static int Camera_setattr(CameraObject * self, char * name, PyObject * v);
    DL_EXPORT(void) initCamera();

    static PyTypeObject Camera_Type = {
        /* The ob_type field must be initialized in the module init function
         * to be portable to Windows without using C++. */
        PyObject_HEAD_INIT(NULL)
        0,          /*ob_size*/
        "Camera",          /*tp_name*/
        sizeof(CameraObject),  /*tp_basicsize*/
        0,          /*tp_itemsize*/
        /* methods */
        (destructor)Camera_dealloc, /*tp_dealloc*/
        0,          /*tp_print*/
        (getattrfunc)Camera_getattr, /*tp_getattr*/
        (setattrfunc)Camera_setattr, /*tp_setattr*/
        0,          /*tp_compare*/
        0,          /*tp_repr*/
        0,          /*tp_as_number*/
        0,          /*tp_as_sequence*/
        0,          /*tp_as_mapping*/
        0,          /*tp_hash*/
    };

#define CameraObject_Check(v) ((v)->ob_type == &Camera_Type)

    // Camera methods

    static void Camera_dealloc(CameraObject * self)
    {
        delete self->camera;
        Py_XDECREF(self->x_attr);
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

    static PyObject * Camera_beginFastPictures(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->beginFastPictures();

        Py_RETURN_NONE;
    }

    static PyObject * Camera_takeFastPicture(CameraObject * self, PyObject * args)
    {
        char * outFile;
        if (! PyArg_ParseTuple(args, "s", &outFile))
            return NULL;

        self->camera->takeFastPicture(outFile);

        Py_RETURN_NONE;
    }

    static PyObject * Camera_endFastPictures(CameraObject * self, PyObject * args)
    {
        if (! PyArg_ParseTuple(args, ""))
            return NULL;

        self->camera->endFastPictures();

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

    static PyMethodDef Camera_methods[] = {
        {"good",                (PyCFunction)Camera_good,                METH_VARARGS},
        {"name",                (PyCFunction)Camera_name,                METH_VARARGS},
        {"takeSinglePicture",   (PyCFunction)Camera_takeSinglePicture,   METH_VARARGS},
        {"beginFastPictures",   (PyCFunction)Camera_beginFastPictures,   METH_VARARGS},
        {"takeFastPicture",     (PyCFunction)Camera_takeFastPicture,     METH_VARARGS},
        {"endFastPictures",     (PyCFunction)Camera_endFastPictures,     METH_VARARGS},
        //{"setPictureCompleteCallback", (PyCFunction)Camera_setPictureCompleteCallback, METH_VARARGS},
        //{"setLiveViewCallback", (PyCFunction)Camera_setLiveViewCallback, METH_VARARGS},
        {"startLiveView",       (PyCFunction)Camera_startLiveView,       METH_VARARGS},
        {"stopLiveView",        (PyCFunction)Camera_stopLiveView,        METH_VARARGS},
        {"liveViewImageSize",   (PyCFunction)Camera_liveViewImageSize,   METH_VARARGS},
        {"zoomPosition",        (PyCFunction)Camera_zoomPosition,        METH_VARARGS},
        {"setZoomPosition",     (PyCFunction)Camera_setZoomPosition,     METH_VARARGS},
        {"zoomRatio",           (PyCFunction)Camera_zoomRatio,           METH_VARARGS},
        {"setZoomRatio",        (PyCFunction)Camera_setZoomRatio,        METH_VARARGS},
        {"whiteBalance",        (PyCFunction)Camera_whiteBalance,        METH_VARARGS},
        {"setWhiteBalance",     (PyCFunction)Camera_setWhiteBalance,     METH_VARARGS},

        {NULL,      NULL} // sentinel
    };

    static PyObject * Camera_getattr(CameraObject * self, char * name)
    {
        if (self->x_attr != NULL) {
            PyObject * v = PyDict_GetItemString(self->x_attr, name);
            if (v != NULL) {
                Py_INCREF(v);
                return v;
            }
        }
        return Py_FindMethod(Camera_methods, (PyObject *)self, name);
    }

    static int Camera_setattr(CameraObject * self, char * name, PyObject * v)
    {
        if (self->x_attr == NULL) {
            self->x_attr = PyDict_New();
            if (self->x_attr == NULL)
                return -1;
        }
        if (v == NULL) {
            int rv = PyDict_DelItemString(self->x_attr, name);
            if (rv < 0)
                PyErr_SetString(PyExc_AttributeError,
                        "delete non-existing Camera attribute");
            return rv;
        }
        else
            return PyDict_SetItemString(self->x_attr, name, v);
    }
    /* --------------------------------------------------------------------- */

    /* Function of no arguments returning new Camera object */
    static PyObject * camera_getFirstCamera(PyObject * , PyObject * )
    {
        CameraObject * self;
        self = PyObject_NEW(CameraObject, &Camera_Type);
        if ( self == NULL )
            return NULL;

        self->x_attr = NULL;
        self->camera = Camera::getFirstCamera();

        return (PyObject *) self;
    }

    /* List of functions defined in the module */
    static PyMethodDef camera_methods[] = {
        {"getFirstCamera", camera_getFirstCamera, METH_VARARGS},
        {NULL, NULL}       /* sentinel */
    };


    /* Initialization function for the module (*must* be called initcamera) */
    DL_EXPORT(void) initCamera()
    {
        PyObject *m, *d;

        /* Initialize the type of the new type object here; doing it here
         * is required for portability to Windows without requiring C++. */
        Camera_Type.ob_type = &PyType_Type;

        /* Create the module and add the functions */
        m = Py_InitModule("Camera", camera_methods);

        /* Add some symbolic constants to the module */
        d = PyModule_GetDict(m);
        ErrorObject = PyErr_NewException("camera.error", NULL, NULL);
        PyDict_SetItemString(d, "error", ErrorObject);
    }
}

