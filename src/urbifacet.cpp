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

#include <urbi/uobject.hh>

#include <vector>

#include "facet.h"

using namespace cv;
using namespace urbi;

class UFacet : public UObject
{
public:
  UFacet(const std::string&);
  ~UFacet();

public:
  int init(UVar&);		// init object
  void changeScale(UVar&);	// change scale function
  void changeMode(UVar&);	// change mode function
  bool loadSettings(const std::string); // load algorithms parameters
  void SetImage(UBinary );	// image processing function
  UVar * source;			// image source

private :
  CvHaarClassifierCascade* cv_cascade;	// haar cascade container
  CvMemStorage* storage;	// haar storage container
  IplImage*		smallimg;	// image for processing
  bool			need_update;// update image settings flag
  double		t;			// temporary time variable
  double		tick;		// -||-

  UVar scale;	// image scale
  UVar width;	// image width
  UVar height;	// image height
  UVar mode;	// mode
  UVar faces;	// number of detected faces
  UVar time;	// processing time
  UVar fps;		// fps processing

  UVar roix;	//  list of face X coordinate (pixels)
  UVar roiy;	//	list of face Y coordinate (pixels)
  UVar angle;	//	list of face declination angle (not verified, for future use)
  UVar LEbBnd;	//	list of left eyebrow bend angle (top)
  UVar LEbDcl;	//	list of left eyebrow declination angle (side)
  UVar LEyOpn;	//	list of distance between the right eyelids (rel. eyeball subregion)
  UVar LEbHgt;	//	list of distance between left pupil and eyebrow top (rel. eye subregion)
  UVar REbBnd;	//	list of right eyebrow bend angle (top)
  UVar REbDcl;	//  list of right eyebrow declination angle (side)
  UVar REyOpn;	//	list of distance between the right eyelids (rel. eyeball subregion)
  UVar REbHgt;	//	list of distance between right pupil and eyebrow top (rel. eye subregion)
  UVar LiAspt;	//	list of aspect ratio of the lips bounding box (percents)
  UVar LLiCnr;	//	list of Y position of the left corner of the lips (rel. lips bounding box)
  UVar RLiCnr;	//	list of Y position of the right corner of the lips (rel. lips bounding box)
  UVar Wrnkls;	//	list of number of horizontal wrinkles in the center of the forehead
  UVar Nstrls;	//	list of nostrils baseline width (rel. face width)
  UVar TeethA;	//	list of area of the visible teeth (rel. lips bounding box)

  Facet facet;

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


  InputPort input;	// Input port for mode 2

  UVar cvimg;	//IplImage  variable
  UVar uimg;	//UImage  variable

  UBinary ubin;		// Storage for last captured image.
  UImage  uimage;	// Last captured image in UImage format.
};

UFacet::UFacet(const std::string& s)
  : UObject(s)
{
	UBindFunction(UFacet, init);
}

UFacet::~UFacet()
{
	// Release all objects
	cvReleaseImage(&smallimg);
};


int UFacet::init(UVar& src)
{

	// Bing all urbi variables
	UBindVar(UFacet, mode);
	UBindVar(UFacet, input);
	UBindVar(UFacet, scale);
	UBindVar(UFacet, width);
	UBindVar(UFacet, height);
	UBindVar(UFacet, time);
	UBindVar(UFacet, fps);

	UBindVar(UFacet, faces);
	UBindVar(UFacet, roix);
	UBindVar(UFacet, roiy);
	UBindVar(UFacet, angle);
	UBindVar(UFacet, LEbBnd);
	UBindVar(UFacet, LEbDcl);
	UBindVar(UFacet, LEyOpn);
	UBindVar(UFacet, LEbHgt);
	UBindVar(UFacet, REbBnd);
	UBindVar(UFacet, REbDcl);
	UBindVar(UFacet, REyOpn);
	UBindVar(UFacet, REbHgt);
	UBindVar(UFacet, LiAspt);
	UBindVar(UFacet, LLiCnr);
	UBindVar(UFacet, RLiCnr);
	UBindVar(UFacet, Wrnkls);
	UBindVar(UFacet, Nstrls);
	UBindVar(UFacet, TeethA);

	UBindVar(UFacet, cvimg);
	UBindVar(UFacet, uimg);

	// Bind functions
	UBindThreadedFunction(UFacet, SetImage, LOCK_INSTANCE);
	UBindFunction(UFacet, loadSettings);


	// set default parameters

	scale=1;
	need_update=true;
	height = -1;
	width = -1;
	mode=0;
	source=new UVar(src);
	smallimg=0;
	faces=0;


	UNotifyChange(scale, &UFacet::changeScale);
	UNotifyChange(mode, &UFacet::changeMode);

	//initialize container
	ubin.type = BINARY_UNKNOWN;
	ubin.common.size = sizeof(IplImage);
	cout <<"UFACET initialized correctly."<<endl;


	return 0;
}
//
// Mode change function
//
void UFacet::changeMode(UVar&)
{
	int tmp=mode;
	switch(tmp){
		case 0:
			source->unnotify();
			input.unnotify();
			break;
		case 1:
			// notiffy function change follows from mode
			input.unnotify();
			UNotifyChange(*source, &UFacet::SetImage);
			break;
		case 2:
			// notiffy function change follows from mode
			source->unnotify();
			UNotifyChange(input, &UFacet::SetImage);
			break;
	}
	cout <<"UFacET mode set " << tmp <<endl;
	return;
}

