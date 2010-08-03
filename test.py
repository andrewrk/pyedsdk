import edsdk
import pythoncom

import pdb; pdb.set_trace()

cam = edsdk.getFirstCamera()
cam.takeSinglePicture("C:\\testpics\\hi.jpg")

pythoncom.PumpMessages()

