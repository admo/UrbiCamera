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
#include <urbi/uconversion.hh>
#include <urbi/uabstractclient.hh>

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

    bool mGetNewFrame;
    unsigned int mFrame;
    unsigned int mAccessFrame;

    // Called on access.
    void getImage(UVar&);

    // Storage for last captured image. 
    UBinary mBinImage;

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

    Mat mMatImage;

    void fpsChanged();
};

UCamera::UCamera(const std::string& s) : grabImageMutexLock(grabImageMutex), urbi::UObject(s) {
    cerr << "UCamera::UCamera(" << s << ")" << endl;
    UBindFunction(UCamera, init);
}

UCamera::~UCamera() {
    cerr << "UCamera::~UCamera()" << endl;
    grabImageThread.interrupt();
    grabImageThread.join();
}

void UCamera::init(int id) {
    cerr << "UCamera::init(" << id << ")" << endl;
    // Urbi constructor
    mGetNewFrame = true;
    mFrame = mAccessFrame = 0;

    if (!videoCapture.open(id))
        throw runtime_error("Failed to initialize camera");

    //Bind all variables
    UBindVar(UCamera, image);
    UBindVar(UCamera, width);
    UBindVar(UCamera, height);
    UBindVar(UCamera, fps);

    // Notify if fps changed
    UNotifyChange(fps, &UCamera::fpsChanged);

    // Get image size
    videoCapture >> mMatImage;
    width = mMatImage.cols;
    height = mMatImage.rows;

    cerr << "\tRetreived image size: x=" << width.as<int>() << " y=" << height.as<int>() << endl;

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
//    cerr << "UCamera::grabImageThreadFunction()" << endl
//            << "\tThread started" << endl;
    try {
        while (true) {
            // Set interruption point
            this_thread::interruption_point();
            // Grab image from camera
            videoCapture.grab();
            ++mFrame;
            // Try to populate data
            if (grabImageMutex.try_lock()) {
//                cerr << "UCamera::grabImageThreadFunction()" << endl
//                        << "\tPopulating retrieved image nr " << mFrame << endl;
                videoCapture.retrieve(mMatImage);
                cvtColor(mMatImage, mMatImage, CV_BGR2RGB);
                grabImageCond.notify_one();
                grabImageMutex.unlock();
            }
        }
    } catch (boost::thread_interrupted&) {
//        cerr << "UCamera::grabImageThreadFunction()" << endl
//                << "\tThread stopped" << endl;
        videoCapture.release();
        return;
    }
}

void UCamera::getImage(UVar& val) {
    lock_guard<mutex> lock(getValMutex);
    if (!mGetNewFrame) {
//        cerr << "UCamera:getVal()" << endl
//                << "\tThere is no need to get new image" << endl;
    } else {
        mGetNewFrame = false;
//        cerr << "UCamera:getVal()" << endl
//                << "\tThere is need to get new image" << endl;

        grabImageCond.wait(grabImageMutexLock);
        mBinImage.image.data = mMatImage.data;
        val = mBinImage;
    }
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
