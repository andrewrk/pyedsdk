from . import Camera as CppCamera
import threading, time

__version__ = '0.4'

_errorMessageCallback = None

class Camera:
    def _checkPictureQueue(self):
        while self._running:
            while self._camera.pictureDoneQueueSize() > 0:
                pic = self._camera.popPictureDoneQueue()
                _flushErrors()
                if self._pictureCompleteCallback:
                    self._pictureCompleteCallback(pic)
            time.sleep(0.10)

    def __init__(self, cpp_camera):
        self._camera = cpp_camera
        self._pictureCompleteCallback = None
        self._liveViewOn = False

        # threads
        self._running = True

        # thread to watch for completed pictures
        self._pictureThread = threading.Thread(target=Camera._checkPictureQueue, args=(self,))
        self._pictureThread.start()

    def __del__(self):
        self._running = False
        self._pictureThread.join()

    def connect(self):
        value = self._camera.connect();
        _flushErrors()
        return value

    def disconnect(self):
        value = self._camera.disconnect();
        _flushErrors()
        return value

    def name(self):
        value = self._camera.name()
        _flushErrors()
        return value

    def takePicture(self, filename):
        value = self._camera.takeSinglePicture(filename)
        _flushErrors()
        return value

    def setPictureCompleteCallback(self, callback):
        """
        callback(filename) will be called after every picture is successfully downloaded to disk.
        """
        self._pictureCompleteCallback = callback

    def startLiveView(self):
        self._liveViewOn = self._camera.startLiveView()
        _flushErrors()
        return self._liveViewOn

    def stopLiveView(self):
        success = self._camera.stopLiveView()
        _flushErrors()
        if success:
            self._liveViewOn = False
        return not self._liveViewOn

    def grabLiveViewFrame(self):
        """
        tell the camera to refresh its frame buffer with a new live view
        frame from the camera.
        """
        if not self._liveViewOn:
            self.startLiveView()
        self._camera.grabLiveViewFrame()
        _flushErrors()

    def liveViewMemoryView(self):
        """
        use this method to get a memoryview object which you can use to
        directly access frame image data.
        """
        return memoryview(self._camera)

    def liveViewImageSize(self):
        value = self._camera.liveViewImageSize()
        _flushErrors()
        return value

    def zoomPosition(self):
        value = self._camera.zoomPosition()
        _flushErrors()
        return value

    def setZoomPosition(self, x, y):
        value = self._camera.setZoomPosition(x, y)
        _flushErrors()
        return value

    def zoomRatio(self):
        value = self._camera.zoomRatio()
        _flushErrors()
        return value

    def setZoomRatio(self, factor):
        value = self._camera.setZoomRatio(factor)
        _flushErrors()
        return value

    def whiteBalance(self):
        value = self._camera.whiteBalance()
        _flushErrors()
        return value

    def setWhiteBalance(self, white_balance):
        value = self._camera.setWhiteBalance(white_balance)
        _flushErrors()
        return value

    def autoFocus(self):
        value = self._camera.autoFocus()
        _flushErrors()
        return value

class ErrorLevel:
    Debug = 0
    Warn = 1
    Error = 2
    NoMessages = 3

def setErrorMessageCallback(callback):
    """
    callback(level, message) will be called when there is an error message
    """
    global _errorMessageCallback
    _errorMessageCallback = callback

def setErrorLevel(level):
    CppCamera.setErrorLevel(level)

def _flushErrors():
    while CppCamera.errMsgQueueSize() > 0:
        level, msg = CppCamera.popErrMsg()
        if _errorMessageCallback is not None:
            _errorMessageCallback(level, msg)

def getFirstCamera():
    cam = Camera(CppCamera.getFirstCamera())
    if cam is not None and cam.connect():
        return cam
    return None

def getFakeCamera(placeHolderImagePath):
    class FakeCamera:
        def __init__(self):
            self._pictureCompleteCallback = None
            handle = open(placeHolderImagePath, "rb")
            try:
                self._liveViewImageBytes = handle.read()
            finally:
                handle.close()
        def setPictureCompleteCallback(self, callback):
            self._pictureCompleteCallback = callback
        def startLiveView(self):
            pass
        def liveViewMemoryView(self):
            return memoryview(self._liveViewImageBytes)
        def grabLiveViewFrame(self):
            pass
        def takePicture(self, filepath):
            handle = open(filepath, "wb")
            try:
                handle.write(self._liveViewImageBytes)
            finally:
                handle.close()
            self._pictureCompleteCallback(filepath)
    return FakeCamera()

def terminate():
    CppCamera.terminate()