//
// Scale change function (if scale variable changes)
//
void UFacet::changeScale(UVar&)
{
	need_update=true;
	cout <<"UFacET scale set " <<(int)scale<<endl;
	return;
}

//
// Load FacET algorithms parameters
//
bool UFacet::loadSettings(string path)
{
	need_update=true;
	if (path=="")   return facet.readSettings("default.cfg");
	else return facet.readSettings(path);
}

//
// Image processing function (if image source changes)
//
void UFacet::SetImage(UBinary src)
{

	// set UBinary
	// get the pointer of frame from UBinary
	IplImage* img = (IplImage*) src.common.data;
		if(!img) return;

	if (need_update){
		need_update=false;

		// Release previous created image
		cvReleaseImage(&smallimg);

		// Create a new image based on the input image and new parameters
		smallimg = cvCreateImage( cvSize((img->width/(double)scale),(img->height/(double)scale)), 8, 3 );

		// Create new Uimage parameters
		uimage.width = smallimg->width;
		uimage.height = smallimg->height;
		uimage.imageFormat = IMAGE_RGB;
		uimage.data = 0;
		uimage.size = smallimg->width * smallimg->height * 3;
		width = uimage.width;
		height = uimage.height;
	}

	// Resize or copy new image to smallimg variable
	if (((double)scale)!=1) cvResize( img, smallimg, CV_INTER_LINEAR );
	else cvCopy(img,smallimg,0);

	// ...to measure all processing time
	double t = (double)cvGetTickCount();

	// Compute fps - algorithm efficency
	fps=((double)cvGetTickFrequency()*1000000)/(t-tick);
	tick=t;


	//////////////////////////////////////////////////////////
	// FACET  FACET  FACET  FACET  FACET  FACET  FACET  FACET

	facet.face.clearElements();
	facet.detectFeat(smallimg,smallimg);

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
	faces=facet.facesList.size();

	for (std::list<facepar_t>::iterator iter = facet.facesList.begin();
		  iter != facet.facesList.end(); ++iter) {
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

	  roix=vec_roix;
	  roiy=vec_roiy;
	  angle=vec_angle;
	  LEbBnd=vec_LEbBnd;
	  LEbDcl=vec_LEbDcl;
	  LEyOpn=vec_LEyOpn;
	  LEbHgt=vec_LEbHgt;
	  REbBnd=vec_REbBnd;
	  REbDcl=vec_REbDcl;
	  REyOpn=vec_REyOpn;
	  REbHgt=vec_REbHgt;
	  LiAspt=vec_LiAspt;
	  LLiCnr=vec_LLiCnr;
	  RLiCnr=vec_RLiCnr;
	  Wrnkls=vec_Wrnkls;
	  Nstrls=vec_Nstrls;
	  TeethA=vec_TeethA;

	facet.cleanFacesList();


	// FACET  FACET  FACET  FACET  FACET  FACET  FACET  FACET
	//////////////////////////////////////////////////////////


	t = (double)cvGetTickCount() - t;
	time= t/((double)cvGetTickFrequency()*1000.);


	// Assign image to UBinary variable.
	ubin.common.data = smallimg;
	cvimg = ubin;

	// Save to UImage also
	uimage.data=(unsigned char*)smallimg->imageData;
	uimg=uimage;


}


UStart(UFacet);
