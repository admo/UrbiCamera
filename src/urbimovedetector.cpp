/* This file is part of UMoveDetection.
 * UMoveDetection is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 /*******************************************
 *
 *	UMoveDetection version 1.1
 *   Simple module to detect movement on the image
 *	Compiled with OpenCV 2.3.1 with TBB libraries
 *   Author: Jan Kedzierski
 *   date: 27.12.2011
 *
 ********************************************/
#include <urbi/uobject.hh>

#include <cv.h>
#include <highgui.h>

#include <vector>

#include <boost/circular_buffer.hpp>

#include <iostream>

using namespace cv;
using namespace std;
using namespace urbi;
using namespace boost;

class UMoveDetector: public UObject {
public:
	UMoveDetector(const string&);
	~UMoveDetector();

	int init(UVar& sourceImage);

private:
	void changeNotifyImage(UVar&);
	void changeScale(UVar&); // change scale function
	void changeImageBufferSize(UVar&);
	void detectFrom(UImage); // image processing function
	void SetImage(UImage);

	// Temporary variables for image processing function
	Mat mResultImage;
	Mat mMHI;

	int64 mLastTick;

	circular_buffer<Mat> mImageBuffer;

	UVar visible; // if object is visible
	UVar x; // position in x of the object center
	UVar y; // position in y of the object center
	UVar scale; // image scale
	UVar time; // processing time
	UVar width; // image width
	UVar height; // image height
	UVar fps; // fps processing
	UVar notifyImage;
	UVar mode; // mode
	UVar duration; // time window for analysis (in seconds)
	UVar frameBuffer; // number of cyclic frame buffer used for motion detection
					  // (should, probably, depend on FPS)
	UVar imageBufferSize;
	UVar diffThreshold; // difference betwen two frames treshold
	UVar smooth; // smooth filter parameter

	UVar image;
	UVar *mInputImage;
	UBinary mBinImage;
};

UMoveDetector::UMoveDetector(const std::string& s) :
		UObject(s) {
	UBindFunction(UMoveDetector, init);
}

UMoveDetector::~UMoveDetector() {
	// Prevent of double free error
	if (mBinImage.image.data == mResultImage.data)
		mBinImage.image.data = 0;

	if (mInputImage)
		delete mInputImage;
}

void UMoveDetector::changeNotifyImage(UVar& var) {
	// Always unnotify
	mInputImage->unnotify();
	if (var.as<bool>())
		UNotifyChange(*mInputImage, &UMoveDetector::detectFrom);
	mode = var.as<bool>();
	notifyImage = var.as<bool>();
}

int UMoveDetector::init(UVar& sourceImage) {
	// Bing all urbi variables
	UBindVars(
			UMoveDetector,
			scale, fps, width, height, visible, x, y, notifyImage, mode, image, duration, frameBuffer, imageBufferSize, diffThreshold, smooth, time);

	// Bind functions
	UBindThreadedFunction(UMoveDetector, detectFrom, LOCK_INSTANCE);
	UBindThreadedFunction(UMoveDetector, SetImage, LOCK_INSTANCE);

	mBinImage.type = BINARY_IMAGE;
	mBinImage.image.imageFormat = IMAGE_RGB;

	// Set default parameters
	x = 0;
	y = 0;
	visible = 0;
	scale = 1;
	height = -1;
	width = -1;

	duration = 1; // time window for analysis (in seconds)
	frameBuffer = 2; // number of cyclic frame buffer used for motion detection
	mImageBuffer.resize(frameBuffer.as<int>());
	diffThreshold = 30; // difference betwen two frames treshold
	smooth = 31; // smooth filter parameter

	mInputImage = new UVar(sourceImage);

	// Notify variables
	UNotifyChange(*mInputImage, &UMoveDetector::detectFrom);
	UNotifyChange(notifyImage, &UMoveDetector::changeNotifyImage);
	UNotifyChange(mode, &UMoveDetector::changeNotifyImage);
	UNotifyChange(scale, &UMoveDetector::changeScale);
	UNotifyChange(frameBuffer, &UMoveDetector::changeImageBufferSize);
	UNotifyChange(imageBufferSize, &UMoveDetector::changeImageBufferSize);

	return 0;
}

