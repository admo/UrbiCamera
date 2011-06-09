/*******************************************
 *
 *	UColorDetector v1.0
 *   Simple module to acces web camera.
 *	Compiled with OpenCV 2.2
 *   Author: Jan Kedzierski
 *   date: 11.03.2011
 *
 ********************************************/

#include <urbi/uobject.hh>

#include <cv.h>
#include <highgui.h>

#include <iostream>
#include <string>

using namespace cv;
using namespace urbi;
using namespace std;

class UColorDetector : public UObject {
public:
    UColorDetector(const string&);
    ~UColorDetector();

public:
    int init(const std::string& sourceImage); // init object
    void changeScale(UVar&); // change scale function
    void changeMode(UVar&); // change mode function
    void SetColor(int, int, int, int, int, int); // change mode function
    void SetImage(UVar&); // image processing function

private:
    IplImage* smallimg; // image for processing
    IplImage* hsv_image;
    IplImage* thresholded;
    CvScalar hsv_min;
    CvScalar hsv_max;
    bool need_update; // update image settings flag
    double t; // temporary time variable
    double tick; // -||-

    UVar visible; // if object is visible
    UVar x; // position in x of the object center
    UVar y; // position in y of the object center
    UVar scale; // image scale
    UVar time; // processing time
    UVar width; // image width
    UVar height; // image height
    UVar fps; // fps processing
    UVar mode; // mode


    InputPort input; // Input port for mode 2

    UVar cvimg; //IplImage  variable
    UVar uimg; //UImage  variable

    UBinary ubin; // Storage for last captured image.
    UImage uimage; // Last captured image in UImage format.
};

UColorDetector::UColorDetector(const string& s) : urbi::UObject(s) {
    UBindFunction(UColorDetector, init);
}

int UColorDetector::init(const std::string& sourceImage) {
    // Bind all urbi variables
    UBindVars(UColorDetector,
            mode,
            input,
            scale,
            time,
            fps,
            width,
            height,
            visible,
            x,
            y,
            uimg);

    // Bind functions
    UBindThreadedFunction(UColorDetector, SetImage, LOCK_INSTANCE);
    UBindFunction(UColorDetector, SetColor);

    // Set default parameters
    x = 0;
    y = 0;
    visible = 0;
    scale = 1;
    height = -1;
    width = -1;
    mode = 0;

    hsv_min = hsv_max = Scalar(0, 0, 0, 0);

    smallimg = 0;
    hsv_image = 0;
    thresholded = 0;

    UNotifyChange(scale, &UColorDetector::changeScale);
    UNotifyChange(mode, &UColorDetector::changeMode);
    UNotifyChange(sourceImage, &UColorDetector::SetImage);

    return 0;
}

void UColorDetector::SetColor(int H_min, int H_max, int S_min, int S_max, int V_min, int V_max) {
    // Set HSV min points
    hsv_min = Scalar(H_min * 180 / 255, S_min, V_min, 0);

    // Set HSV max points
    hsv_max = Scalar(H_max * 180 / 255, S_max, V_max, 0);
}

void UColorDetector::changeMode(UVar&) {
    int tmp = mode;
//    switch (tmp) {
//        case 0:
//            source->unnotify();
//            input.unnotify();
//            break;
//        case 1:
//            // Notiffy source
//            input.unnotify();
//            UNotifyChange(*source, &UColorDetector::SetImage);
//            break;
//        case 2:
//            // Notiffy input port
//            source->unnotify();
//            UNotifyChange(input, &UColorDetector::SetImage);
//            break;
//    }
}

void UColorDetector::changeScale(UVar&) {
    need_update = true;
    return;
}

