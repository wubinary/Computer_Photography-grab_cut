#include "GrabCut.h"

GrabCut2D::~GrabCut2D(void)
{
}

void GrabCut2D::GrabCut(cv::InputArray _img, cv::InputOutputArray _mask, cv::Rect rect, cv::InputOutputArray _bgdModel, cv::InputOutputArray _fgdModel, int iterCount, int mode)
{
	std::cout << "\n[Info] Execute GrabCut Function" << std::endl;

	//1.Load Input Image: 加载输入颜色图像;
	Mat img = _img.getMat();

	//2.Init Mask: 用矩形框初始化Mask的Label值（确定背景：0， 确定前景：1，可能背景：2，可能前景：3）,矩形框以外设置为确定背景，矩形框以内设置为可能前景;
	Mat &mask = _mask.getMatRef();
	if (mode == GC_WITH_RECT)
		initMask(mask,img.size(),rect);

	//3.Init GMM: 定义并初始化GMM(其他模型完成分割也可得到基本分数，GMM完成会加分）
	GMM fgdGMM(_bgdModel.getMatRef()), bgdGMM(_bgdModel.getMatRef());
	
	for (int iter = 1; iter <= iterCount; iter++)
	{
		//4.Sample Points:前背景颜色采样并进行聚类（建议用kmeans，其他聚类方法也可)
		Mat fgdLabel, bgdLabel;
		std::vector<Vec3f> fgdSamples, bgdSamples;
		samplePoints(img, mask, fgdSamples, bgdSamples, fgdLabel, bgdLabel);

		//5.Learn GMM(根据聚类的样本更新每个GMM组件中的均值、协方差等参数）
		learnGMMs(img, mask, fgdGMM, bgdGMM, fgdSamples, bgdSamples, fgdLabel, bgdLabel);

		//6.Construct Graph（计算t-weight(数据项）和n-weight（平滑项））
		GCGraph<double> graph;
		constructGraph(img, mask, fgdGMM, bgdGMM, graph);

		//7.Estimate Segmentation(调用maxFlow库进行分割)
		estimateSegmentation(graph, mask);

		cout << "\t Iteration:" << iter << " complete ! " << endl;
	}
	std::cout << "[Info] Execute GrabCut Function complete\n" << std::endl;
}

void GrabCut2D::estimateSegmentation(GCGraph<double>& graph, Mat& mask)
{
	graph.maxFlow();
	Point p;
	for (p.y = 0; p.y < mask.rows; p.y++)
	{
		for (p.x = 0; p.x < mask.cols; p.x++)
		{
			if (mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD)
			{
				if (graph.inSourceSegment(p.y*mask.cols + p.x /*vertex index*/))
					mask.at<uchar>(p) = GC_PR_FGD;
				else
					mask.at<uchar>(p) = GC_PR_BGD;
			}
		}
	}
}

void GrabCut2D::constructGraph(const Mat &img, const Mat &mask, const GMM &fgdGMM, const GMM &bgdGMM, GCGraph<double> &graph)
{
	int vtxCount=img.rows*img.cols,
		edgCount= 2 * (4 * img.cols*img.rows - 3 * (img.cols + img.rows) + 2);
	graph.create(vtxCount, edgCount);

	const double beta = calcBeta(img);
	const double gamma = 50;
	const double lambda = 9 * gamma;
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			int vtxId = graph.addVtx();

			// Add edge to Src or Dst
			double fromSrc, toSink;
			if (mask.at<uchar>(p) == GC_PR_FGD) {
				fromSrc = -log(bgdGMM((Vec3d)img.at<Vec3b>(p)));
				toSink = -log(fgdGMM((Vec3d)img.at<Vec3b>(p)));
			}
			else if (mask.at<uchar>(p) == GC_PR_BGD) {
				fromSrc = -log(bgdGMM((Vec3d)img.at<Vec3b>(p)));
				toSink = -log(fgdGMM((Vec3d)img.at<Vec3b>(p)));
			}
			else if (mask.at<uchar>(p) == GC_FGD) {
				fromSrc = lambda;
				toSink = 0;
			}
			else if (mask.at<uchar>(p) == GC_BGD) {
				fromSrc = 0;
				toSink = lambda;
			}
			graph.addTermWeights(vtxId, fromSrc, toSink);

			// Add edge to neighbor
			if (p.x > 0) { //Left
				Vec3d diff = (Vec3d)img.at<Vec3b>(p.y, p.x) - (Vec3d)img.at<Vec3b>(p.y, p.x - 1);
				double w = gamma * exp(-beta * diff.dot(diff));
				graph.addEdges(vtxId, vtxId - 1, w, w);
			}
			if (p.y > 0 && p.x > 0) { //Left Up
				Vec3d diff = (Vec3d)img.at<Vec3b>(p.y, p.x) - (Vec3d)img.at<Vec3b>(p.y - 1, p.x - 1);
				double w = gamma * exp(-beta * diff.dot(diff));
				graph.addEdges(vtxId, vtxId - img.cols - 1, w, w);
			}
			if (p.y > 0) { //Up
				Vec3d diff = (Vec3d)img.at<Vec3b>(p.y, p.x) - (Vec3d)img.at<Vec3b>(p.y - 1, p.x);
				double w = gamma * exp(-beta * diff.dot(diff));
				graph.addEdges(vtxId, vtxId - img.cols, w, w);
			}
			if (p.y > 0 && p.x < img.cols - 1) { //Up Right
				Vec3d diff = (Vec3d)img.at<Vec3b>(p.y, p.x) - (Vec3d)img.at<Vec3b>(p.y - 1, p.x + 1);
				double w = gamma * exp(-beta * diff.dot(diff));
				graph.addEdges(vtxId, vtxId - img.cols + 1, w, w);
			}
		}
	}
}

