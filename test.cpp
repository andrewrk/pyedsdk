#include "edsdk/Camera.h"

#include <iostream>
using namespace std;

int main() {
    cout << "Grabbing the first camera" << endl;
    Camera * cam = Camera::getFirstCamera();

    if (cam) {
        cout << "Taking a picture to c:\\hi.jpg" << endl;
        cam->takeSinglePicture("C:\\hi.jpg");
    }

    return 0;
}

