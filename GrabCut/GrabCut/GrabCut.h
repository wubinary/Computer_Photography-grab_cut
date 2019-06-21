#pragma once
#include <opencv/cv.h>
#include <iostream>
#include "GMM.h"
#include "gcgraph.h"

using namespace cv;

enum
{
	GC_WITH_RECT  = 0, 
	GC_WITH_MASK  = 1, 
	GC_CUT        = 2  
};
class GrabCut2D
{
public:
	void GrabCut( cv::InputArray _img, cv::InputOutputArray _mask, cv::Rect rect,
		cv::InputOutputArray _bgdModel,cv::InputOutputArray _fgdModel,
		int iterCount, int mode );  
	
	void initMask(Mat &, Size, Rect);

	void samplePoints(const Mat img, const Mat mask,
		std::vector<Vec3f> &fgdSamples, std::vector<Vec3f> &bgdSamples, Mat &fgdLabel, Mat &bgdLabel);

	void learnGMMs(const Mat &img, const Mat &mask, GMM &fgdGMM, GMM &bgdGMM, 
		std::vector<Vec3f> &fgdSamples, std::vector<Vec3f> &bgdSamples, const Mat &fgdLabel, const Mat &bgdLabel);
	
	double calcBeta(const Mat &img);

	void constructGraph(const Mat &img, const Mat &mask, const GMM &fgdGMM, const GMM &bgdGMM, GCGraph<double> &graph);
	~GrabCut2D(void);

	void estimateSegmentation(GCGraph<double>& graph, Mat& mask);
};