double GrabCut2D::calcBeta(const Mat &img)
{
	double sum = 0, count = 0;
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			Vec3d color = (Vec3d)img.at<Vec3b>(p);
			if (p.y > 0) { //Up
				Vec3d diff = color - (Vec3d)img.at<Vec3b>(p.y - 1, p.x);
				sum += diff.dot(diff);
				count++;
			}
			if (p.y > 0 && p.x < img.cols - 1) { //Up Right
				Vec3d diff = color - (Vec3d)img.at<Vec3b>(p.y - 1, p.x + 1);
				sum += diff.dot(diff);
				count++;
			}
			if (p.x < img.cols - 1) { //Right
				Vec3d diff = color - (Vec3d)img.at<Vec3b>(p.y, p.x + 1);
				sum += diff.dot(diff);
				count++;
			}
			if (p.y < img.rows - 1 && p.x < img.cols - 1) { //Down Right
				Vec3d diff = color - (Vec3d)img.at<Vec3b>(p.y + 1, p.x + 1);
				sum += diff.dot(diff);	
				count++;
			}
		}
	}
	double beta = 1.0/ 2.0 / (sum / count);
	return beta; // beta = 1 / (2 * avg(sqr(|| color[i] - color[j] || )))
}

void GrabCut2D::learnGMMs(const Mat&img, const Mat&mask, GMM &fgdGMM, GMM &bgdGMM, std::vector<Vec3f> &fgdSamples, std::vector<Vec3f> &bgdSamples, const Mat &fgdLabel, const Mat &bgdLabel)
{
	// learn from samplePoints
	fgdGMM.initLearning();
	bgdGMM.initLearning();

	for (int i = 0; i < (int)fgdSamples.size(); i++)
		fgdGMM.addSample(fgdLabel.at<int>(i, 0), fgdSamples[i]);
	for (int i = 0; i < (int)bgdSamples.size(); i++)
		bgdGMM.addSample(bgdLabel.at<int>(i, 0), bgdSamples[i]);

	fgdGMM.endLearning();
	bgdGMM.endLearning();
}

void GrabCut2D::samplePoints(const Mat img, const Mat mask, std::vector<Vec3f> &fgdSamples, std::vector<Vec3f> &bgdSamples, Mat &fgdLabel, Mat &bgdLabel)
{
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			if (mask.at<uchar>(p) == GC_FGD || mask.at<uchar>(p) == GC_PR_FGD)
				fgdSamples.push_back((Vec3f)img.at<Vec3b>(p));
			else
				bgdSamples.push_back((Vec3f)img.at<Vec3b>(p));
		}
	}

	// kmeans分类
	const int kMeansItCount = 10;
	kmeans(fgdSamples, 5, fgdLabel, TermCriteria(CV_TERMCRIT_ITER, kMeansItCount, 0.0), 5, KMEANS_PP_CENTERS);
	kmeans(bgdSamples, 5, bgdLabel, TermCriteria(CV_TERMCRIT_ITER, kMeansItCount, 0.0), 5, KMEANS_PP_CENTERS);
	//openCV kmeans(inputArray, K, inputOutputArray, cv::TermCriteria, attempts, flags) 
}

void GrabCut2D::initMask(Mat &_mask, Size size, Rect rect)
{
	_mask.create(size, CV_8UC1);
	_mask.setTo(GC_BGD);
	_mask(rect).setTo(Scalar(GC_PR_FGD));
}
