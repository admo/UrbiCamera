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
    void changeNotifyImage(UVar&); // change mode function
    void changeScale(UVar&);
    void setColor(int, int, int, int, int, int); // change color
    void SetColor(int, int, int, int, int, int); // change color
    void detectFrom(UVar); // image processing function
    void SetImage(UVar&);

    // Temporary variables for image processing function
    Mat mResultImage;
    
    // Color in HSV representation
    Scalar hsv_min;
    Scalar hsv_max;
    
    int64 mLastTick;

    // Variables definig the class states
    UVar notifyImage;
    UVar mode;
    UVar visible; // if object is visible
    UVar x; // position in x of the object center
    UVar y; // position in y of the object center
    UVar scale; // image scale
    UVar width; // image width
    UVar height; // image height
    UVar fps; // fps processing
    UVar image; //image after processing
    UVar *mInputImage;
    UBinary mBinImage;
};

UColorDetector::UColorDetector(const string& s) : urbi::UObject(s) {
    UBindFunction(UColorDetector, init);
}

UColorDetector::~UColorDetector() {
    // Prevent of double free error
    if(mBinImage.image.data == mResultImage.data)
        mBinImage.image.data = 0;
    
    if(mInputImage)
        delete mInputImage;
}

int UColorDetector::init(UVar& sourceImage) {
    // Bind all urbi variables
    UBindVars(UColorDetector,
            scale,
            fps,
            width,
            height,
            visible,
            x,
            y,
            notifyImage,
            mode,
            image);

    // Bind functions
    UBindThreadedFunction(UColorDetector, detectFrom, LOCK_INSTANCE);
    UBindThreadedFunction(UColorDetector, SetImage, LOCK_INSTANCE);
    UBindFunction(UColorDetector, setColor);
    UBindFunction(UColorDetector, SetColor);
    
    mBinImage.type = BINARY_IMAGE;
    mBinImage.image.imageFormat = IMAGE_RGB;

    // Set default parameters
    x = 0;
    y = 0;
    visible = 0;
    scale = 1;
    height = -1;
    width = -1;

    hsv_min = hsv_max = Scalar(0, 0, 0, 0);

    mInputImage = new UVar(sourceImage);
    UNotifyChange(*mInputImage, &UColorDetector::detectFrom);
    UNotifyChange(notifyImage, &UColorDetector::changeNotifyImage);
    UNotifyChange(mode, &UColorDetector::changeNotifyImage);
    UNotifyChange(scale, &UColorDetector::changeScale);
    
    return 0;
}

void UColorDetector::setColor(int H_min, int H_max, int S_min, int S_max, int V_min, int V_max) {
    // Set HSV min points
    hsv_min = Scalar(H_min * 180 / 255, S_min, V_min, 0);

    // Set HSV max points
    hsv_max = Scalar(H_max * 180 / 255, S_max, V_max, 0);
}

void UColorDetector::SetColor(int H_min, int H_max, int S_min, int S_max, int V_min, int V_max) {
    setColor(H_min, H_max, S_min, S_max, V_min, V_max);
}

void UColorDetector::changeNotifyImage(UVar& var) {
    // Always unnotify
    mInputImage->unnotify();
    if (var.as<bool>())
        UNotifyChange(*mInputImage, &UColorDetector::detectFrom);
}

void UColorDetector::changeScale(UVar& newScale) {
    double tmp = newScale.as<double>();
    tmp = tmp > 1.0 ? tmp : 1.0;
    
    scale = tmp;
}

void UColorDetector::detectFrom(UVar src) {
    // Build MatImage with data from uImage
    Mat processImage(Size(src.width, src.height), CV_8UC3, src.data);

    // Resize image
    Mat resizedImage(cvRound(processImage.rows/scale.as<double>()), cvRound(processImage.cols/scale.as<double>()), CV_8UC1);
    resize(processImage, resizedImage, resizedImage.size(), 0, 0, INTER_LINEAR);
    width = resizedImage.cols;
    height = resizedImage.rows;
    
    // Copy image to mMatImage as grayscaled image
    Mat grayscaleImage;
    cvtColor(resizedImage, grayscaleImage, CV_RGB2GRAY);
    cvtColor(grayscaleImage, mResultImage, CV_GRAY2RGB);

    // Compute fps - algorithm efficency
    int64 startTick = getTickCount();
    fps = static_cast<double>(getTickFrequency()) / (startTick - mLastTick);
    mLastTick = startTick;

    // Convert From RGB to HSV color space
    Mat hsvImage;
    cvtColor(resizedImage, hsvImage, CV_RGB2HSV);

    // Find regions
    Mat thresholdImage;
    inRange(hsvImage, hsv_min, hsv_max, thresholdImage);

    // Filter
    medianBlur(thresholdImage, thresholdImage, 13);

    // Add detected region to gray scale image
    add(mResultImage, resizedImage, mResultImage, thresholdImage);

    // Compute center of the position 
    cv::Moments computedMoments(moments(thresholdImage));
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
    
    // Copy mResult image to UImage
    mBinImage.image.width = mResultImage.cols;
    mBinImage.image.height = mResultImage.rows;
    mBinImage.image.size = mResultImage.cols * mResultImage.rows * 3;
    mBinImage.image.data = mResultImage.data;
    image = mBinImage;
}

void UColorDetector::SetImage(UVar& var) {
    detectFrom(var);
}

UStart(UColorDetector);