void UMoveDetector::changeScale(UVar& newScale) {
	double tmp = newScale.as<double>();
	tmp = tmp > 1.0 ? tmp : 1.0;
	scale = tmp;
	mImageBuffer.clear();
}

void UMoveDetector::changeImageBufferSize(UVar& newBufferSize) {
	int tmp = newBufferSize.as<int>();
	tmp = tmp > 0 ? tmp : 1;
	imageBufferSize = tmp;
	frameBuffer = tmp;
	mImageBuffer.resize(tmp);
	mImageBuffer.clear();
	return;
}

void UMoveDetector::detectFrom(UImage sourceImage) {
	// Build MatImage with data from uImage
	Mat processImage(Size(sourceImage.width, sourceImage.height), CV_8UC3,
			sourceImage.data);

	//Resize image
	Mat resizedImage(cvRound(processImage.rows / scale.as<double>()),
			cvRound(processImage.cols / scale.as<double>()), CV_8UC1);
	resize(processImage, resizedImage, resizedImage.size(), 0, 0, INTER_LINEAR);
	width = resizedImage.cols;
	height = resizedImage.rows;

	if (resizedImage.size() != mMHI.size()) {
		mMHI = Mat::zeros(resizedImage.size(), CV_32F);
	}

	// Copy image to mMatImage as grayscaled image
	Mat grayscaleImage;
	cvtColor(resizedImage, grayscaleImage, CV_RGB2GRAY);
	cvtColor(grayscaleImage, mResultImage, CV_GRAY2RGB);
	mImageBuffer.push_back(grayscaleImage);

	if (!mImageBuffer.full())
		return;

	//Compute fps - algorithm efficency
	int64 startTick = getTickCount();
	fps = static_cast<double>(getTickFrequency()) / (startTick - mLastTick);
	mLastTick = startTick;
	double timestamp = (double) mLastTick / getTickFrequency();

	Mat silh;
	absdiff(mImageBuffer.front(), mImageBuffer.back(), silh);
	threshold(silh, silh, diffThreshold.as<double>(), 1, CV_THRESH_BINARY);
	updateMotionHistory(silh, mMHI, timestamp, duration.as<double>());

	Mat thresholdImage;
	mMHI.convertTo(thresholdImage, CV_8U, 255. / duration.as<double>(),
			(duration.as<double>() - timestamp) * 255. / duration.as<double>());
	threshold(thresholdImage, thresholdImage, 1, 255, CV_THRESH_BINARY);
	medianBlur(thresholdImage, thresholdImage, smooth.as<int>());

	Mat_<Vec3b> greenImage(thresholdImage.size(), Vec3b(255,0,0));
	add(greenImage, mResultImage, mResultImage, thresholdImage);

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
		line(mResultImage, Point(xx, yy),
				Point(mResultImage.cols / 2, mResultImage.rows / 2),
				Scalar(255, 0, 0), 2);
	} else {
		x = 0;
		y = 0;
		visible = 0;
	}

	// Draw horizontal and vertical line in the middle of the image
	line(mResultImage, Point(0, mResultImage.rows / 2),
			Point(mResultImage.cols, mResultImage.rows / 2),
			Scalar(100, 100, 100), 1);
	line(mResultImage, Point(mResultImage.cols / 2, 0),
			Point(mResultImage.cols / 2, mResultImage.rows),
			Scalar(100, 100, 100), 1);

	// Copy mResult image to UImage
	mBinImage.image.width = mResultImage.cols;
	mBinImage.image.height = mResultImage.rows;
	mBinImage.image.size = mResultImage.cols * mResultImage.rows * 3;
	mBinImage.image.data = mResultImage.data;
	image = mBinImage;
}

void UMoveDetector::SetImage(UImage src) {
	detectFrom(src);
}

UStart(UMoveDetector);
