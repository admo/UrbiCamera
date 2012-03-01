/*******************************************
 *
 *	UFacet version 1.2
 *   UModule based on FacET library for detecting and parameterising face components.
 *   Copyright (C) 2009  Marek Wnuk <marek.wnuk@pwr.wroc.pl>
 *	Compiled with OpenCV 2.3.1 with TBB libraries
 *   Author: Jan Kedzierski
 *   date: 27.12.2011
 *
 ********************************************/

#include <vector>
#include <list>
#include <string>

#include <boost/scoped_ptr.hpp>

#include <urbi/uobject.hh>

#include <cv.h>
#include <highgui.h>

#include <iostream>

#include "facet.h"

using namespace cv;
using namespace urbi;
using namespace std;
using namespace boost;

class UFacet: public UObject {
public:
	UFacet(const std::string&);
	~UFacet();

	int init(UVar&); // init object

private:
	void changeNotifyImage(UVar&); // change mode function
	void changeScale(UVar&); // change scale function
	bool loadSettings(const std::string); // load algorithms parameters
	void detectFrom(UImage); // image processing function
	void SetImage(UImage);

	Mat mResultImage;

	int64 mLastTick;

	scoped_ptr<Facet> mFacet;

	// Variables definig the class states
	UVar notify;
	UVar mode;
	UVar scale; // image scale
	UVar width; // image width
	UVar height; // image height
	UVar fps; // fps processing
	UVar image; //image after processing
	scoped_ptr<UVar> mInputImage;
	UBinary mBinImage;

	UVar faces; // number of detected faces
	UVar roix; //  list of face X coordinate (pixels)
	UVar roiy; //	list of face Y coordinate (pixels)
	UVar angle; //	list of face declination angle (not verified, for future use)
	UVar LEbBnd; //	list of left eyebrow bend angle (top)
	UVar LEbDcl; //	list of left eyebrow declination angle (side)
	UVar LEyOpn; //	list of distance between the right eyelids (rel. eyeball subregion)
	UVar LEbHgt; //	list of distance between left pupil and eyebrow top (rel. eye subregion)
	UVar REbBnd; //	list of right eyebrow bend angle (top)
	UVar REbDcl; //  list of right eyebrow declination angle (side)
	UVar REyOpn; //	list of distance between the right eyelids (rel. eyeball subregion)
	UVar REbHgt; //	list of distance between right pupil and eyebrow top (rel. eye subregion)
	UVar LiAspt; //	list of aspect ratio of the lips bounding box (percents)
	UVar LLiCnr; //	list of Y position of the left corner of the lips (rel. lips bounding box)
	UVar RLiCnr; //	list of Y position of the right corner of the lips (rel. lips bounding box)
	UVar Wrnkls; //	list of number of horizontal wrinkles in the center of the forehead
	UVar Nstrls; //	list of nostrils baseline width (rel. face width)
	UVar TeethA; //	list of area of the visible teeth (rel. lips bounding box)

	vector<double> vec_roix;
	vector<double> vec_roiy;
	vector<double> vec_angle;
	vector<double> vec_LEbBnd;
	vector<double> vec_LEbDcl;
	vector<double> vec_LEyOpn;
	vector<double> vec_LEbHgt;
	vector<double> vec_REbBnd;
	vector<double> vec_REbDcl;
	vector<double> vec_REyOpn;
	vector<double> vec_REbHgt;
	vector<double> vec_LiAspt;
	vector<double> vec_LLiCnr;
	vector<double> vec_RLiCnr;
	vector<double> vec_Wrnkls;
	vector<double> vec_Nstrls;
	vector<double> vec_TeethA;
};

UFacet::UFacet(const std::string& s) :
		UObject(s), mFacet(NULL), mInputImage(NULL) {
	UBindFunction(UFacet, init);
}

UFacet::~UFacet() {
	// Prevent of double free error
	if (mBinImage.image.data == mResultImage.data)
		mBinImage.image.data = 0;
}

int UFacet::init(UVar& sourceImage) {

	UBindVars(
			UFacet,
			notify, mode, scale, width, height, fps, image, faces, roix, roiy, angle, LEbBnd, LEbDcl, LEyOpn, LEbHgt, REbBnd, REbDcl, REyOpn, REbHgt, LiAspt, LLiCnr, RLiCnr, Wrnkls, Nstrls, TeethA);

	// Bind functions
	UBindThreadedFunction(UFacet, detectFrom, LOCK_INSTANCE);
	UBindThreadedFunction(UFacet, SetImage, LOCK_INSTANCE);
	UBindFunction(UFacet, loadSettings);

	mBinImage.type = BINARY_IMAGE;
	mBinImage.image.imageFormat = IMAGE_RGB;

	// set default parameters
	scale = 1;
	height = -1;
	width = -1;
	mode = 0;
	faces = 0;

	mInputImage.reset(new UVar(sourceImage));

	mFacet.reset(new Facet);

	UNotifyChange(scale, &UFacet::changeScale);
	UNotifyChange(mode, &UFacet::changeNotifyImage);

	return 0;
}

