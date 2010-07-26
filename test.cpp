#include "edsdk/Camera.h"

#include <iostream>
using namespace std;

#include <windows.h>

bool quit = true;

void pictureDone(string filename)
{
    cout << "picture which was saved to " << filename << " is done.";
    quit = true;
}

int main() {
    CoInitializeEx(0, 2);

    cout << "Grabbing the first camera" << endl;
    Camera * cam = Camera::getFirstCamera();

    if (cam) {
        cam->setPictureCompleteCallback(&pictureDone);

        cout << "Taking a picture to c:\\testpics\\hi.jpg" << endl;
        cam->takeSinglePicture("C:\\testpics\\hi.jpg");

        quit = false;
        while(! quit){
            Sleep(1);
        }
    }

    return 0;
}