void UColorDetector::SetImage(UVar& src) {

    // Set UBinary
    UBinary image = src;
    Mat matImage(Size(640, 480), CV_8UC3, image.image.data);

    cerr<<"Mat sizes " << matImage.cols << " " << matImage.rows << endl
            << matImage.data << " ? " << image.image.data << endl;
    

//    if (need_update) {
//        need_update = false;
//
//        // Release previous created image
//        cvReleaseImage(&smallimg);
//        cvReleaseImage(&hsv_image);
//        cvReleaseImage(&thresholded);
//
//        // Create a new image based on the input image and new parameters
//        smallimg = cvCreateImage(cvSize((img->width / (double) scale), (img->height / (double) scale)), 8, 3);
//        hsv_image = cvCreateImage(cvSize((img->width / (double) scale), (img->height / (double) scale)), 8, 3);
//        thresholded = cvCreateImage(cvSize((img->width / (double) scale), (img->height / (double) scale)), 8, 1);
//
//        // Create new Uimage parameters
//        uimage.width = smallimg->width;
//        uimage.height = smallimg->height;
//        uimage.imageFormat = IMAGE_RGB;
//        uimage.data = 0;
//        uimage.size = smallimg->width * smallimg->height * 3;
//        width = uimage.width;
//        height = uimage.height;
//        x.rangemax = smallimg->width / 2;
//        x.rangemin = -smallimg->width / 2;
//        y.rangemax = smallimg->height / 2;
//        y.rangemin = -smallimg->height / 2;
//
//    }

    // Resize or copy new image to smallimg variable
//    if (((double) scale) != 1) cvResize(img, smallimg, CV_INTER_LINEAR);
//    else cvCopy(img, smallimg, 0);
//
//    // ...to measure all processing time
//    double t = (double) cvGetTickCount();
//
//    // Compute fps - algorithm efficency
//    fps = ((double) cvGetTickFrequency()*1000000) / (t - tick);
//    tick = t;
//
//    // Convert From RGB to HSV color space
//    cvCvtColor(smallimg, hsv_image, CV_RGB2HSV);
//
//    // Fint regions
//    cvInRangeS(hsv_image, hsv_min, hsv_max, thresholded);
//
//    // Filter
//    cvSmooth(thresholded, thresholded, CV_MEDIAN, 13, 0, 0, 0);
//
//    // Moments
//    CvMoments moments;
//
//    // Compute center of the position 
//    cvMoments(thresholded, &moments, 0);
//    int xx = (int) (moments.m10 / moments.m00);
//    int yy = (int) (moments.m01 / moments.m00);
//
//    t = (double) cvGetTickCount() - t;
//    time = t / ((double) cvGetTickFrequency()*1000.);
//
//    // Add detected region to gray scale image
//    IplImage* tmp = cvCreateImage(cvSize((img->width / (double) scale), (img->height / (double) scale)), 8, 1);
//    cvCvtColor(smallimg, tmp, CV_BGR2GRAY);
//    IplImage* gray = cvCreateImage(cvSize((img->width / (double) scale), (img->height / (double) scale)), 8, 3);
//    cvCvtColor(tmp, gray, CV_GRAY2BGRA);
//    cvAddS(smallimg, CvScalar(), gray, thresholded);
//    cvCopy(gray, smallimg);
//    cvReleaseImage(&gray);
//    cvReleaseImage(&tmp);
//
//
//
//    // Finally set visible
//    if ((xx > 0) && (yy > 0)) {
//        // Set point and visible
//        x = xx - smallimg->width / 2;
//        y = -yy + smallimg->height / 2;
//        visible = 1;
//
//        // Define position
//        CvPoint pos = cvPoint(xx, yy);
//
//        // Draw line from image center to object center
//        cvLine(smallimg, pos, cvPoint(smallimg->width / 2, smallimg->height / 2), cvScalar(255, 0, 0), 2);
//    } else {
//        x = 0;
//        y = 0;
//        visible = 0;
//    }
//
//    // Draw horizontal and vertical line in the middle of the image
//    cvLine(smallimg, cvPoint(0, smallimg->height / 2), cvPoint(smallimg->width, smallimg->height / 2), cvScalar(100, 100, 100), 1);
//    cvLine(smallimg, cvPoint(smallimg->width / 2, 0), cvPoint(smallimg->width / 2, smallimg->height), cvScalar(100, 100, 100), 1);
//
//    // Assign image to UBinary variable.
//    ubin.common.data = smallimg;
//    cvimg = ubin;
//
//    // Save to UImage also
//    uimage.data = (unsigned char*) smallimg->imageData;
//    uimg = uimage;

}

UStart(UColorDetector);
