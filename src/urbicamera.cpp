/*******************************************
 *
 *	UCamera v2.1
 *   Simple module to acces web camera.
 *	Compiled with OpenCV 2.2
 *   Author: Jan Kedzierski
 *   date: 11.03.2011
 *
 ********************************************/

#include <urbi/uobject.hh>

#include <cv.h>
#include <highgui.h>

#include <boost/thread.hpp>

#include <iostream>

using namespace cv;
using namespace urbi;
using namespace std;
using namespace boost;

class UCamera : public UObject {
public:
    // The class must have a single constructor taking a string. 
    UCamera(const std::string&);
    virtual ~UCamera();

    virtual int update();

private:
    // Urbi constructor. Throw error in case of error.
    void init(int);

    // Our image variable and dimensions
    UVar image;
    UVar width;
    UVar height;
    UVar fps;
    UVar notify;
    UVar flip;
    enum {flipD0, flipD90, flipD180, flipD270} mFlipImage;

    bool mGetNewFrame; //
    unsigned int mFrame; // ID of already grabed frame
    unsigned int mAccessFrame; // ID of already retrieved frame

    // Called on access.
    void getImage();
    void GetImage();
    
    //
    void changeNotifyImage(UVar&);
    void changeFlipImage();

    // Access object to camera
    VideoCapture videoCapture;

    // Thread object
    boost::thread grabImageThread;
    // Thread function to grab image
    void grabImageThreadFunction();

    // Mutex and conditional variable to synchronize threads
    boost::mutex getValMutex;
    boost::mutex grabImageMutex;
    boost::condition_variable grabImageCond;
    boost::unique_lock<mutex> grabImageMutexLock;

    // Storage for last captured image. 
    UBinary mBinImage;
    Mat mMatImage;

    void fpsChanged();
};

UCamera::UCamera(const std::string& s) : grabImageMutexLock(grabImageMutex), urbi::UObject(s) {
    UBindFunction(UCamera, init);
}

UCamera::~UCamera() {
    grabImageThread.interrupt();
    grabImageThread.join();
    // Prevent from double free
    if (mMatImage.data == mBinImage.image.data)
        mBinImage.image.data = NULL;
}

void UCamera::init(int id) {
    cerr << "UCamera::init(" << id << ")" << endl;
    // Urbi constructor
    mGetNewFrame = true;
    mFrame = mAccessFrame = 0;
    mFlipImage = flipD0;

    if (!videoCapture.open(id))
        throw runtime_error("Failed to initialize camera");

    // Bind all variables
    UBindVar(UCamera, image);
    UBindVar(UCamera, width);
    UBindVar(UCamera, height);
    UBindVar(UCamera, fps);
    UBindVar(Ucamera, notify);
    UBindVar(UCamera, flip);
    flip = 0;
    
    // Bind all functions
    UBindThreadedFunction(UCamera, getImage, LOCK_INSTANCE);
    UBindThreadedFunction(UCamera, GetImage, LOCK_INSTANCE);

    // Notify if fps changed
    UNotifyChange(fps, &UCamera::fpsChanged);
    UNotifyChange(notify, &UCamera::changeNotifyImage);
    UNotifyChange(flip, &UCamera::changeFlipImage);

    // Get image size
    videoCapture >> mMatImage;
    width = mMatImage.cols;
    height = mMatImage.rows;

    cerr << "\tImage size: x=" << width.as<int>() << " y=" << height.as<int>() << endl;

    UNotifyAccess(image, &UCamera::getImage);

    mBinImage.type = BINARY_IMAGE;
    mBinImage.image.width = width.as<size_t > ();
    mBinImage.image.height = height.as<size_t > ();
    mBinImage.image.imageFormat = IMAGE_RGB;
    mBinImage.image.size = width.as<size_t > () * height.as<size_t > () * 3;

    // Start video grabbing thread
    grabImageThread = boost::thread(&UCamera::grabImageThreadFunction, this);

    // Set update period
    fps = 25;
}

void UCamera::grabImageThreadFunction() {
    cerr << "UCamera::grabImageThreadFunction()" << endl
            << "\tThread started" << endl;
    try {
        while (true) {
            this_thread::interruption_point();
            if (!videoCapture.grab()) {
				this_thread::sleep(posix_time::milliseconds(15));
				continue;
			}
            ++mFrame;
            // Try to populate data
            if (grabImageMutex.try_lock()) {
                videoCapture.retrieve(mMatImage);
                Mat tmp;
                switch (mFlipImage) {
                case flipD90:
                	cv::flip(mMatImage, tmp, 1);
                	transpose(tmp, mMatImage);
                	break;
                case flipD180:
                	cv::flip(mMatImage, mMatImage, -1);
                	break;
                case flipD270:
                	cv::flip(mMatImage, tmp, 0);
                	transpose(tmp, mMatImage);
                	break;
                }
                cvtColor(mMatImage, mMatImage, CV_BGR2RGB);
                grabImageCond.notify_one();
                grabImageMutex.unlock();
            }
        }
    } catch (boost::thread_interrupted&) {
        cerr << "UCamera::grabImageThreadFunction()" << endl
                << "\tThread stopped" << endl;
        videoCapture.release();
        return;
    }
}

void UCamera::getImage() {
    // Lock access to this method from urbi
    lock_guard<mutex> lock(getValMutex);
    
    // If there is new frame
    if(mGetNewFrame) {
        mGetNewFrame = false;

        grabImageCond.wait(grabImageMutexLock);
        mBinImage.image.data = mMatImage.data;
        // Copy frame to an external variable
        image = mBinImage;
    }
}

void UCamera::GetImage() {
    getImage();
}

void UCamera::changeNotifyImage(UVar& var) {
    // Always unnotify
    image.unnotify();
    if(var.as<bool>())
        UNotifyAccess(image, &UCamera::getImage);  
}


void UCamera::changeFlipImage() {
    int tmp = flip.as<int>();
    if (((flip == 0 || flip == 2) && (mFlipImage == flipD90 || mFlipImage == flipD270)) ||
    		((flip == 1 || flip == 3) && (mFlipImage == flipD0 || mFlipImage == flipD180))) {
    	width = mMatImage.rows;
    	height = mMatImage.cols;
    }
    switch(tmp) {
    case 0:
    	mFlipImage = flipD0;
    	break;
    case 1:
    	mFlipImage = flipD90;
       	break;
    case 2:
    	mFlipImage = flipD180;
    	break;
    case 3:
    	mFlipImage = flipD270;
    	break;
    default:
    	throw runtime_error("flip should be from 0 to 3");
    	break;
    }
    mBinImage.image.width = width.as<size_t > ();
    mBinImage.image.height = height.as<size_t > ();
}

int UCamera::update() {
    if (mAccessFrame != mFrame) {
        mGetNewFrame = true;
        mAccessFrame = mFrame;
    }
    return 0;
}

void UCamera::fpsChanged() {
    cerr << "UCamera::fpsChanged()" << endl
            << "\tCamera fps changed to " << fps.as<int>() << endl;
    USetUpdate(fps.as<int>() > 0 ? 1000.0 / fps.as<int>() : -1.0);
}

UStart(UCamera);