void UFacet::changeNotifyImage(UVar& var) {
	mInputImage->unnotify();
	if (var.as<bool>())
		UNotifyChange(*mInputImage, &UFacet::detectFrom);

	mode = var.as<bool>();
	notify = var.as<bool>();
}

void UFacet::changeScale(UVar& newScale) {
	double tmp = newScale.as<double>();
	tmp = tmp > 1.0 ? tmp : 1.0;
	scale = tmp;
}

//
// Load FacET algorithms parameters
//
bool UFacet::loadSettings(string path) {
	if (path == "")
		return mFacet->readSettings();
	else
		return mFacet->readSettings(path);
}

//
// Image processing function (if image source changes)
//
void UFacet::detectFrom(UImage sourceImage) {

	Mat processImage(Size(sourceImage.width, sourceImage.height), CV_8UC3,
			sourceImage.data);

	Mat resizedImage(cvRound(processImage.rows / scale.as<double>()),
			cvRound(processImage.cols / scale.as<double>()), CV_8UC1);
	resize(processImage, resizedImage, resizedImage.size(), 0, 0, INTER_LINEAR);
	width = resizedImage.cols;
	height = resizedImage.rows;

	//Compute fps - algorithm efficency
	int64 startTick = getTickCount();
	fps = static_cast<double>(getTickFrequency()) / (startTick - mLastTick);
	mLastTick = startTick;
	double timestamp = (double) mLastTick / getTickFrequency();

	//////////////////////////////////////////////////////////
	// FACET  FACET  FACET  FACET  FACET  FACET  FACET  FACET
	IplImage iplimg = resizedImage;

	mFacet->face.clearElements();
	mFacet->detectFeat(&iplimg, &iplimg);

	vec_roix.clear();
	vec_roiy.clear();
	vec_angle.clear();
	vec_LEbBnd.clear();
	vec_LEbDcl.clear();
	vec_LEyOpn.clear();
	vec_LEbHgt.clear();
	vec_REbBnd.clear();
	vec_REbDcl.clear();
	vec_REyOpn.clear();
	vec_REbHgt.clear();
	vec_LiAspt.clear();
	vec_LLiCnr.clear();
	vec_RLiCnr.clear();
	vec_Wrnkls.clear();
	vec_Nstrls.clear();
	vec_TeethA.clear();
	faces = mFacet->facesList.size();

	for (std::list<facepar_t>::iterator iter = mFacet->facesList.begin();
			iter != mFacet->facesList.end(); ++iter) {
		vec_roix.push_back(iter->roix);
		vec_roiy.push_back(iter->roiy);
		vec_angle.push_back(iter->angle);
		vec_LEbBnd.push_back(iter->LEbBnd);
		vec_LEbDcl.push_back(iter->LEbDcl);
		vec_LEyOpn.push_back(iter->LEyOpn);
		vec_LEbHgt.push_back(iter->LEbHgt);
		vec_REbBnd.push_back(iter->REbBnd);
		vec_REbDcl.push_back(iter->REbDcl);
		vec_REyOpn.push_back(iter->REyOpn);
		vec_REbHgt.push_back(iter->REbHgt);
		vec_LiAspt.push_back(iter->LiAspt);
		vec_LLiCnr.push_back(iter->LLiCnr);
		vec_RLiCnr.push_back(iter->RLiCnr);
		vec_Wrnkls.push_back(iter->Wrnkls);
		vec_Nstrls.push_back(iter->Nstrls);
		vec_TeethA.push_back(iter->TeethA);
	}

	roix = vec_roix;
	roiy = vec_roiy;
	angle = vec_angle;
	LEbBnd = vec_LEbBnd;
	LEbDcl = vec_LEbDcl;
	LEyOpn = vec_LEyOpn;
	LEbHgt = vec_LEbHgt;
	REbBnd = vec_REbBnd;
	REbDcl = vec_REbDcl;
	REyOpn = vec_REyOpn;
	REbHgt = vec_REbHgt;
	LiAspt = vec_LiAspt;
	LLiCnr = vec_LLiCnr;
	RLiCnr = vec_RLiCnr;
	Wrnkls = vec_Wrnkls;
	Nstrls = vec_Nstrls;
	TeethA = vec_TeethA;

	mFacet->cleanFacesList();

	// Copy mResult image to UImage
	mBinImage.image.width = resizedImage.cols;
	mBinImage.image.height = resizedImage.rows;
	mBinImage.image.size = resizedImage.cols * resizedImage.rows * 3;
	mBinImage.image.data = resizedImage.data;
	image = mBinImage;
}

void UFacet::SetImage(UImage image) {
	detectFrom(image);
}

UStart(UFacet);
