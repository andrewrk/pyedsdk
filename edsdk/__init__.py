import Camera as CppCamera
import threading, time

__version__ = '0.1'

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

    def takeSinglePicture(self, filename):
        return self._camera.takeSinglePicture(filename)

    def beginFastPictures(self):
        return self._camera.beginFastPictures()

    def takeFastPicture(self, filename):
        return self._camera.takeFastPicture(filename)

    def endFastPictures(self):
        return self._camera.endFastPictures()

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
