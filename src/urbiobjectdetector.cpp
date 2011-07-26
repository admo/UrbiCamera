/*******************************************
*
*	UObjectDetection v2.3
*   Simple module to detect object
*	Compiled with OpenCV 2.2
*   Author: Jan Kedzierski
*   date: 06.04.2011
*
********************************************/

#include <urbi/uobject.hh>

#include <cv.h>
#include <highgui.h>

#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using namespace urbi;
using namespace std;

class UObjectDetector : public UObject {
public:
    UObjectDetector(const string&);
    ~UObjectDetector();
    
    int init(UVar& sourceImage, const string& cascadeFiles);
    
private:
    cv::CascadeClassifier mCVCascade;
    Mat mResultImage;
    int64 mLastTick;
    
    // Urbi functions
    void changeNotifyImage(); // change mode function
    void changeHaarCascade();
    void detectFrom(UVar&); // image processing function
    
    // Urbi variables
    // Results
    UVar number; // number of visible object
    UVar visible; // if any object is visible
    UVar x; // position of the object center
    UVar y; // position of the object center
    UVar fps; // fps processing
    UVar image; //image after processing
    UVar *mInputImage;
    UBinary mBinImage;
    // Parameters
    UVar scale; // image scale
    UVar cascade; // har cascade access
    UVar width; // image width
    UVar height; // image height
    UVar notifyImage; // process new images;
};

UObjectDetector::UObjectDetector(const string& s) : UObject(s) {
    UBindFunction(UObjectDetector, init);
}

UObjectDetector::~UObjectDetector() {
    // Prevent of double free error
    if(mBinImage.image.data == mResultImage.data)
        mBinImage.image.data = 0;
    
    if(mInputImage)
        delete mInputImage;
}

int UObjectDetector::init(UVar& sourceImage, const string& cascadeFile) {
    // Bind all urbi variables
    UBindVars(UObjectDetector,
            number,
            visible,
            x,
            y,
            fps,
            image,
            scale,
            cascade,
            width,
            height,
            notifyImage,
            cascade);
    
    // Bind functions
    UBindThreadedFunction(UObjectDetector, detectFrom, LOCK_INSTANCE);
    
    mBinImage.type = BINARY_IMAGE;
    mBinImage.image.imageFormat = IMAGE_RGB;
    
    // Set default parameters
    x = 0;
    y = 0;
    visible = 0;
    scale = 1;
    height = -1;
    width = -1;
    cascade = cascadeFile;
    
    mInputImage = new UVar(sourceImage);
    UNotifyChange(*mInputImage, &UObjectDetector::detectFrom);
    UNotifyChange(notifyImage, &UObjectDetector::changeNotifyImage);
    UNotifyChange(cascade, &UObjectDetector::changeHaarCascade);
    
    changeHaarCascade();
    
    return 0;
}

void UObjectDetector::changeNotifyImage() {
    // Always unnotify
    mInputImage->unnotify();
    if (notifyImage.as<bool>())
        UNotifyChange(*mInputImage, &UObjectDetector::detectFrom);
}

void UObjectDetector::changeHaarCascade() {
    if(!mCVCascade.load(cascade))
        throw std::runtime_error("Could not load cascade classifier");
    
    cerr << "New " << cascade.as<string>() << " loaded." << endl;
}

void UObjectDetector::detectFrom(UVar& src) {
    // Copy image
    UImage uImage = src;
    
    // Build MatImage with data from uImage
    Mat processImage(Size(uImage.width, uImage.height), CV_8UC3, uImage.data);
    
    // Resize image
    resize(processImage, mResultImage, Size(), scale.as<double>(), scale.as<double>(), INTER_LINEAR);
    width = mResultImage.cols;
    height = mResultImage.rows;
    
    if(mCVCascade.empty()) {
        throw std::runtime_error("Cascade classifier not loaded");
    } else {
        // ...to measure all processing time
        int64 startTick = getTickCount();
        fps = static_cast<double>(getTickFrequency()) / (startTick - mLastTick);
        mLastTick = startTick;
        
        vector<Rect> objects;
        mCVCascade.detectMultiScale(mResultImage, objects, 1.1, 2, 0, Size(30, 30));
        
        if(!objects.empty()) {
            //TODO wykorzystać boost??
            vector<Rect>::const_iterator biggest = objects.begin();
            for(vector<Rect>::const_iterator i = objects.begin(); i < objects.end(); ++i) {
                if(i->area() > biggest->area())
                    biggest = i;
            }
            
            // Draw on a mResultImage
            Point center(biggest->x+biggest->width/2, biggest->y+biggest->height/2);
            int radius = (biggest->height + biggest->height)/4;
            circle(mResultImage, center, radius, Scalar(255,0,0), 3, 8, 0);
            line(mResultImage, center, Point(mResultImage.cols/2, mResultImage.rows/2), Scalar(255, 0, 0), 2);
            
            // Set position of the object
            x = biggest->x-mResultImage.cols/2;
            y = -biggest->y-mResultImage.rows/2;
            
            visible = 1;
        } else {
            visible = 0;
            x = 0;
            y = 0;
        }
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

UStart(UObjectDetector);