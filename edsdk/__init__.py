from . import Camera as CppCamera
import threading, time
import pythoncom
import queue

__version__ = '0.6'

_errorMessageCallback = None
_methodQueue = queue.Queue()
_running = False
_comThread = None
_callbacksThread = None
_callbackQueue = queue.Queue()

def _make_thread(target, name, args=[]):
    return threading.Thread(target=target, name=name, args=args)

def _run_com_thread():
    pythoncom.CoInitializeEx(2)

    while _running:
        try:
            while True:
                func, args, callback = _methodQueue.get(block=False)
                result = func(*args)
                _flushErrors()
                if callback is not None:
                    _callbackQueue.put((callback, result))
        except queue.Empty:
            pass
        pythoncom.PumpWaitingMessages()
        time.sleep(0.05)

def _run_callbacks_thread():
    while _running:
        func, args = _callbackQueue.get(block=True)
        func(args)

def _runInComThread(func, args=None, callback=None):
    if args is None:
        args = []
    _methodQueue.put((func, args, callback))

def getFirstCamera(callback):
    """
    calls callback(camera) with the first connected camera
    """
    def f():
        cpp_cam = CppCamera.getFirstCamera()
        if cpp_cam is None:
            return None
        else:
            cam = Camera(cpp_cam)
            cam.connect()
            return cam
    _runInComThread(f, callback=callback)

class ErrorLevel:
    Debug = 0
    Warn = 1
    Error = 2
    NoMessages = 3

class MeteringMode:
    SpotMetering = 1
    EvaluativeMetering = 3
    PartialMetering = 4
    CenterWeightedMetering = 5

class DriveMode:
    SingleFrameShooting = 0x00000000
    ContinuousShooting = 0x00000001
    Video = 0x00000002
    HighSpeedContinuousShooting = 0x00000004
    LowSpeedContinuousShooting = 0x00000005
    SilentSingleShooting = 0x00000006
    TenSecSelfTimerPlusShots = 0x00000007
    TenSecSelfTimer = 0x00000010
    TwoSecSelfTimer = 0x00000011

class AFMode:
    OneShotAF = 0
    AIServoAF = 1
    AIFocusAF = 2
    ManualFocus = 3


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

def terminate():
    global _running
    if not _running:
        # already terminated
        return

    def f():
        _callbacksThread.join()
        CppCamera.terminate()
        global _running
        _running = False
    _callbackQueue.put((lambda x: x, None))
    _runInComThread(f)

def initialize():
    global _running
    if _running:
        # already initialized
        return

    _running = True

    global _comThread
    _comThread = _make_thread(_run_com_thread, "edsdk._run_com_thread")
    _comThread.start()

    global _callbacksThread
    _callbacksThread = _make_thread(_run_callbacks_thread, "edsdk._run_callbacks_thread")
    _callbacksThread.start()

class Camera:
    """
    class which represents a Canon camera. don't forget to call disconnect
    when you're done or a thread will keep runnning.
    """
    def _checkPictureQueue(self):
        while self._running:
            while self._camera.pictureDoneQueueSize() > 0:
                pic = self._camera.popPictureDoneQueue()
                _flushErrors()
                if self._pictureCompleteCallback:
                    _callbackQueue.put((self._pictureCompleteCallback, pic))
            time.sleep(0.10)

    def __init__(self, cpp_camera):
        self._camera = cpp_camera
        self._pictureCompleteCallback = None
        self._liveViewOn = False
        self._running = False

    def connect(self):
        if self._running:
            return

        # threads
        self._running = True

        # thread to watch for completed pictures
        self._pictureThread = _make_thread(Camera._checkPictureQueue, "edsdk._checkPictureQueue", args=(self,))
        self._pictureThread.start()

        _runInComThread(self._camera.connect)

    def disconnect(self):
        if not self._running:
            return

        self._running = False

        _runInComThread(self._camera.disconnect)
        self._pictureThread.join()

    def name(self, callback):
        """
        callback(name) will be called when the camera name is ascertained.
        """
        _runInComThread(self._camera.name, callback=callback)

    def takePicture(self, filename):
        _runInComThread(self._camera.takeSinglePicture, args=[filename])

    def setPictureCompleteCallback(self, callback):
        """
        callback(filename) will be called after every picture is successfully downloaded to disk.
        """
        self._pictureCompleteCallback = callback

    def startLiveView(self):
        def cb(success):
            self._liveViewOn = success
        _runInComThread(self._camera.startLiveView, callback=cb)
        self._liveViewOn = True

    def stopLiveView(self):
        def cb(success):
            self._liveViewOn = not success
        _runInComThread(self._camera.stopLiveView, callback=cb)
        self._liveViewOn = False

    def grabLiveViewFrame(self):
        """
        tell the camera to refresh its frame buffer with a new live view
        frame from the camera.
        """
        if not self._liveViewOn:
            self.startLiveView()
        _runInComThread(self._camera.grabLiveViewFrame)

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
        self._camera.setZoomPosition(x, y)

    def zoomRatio(self):
        return self._camera.zoomRatio()

    def setZoomRatio(self, factor):
        self._camera.setZoomRatio(factor)

    def whiteBalance(self, callback):
        """
        callback(white_balance) will be called when the data is ready
        """
        _runInComThread(self._camera.whiteBalance, callback=callback)

    def setWhiteBalance(self, white_balance):
        self._camera.setWhiteBalance(white_balance)

    def meteringMode(self, callback):
        """
        callback(metering_mode) will be called when the data is ready
        """
        _runInComThread(self._camera.meteringMode, callback=callback)

    def setMeteringMode(self, mode):
        _runInComThread(self._camera.setMeteringMode, args=[mode])
    
    def driveMode(self, callback):
        """
        callback(drive_mode) will be called when the data is ready
        """
        _runInComThread(self._camera.driveMode, callback=callback)

    def setDriveMode(self, mode):
        _runInComThread(self._camera.setDriveMode, args=[mode])

    def afMode(self, callback):
        """
        callback(af_mode) will be called
        """
        _runInComThread(self._camera.afMode, callback=callback)

    def setAFMode(self, mode):
        _runInComThread(self._camera.setAFMode, args=[mode])

    def autoFocus(self):
        _runInComThread(self._camera.autoFocus)

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
        def autoFocus(self):
            pass
    return FakeCamera()

initialize()

