#include "edsdk/Camera.h"

#include <iostream>
using namespace std;

#include <windows.h>

void pictureDone(string filename)
{
    cout << "picture which was saved to " << filename << " is done.";
}

int main() {
    cout << "Grabbing the first camera" << endl;
    Camera * cam = Camera::getFirstCamera();

    if (cam) {
        cam->setPictureCompleteCallback(&pictureDone);

        cout << "Taking a picture to c:\\testpics\\hi.jpg" << endl;
        cam->takeSinglePicture("C:\\testpics\\hi.jpg");

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

