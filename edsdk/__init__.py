from . import Camera as CppCamera
import threading, time

__version__ = '0.4'

class Camera:
    def _checkPictureQueue(self):
        while self._running:
            while self._camera.pictureDoneQueueSize() > 0:
                pic = self._camera.popPictureDoneQueue()
                self.flushErrors()
                if self._pictureCompleteCallback:
                    self._pictureCompleteCallback(pic)
            time.sleep(0.10)

    def __init__(self, cpp_camera):
        self._camera = cpp_camera
        self._pictureCompleteCallback = None
        self._errorMessageCallback = None
        self._liveViewOn = False

        # threads
        self._running = True

        # thread to watch for completed pictures
        self._pictureThread = threading.Thread(target=Camera._checkPictureQueue, args=(self,))
        self._pictureThread.start()

    def __del__(self):
        self._running = False
        self._pictureThread.join()

    def name(self):
        value = self._camera.name()
        self.flushErrors()
        return value

    def takePicture(self, filename):
        value = self._camera.takeSinglePicture(filename)
        self.flushErrors()
        return value

    def setPictureCompleteCallback(self, callback):
        """
        callback(filename) will be called after every picture is successfully downloaded to disk.
        """
        self._pictureCompleteCallback = callback

    def setErrorMessageCallback(self, callback):
        """
        callback(level, message) will be called when there is an error message
        """
        self._errorMessageCallback = callback

    def startLiveView(self):
        self._liveViewOn = self._camera.startLiveView()
        self.flushErrors()
        return self._liveViewOn

    def stopLiveView(self):
        success = self._camera.stopLiveView()
        self.flushErrors()
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
        self.flushErrors()

    def liveViewMemoryView(self):
        """
        use this method to get a memoryview object which you can use to
        directly access frame image data.
        """
        return memoryview(self._camera)

    def liveViewImageSize(self):
        value = self._camera.liveViewImageSize()
        self.flushErrors()
        return value

    def zoomPosition(self):
        value = self._camera.zoomPosition()
        self.flushErrors()
        return value

    def setZoomPosition(self, x, y):
        value = self._camera.setZoomPosition(x, y)
        self.flushErrors()
        return value

    def zoomRatio(self):
        value = self._camera.zoomRatio()
        self.flushErrors()
        return value

    def setZoomRatio(self, factor):
        value = self._camera.setZoomRatio(factor)
        self.flushErrors()
        return value

    def whiteBalance(self):
        value = self._camera.whiteBalance()
        self.flushErrors()
        return value

    def setWhiteBalance(self, white_balance):
        value = self._camera.setWhiteBalance(white_balance)
        self.flushErrors()
        return value

    def autoFocus(self):
        value = self._camera.autoFocus()
        self.flushErrors()
        return value

    def flushErrors(self):
        while self._camera.errMsgQueueSize() > 0:
            level, msg = self._camera.popErrMsg()
            if self._errorMessageCallback:
                self._errorMessageCallback(level, msg)

class ErrorLevel:
    Debug = 0
    Warn = 1
    Error = 2
    NoMessages = 3

def getFirstCamera():
    return Camera(CppCamera.getFirstCamera())

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
