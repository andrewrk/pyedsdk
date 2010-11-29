from . import Camera as CppCamera
import threading, time

__version__ = '0.2'

class Camera:
    def _checkPictureQueue(self):
        while self._running:
            while self._camera.pictureDoneQueueSize() > 0:
                pic = self._camera.popPictureDoneQueue()
                if self._pictureCompleteCallback:
                    self._pictureCompleteCallback(pic)
            time.sleep(0.10)

    def _grabLiveViewFrame(self):
        while self._running:
            if self._liveViewOn:
                # get frame
                pass
            time.sleep(0.30)

    def __init__(self, cpp_camera):
        self._camera = cpp_camera
        self._pictureCompleteCallback = None
        self._liveViewCallback = None
        self._liveViewOn = False

        # threads
        self._running = True

        # thread to watch for completed pictures
        self._pictureThread = threading.Thread(target=Camera._checkPictureQueue, args=(self,))
        self._pictureThread.start()

        # thread to grab live view frames
        self._liveViewThread = threading.Thread(target=Camera._grabLiveViewFrame, args=(self,))
        self._liveViewThread.start()

    def __del__(self):
        self._running = False
        self._pictureThread.join()
        self._liveViewThread.join()

    def good(self):
        return self._camera.good()

    def name(self):
        return self._camera.name()

    def takePicture(self, filename):
        return self._camera.takeSinglePicture(filename)

    def setPictureCompleteCallback(self, callback):
        """
        callback(filename) will be called after every picture is successfully downloaded to disk.
        """
        self._pictureCompleteCallback = callback

    def setLiveViewCallback(self, callback):
        """
        callback(bytes) will be called for each live view frame coming from the camera.
        """
        self._liveViewCallback = callback

    def startLiveView(self):
        self._camera.startLiveView()
        self._liveViewOn = True

    def stopLiveView(self):
        self._camera.stopLiveView()
        self._liveViewOn = False

    def grabLiveViewFrame(self):
        """
        tell the camera to refresh its frame buffer with a new live view
        frame from the camera.
        """
        if not self._liveViewOn:
            self.startLiveView()
        self._camera.grabLiveViewFrame()

    def liveViewMemoryView(self):
        """
        use this method to get a memoryview object which you can use to
        directly access frame image data.
        """
        return memoryview(self._camera)

    def liveViewImageSize(self):
        return self._camera.liveViewImageSize()

    def zoomPosition(self):
        return self._camera.zoomPosition()

    def setZoomPosition(self, x, y):
        return self._camera.setZoomPosition(x, y)

    def zoomRatio(self):
        return self._camera.zoomRatio()

    def setZoomRatio(self, factor):
        return self._camera.setZoomRatio(factor)

    def whiteBalance(self):
        return self._camera.whiteBalance()

    def setWhiteBalance(self, white_balance):
        return self._camera.setWhiteBalance(white_balance)

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

