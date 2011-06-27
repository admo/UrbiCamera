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

    int init(UVar& sourceImage); // init object
    
private:
    void changeScale(UVar&); // change scale function
    void changeMode(UVar&); // change mode function
    void setColor(int, int, int, int, int, int); // change mode function
    void newImage(UVar&); // image processing function

    // Temporary variables for image processing function
    Mat mResultImage;
    Mat mGrayscaleImage;
    Mat mHSVImage;
    Mat mThresholdImage;
    
    Scalar hsv_min;
    Scalar hsv_max;
    
    int64 mLastTick;

    // Variables definig the class states
    UVar visible; // if object is visible
    UVar x; // position in x of the object center
    UVar y; // position in y of the object center
    UVar scale; // image scale
    UVar width; // image width
    UVar height; // image height
    UVar fps; // fps processing
    UVar mode; // mode
    UVar image; //image after processing
    UVar *mInputImage;
    UBinary mBinImage;
    string mUVarInputImageName;

    InputPort input; // Input port for mode 2
};

UColorDetector::UColorDetector(const string& s) : urbi::UObject(s) {
    UBindFunction(UColorDetector, init);
}

UColorDetector::~UColorDetector() {}

int UColorDetector::init(UVar& sourceImage) {
    // Bind all urbi variables
    UBindVars(UColorDetector,
            mode,
            input,
            scale,
            fps,
            width,
            height,
            visible,
            x,
            y,
            image);

    // Bind functions
    UBindThreadedFunction(UColorDetector, newImage, LOCK_INSTANCE);
    UBindFunction(UColorDetector, setColor);
    
    mBinImage.type = BINARY_IMAGE;
    mBinImage.image.imageFormat = IMAGE_RGB;

    // Set default parameters
    x = 0;
    y = 0;
    visible = 0;
    scale = 1;
    height = -1;
    width = -1;
    mode = 0;

    hsv_min = hsv_max = Scalar(0, 0, 0, 0);

    mInputImage = new UVar(sourceImage);
    UNotifyChange(mode, &UColorDetector::changeMode);
    UNotifyChange(*mInputImage, &UColorDetector::newImage);
    
    return 0;
}

void UColorDetector::setColor(int H_min, int H_max, int S_min, int S_max, int V_min, int V_max) {
    // Set HSV min points
    hsv_min = Scalar(H_min * 180 / 255, S_min, V_min, 0);

    // Set HSV max points
    hsv_max = Scalar(H_max * 180 / 255, S_max, V_max, 0);
}

void UColorDetector::changeMode(UVar&) {
    switch (mode.as<int>()) {
        case 0:
            mInputImage->unnotify();
            break;
        case 1:
            // Notiffy source
            UNotifyChange(*mInputImage, &UColorDetector::newImage);
            break;
        case 2:
            // Notiffy input port
            mInputImage->unnotify();
            break;
    }
}

void UColorDetector::newImage(UVar& src) {
    cerr << "UColorDetector::newImage" << endl;
    // Copy image
    UImage uImage = src;
    
    // Build MatImage with data from uImage
    Mat processImage(Size(uImage.width, uImage.height), CV_8UC3, uImage.data);

    // Resize image
    resize(processImage, mResultImage, Size(), scale.as<double>(), scale.as<double>(), INTER_LINEAR);

    // ...to measure all processing time
    int64 t = getTickCount();
    
    // Copy image to mMatImage as grayscaled image
    cvtColor(mResultImage, mGrayscaleImage, CV_RGB2GRAY);
    cvtColor(mGrayscaleImage, mResultImage, CV_GRAY2RGB);

    // Compute fps - algorithm efficency
    fps = static_cast<double>(getTickFrequency()) / (t - mLastTick);
    mLastTick = t;

    // Convert From RGB to HSV color space
    cvtColor(processImage, mHSVImage, CV_RGB2HSV);

    // Find regions
    inRange(mHSVImage, hsv_min, hsv_max, mThresholdImage);

    // Filter
    medianBlur(mThresholdImage, mThresholdImage, 13);

    // Add detected region to gray scale image
    add(mResultImage, processImage, mResultImage, mThresholdImage);

    // Compute center of the position 
    cv::Moments computedMoments(moments(mThresholdImage));
    int xx = static_cast<int>(computedMoments.m10 / computedMoments.m00);
    int yy = static_cast<int>(computedMoments.m01 / computedMoments.m00);
    
    // Finally set visible
    if ((xx > 0) && (yy > 0)) {
        // Set point and visible
        x = xx - processImage.cols / 2;
        y = -yy + processImage.rows / 2;
        visible = 1;

        // Draw line from image center to object center
        line(mResultImage, Point(xx, yy), Point(mResultImage.cols/2, mResultImage.rows/2), Scalar(255, 0, 0), 2);
    } else {
        x = 0;
        y = 0;
        visible = 0;
    }

    // Draw horizontal and vertical line in the middle of the image
    line(mResultImage, Point(0, mResultImage.rows/2), Point(mResultImage.cols, mResultImage.rows/2), Scalar(100, 100, 100), 1);
    line(mResultImage, Point(mResultImage.cols/2, 0), Point(mResultImage.cols/2, mResultImage.rows), Scalar(100, 100, 100), 1);
    
    mBinImage.image.width = mResultImage.cols;
    mBinImage.image.height = mResultImage.rows;
    mBinImage.image.size = mResultImage.cols * mResultImage.rows * 3;
    mBinImage.image.data = mResultImage.data;
    image = mBinImage;
    cerr << "UColorDetector::newImage end" << endl;
}

UStart(UColorDetector);