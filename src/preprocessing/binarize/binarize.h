/*
 * binarize.h
 *
 *  Created on: Jan 13, 2015
 *      Author: xxy
 */

#ifndef PREPROCESSING_SRC_BINARIZE_H_
#define PREPROCESSING_SRC_BINARIZE_H_
#include <opencv2/opencv.hpp>
#include <iostream>
#include "../utils/FileUtil.h"
#include "../noiseLevel/noiseLevel.h"

using namespace std;
using namespace cv;

enum NiblackVersion {
	NIBLACK = 0, SAUVOLA, WOLFJOLION,
};

#define BINARIZEWOLF_VERSION	"2.4 (August 1st, 2014)"

#define uget(x,y)    at<unsigned char>(y,x)
#define uset(x,y,v)  at<unsigned char>(y,x)=v;
#define fget(x,y)    at<float>(y,x)
#define fset(x,y,v)  at<float>(y,x)=v;
void binarize(Mat& src, Mat& dst, NiblackVersion versionCode, int winx = 0,
		int winy = 0, float optK = 0.5);
void binarizeDir(const char* inputDir, const char* outputDir,
		NiblackVersion versionCode, int winx = 0, int winy = 0,
		float optK = 0.5);

class Binarize {
public:
	// *************************************************************
	// glide a window across the image and
	// create two maps: mean and standard deviation.
	//
	// Version patched by Thibault Yohan (using opencv integral images)
	// *************************************************************
	static double calcLocalStats(Mat &im, Mat &map_m, Mat &map_s, int winx,
			int winy) {
		Mat im_sum, im_sum_sq;
		cv::integral(im, im_sum, im_sum_sq, CV_64F);

		double m, s, max_s, sum, sum_sq;
		int wxh = winx / 2;
		int wyh = winy / 2;
		int x_firstth = wxh;
		int y_lastth = im.rows - wyh - 1;
		int y_firstth = wyh;
		double winarea = winx * winy;

		max_s = 0;
		for (int j = y_firstth; j <= y_lastth; j++) {
			sum = sum_sq = 0;

			sum = im_sum.at<double>(j - wyh + winy, winx)
					- im_sum.at<double>(j - wyh, winx)
					- im_sum.at<double>(j - wyh + winy, 0)
					+ im_sum.at<double>(j - wyh, 0);
			sum_sq = im_sum_sq.at<double>(j - wyh + winy, winx)
					- im_sum_sq.at<double>(j - wyh, winx)
					- im_sum_sq.at<double>(j - wyh + winy, 0)
					+ im_sum_sq.at<double>(j - wyh, 0);

			m = sum / winarea;
			s = sqrt((sum_sq - m * sum) / winarea);
			if (s > max_s)
				max_s = s;

			map_m.fset(x_firstth, j, m);
			map_s.fset(x_firstth, j, s);

			// Shift the window, add and remove	new/old values to the histogram
			for (int i = 1; i <= im.cols - winx; i++) {

				// Remove the left old column and add the right new column
				sum -= im_sum.at<double>(j - wyh + winy, i)
						- im_sum.at<double>(j - wyh, i)
						- im_sum.at<double>(j - wyh + winy, i - 1)
						+ im_sum.at<double>(j - wyh, i - 1);
				sum += im_sum.at<double>(j - wyh + winy, i + winx)
						- im_sum.at<double>(j - wyh, i + winx)
						- im_sum.at<double>(j - wyh + winy, i + winx - 1)
						+ im_sum.at<double>(j - wyh, i + winx - 1);

				sum_sq -= im_sum_sq.at<double>(j - wyh + winy, i)
						- im_sum_sq.at<double>(j - wyh, i)
						- im_sum_sq.at<double>(j - wyh + winy, i - 1)
						+ im_sum_sq.at<double>(j - wyh, i - 1);
				sum_sq += im_sum_sq.at<double>(j - wyh + winy, i + winx)
						- im_sum_sq.at<double>(j - wyh, i + winx)
						- im_sum_sq.at<double>(j - wyh + winy, i + winx - 1)
						+ im_sum_sq.at<double>(j - wyh, i + winx - 1);

				m = sum / winarea;
				s = sqrt((sum_sq - m * sum) / winarea);
				if (s > max_s)
					max_s = s;

				map_m.fset(i+wxh, j, m);
				map_s.fset(i+wxh, j, s);
			}
		}

		return max_s;
	}

	/**********************************************************
	 * The binarization routine
	 **********************************************************/
	static void NiblackSauvolaWolfJolion(Mat& img, Mat& output,
			NiblackVersion version, int winx, int winy, double k, double dR) {

		Size sz = Size(img.cols/2, img.rows/2);
		Mat imgSmall = Mat(sz, img.type());
		cv::resize(img, imgSmall, sz);

		output.create(img.size(), img.type());
		double m, s, max_s;
		double th = 0;
		double min_I, max_I;
		int wxh = winx / 2;
		int wyh = winy / 2;
		int x_firstth = wxh;
		int x_lastth = imgSmall.cols - wxh - 1;
		int y_lastth = imgSmall.rows - wyh - 1;
		int y_firstth = wyh;

		// Create local statistics and store them in a double matrices
		Mat map_m = Mat::zeros(imgSmall.rows, imgSmall.cols, CV_32F);
		Mat map_s = Mat::zeros(imgSmall.rows, imgSmall.cols, CV_32F);
		max_s = calcLocalStats(imgSmall, map_m, map_s, winx, winy);

		minMaxLoc(imgSmall, &min_I, &max_I);

		Mat thsurf(imgSmall.rows, imgSmall.cols, CV_32F);

		// Create the threshold surface, including border processing
		// ----------------------------------------------------

		for (int j = y_firstth; j <= y_lastth; j++) {

			// NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
			for (int i = 0; i <= imgSmall.cols - winx; i++) {

				m = map_m.fget(i+wxh, j);
				s = map_s.fget(i+wxh, j);

				// Calculate the threshold
				switch (version) {

					case NIBLACK:
					th = m + k*s;
					break;

					case SAUVOLA:
					th = m * (1 + k*(s/dR-1));
					break;

					case WOLFJOLION:
					th = m + k * (s/max_s-1) * (m-min_I);
					break;

					default:
					cerr << "Unknown threshold type in ImageThresholder::surfaceNiblackImproved()\n";
					exit (1);
				}

				thsurf.fset(i+wxh,j,th);

				if (i==0) {
					// LEFT BORDER
					for (int i=0; i<=x_firstth; ++i)
					thsurf.fset(i,j,th);

					// LEFT-UPPER CORNER
					if (j==y_firstth)
					for (int u=0; u<y_firstth; ++u)
					for (int i=0; i<=x_firstth; ++i)
					thsurf.fset(i,u,th);

					// LEFT-LOWER CORNER
					if (j==y_lastth)
					for (int u=y_lastth+1; u<imgSmall.rows; ++u)
					for (int i=0; i<=x_firstth; ++i)
					thsurf.fset(i,u,th);
				}

				// UPPER BORDER
				if (j==y_firstth)
				for (int u=0; u<y_firstth; ++u)
				thsurf.fset(i+wxh,u,th);

				// LOWER BORDER
				if (j==y_lastth)
				for (int u=y_lastth+1; u<imgSmall.rows; ++u)
				thsurf.fset(i+wxh,u,th);
			}

			// RIGHT BORDER
			for (int i=x_lastth; i<imgSmall.cols; ++i)
			thsurf.fset(i,j,th);

			// RIGHT-UPPER CORNER
			if (j==y_firstth)
			for (int u=0; u<y_firstth; ++u)
			for (int i=x_lastth; i<imgSmall.cols; ++i)
			thsurf.fset(i,u,th);

			// RIGHT-LOWER CORNER
			if (j==y_lastth)
			for (int u=y_lastth+1; u<imgSmall.rows; ++u)
			for (int i=x_lastth; i<imgSmall.cols; ++i)
			thsurf.fset(i,u,th);
		}
		//cerr << "surface created" << endl;

		for (int y = 0; y < img.rows; ++y) {
			for (int x = 0; x < img.cols; ++x) {
				int y2= y/2;
				int x2= x/2;
				if (img.uget(x,y)>= thsurf.fget(x2,y2))
				{
					output.uset(x2,y2,255);
				}
				else
				{
					output.uset(x2,y2,0);
				}
			}
		}
	}
	//normalize to map from 1.5-3.0 to 1.5-15
	static double getDividor(double level)
	{
		if(level < 1)
			return 3.5;
		if(level < 1.5)
			return level;
		double a1 = 1.6;
		double b1 = 3;
		double a2 = 1;
		double b2 = 15;
		return (level-a1)/(b1-a1) * (b2-a2) + a2;
	}
	static void binarize(Mat& src, Mat& dst)
	{
		CV_Assert(src.channels() == 1);
		Mat tmp = src.clone();
//		double level = getAvgNoiseLevel(src,25,0.7,19,5);
//		double dividor = getDividor(level);
//		cout<<"level : " << level<<endl;
		double dividor = 3.5;
//		cout<<"dividor : " << dividor<<endl;
		int winx = tmp.cols / dividor;
		int winy = tmp.rows / dividor;
//		cout<<"winx : " <<winx << endl;
//		cout<<"winy : " <<winy << endl;
//		int winx = 19;
//		int winy = 19;
		double optK = 0.5;
		NiblackSauvolaWolfJolion(tmp, dst, WOLFJOLION, winx, winy, optK, 128);
//		cout<<"orig rows: " << src.rows<<endl;
//		cout<<"gen rows: " << dst.rows << endl;
//		cout<<dst(Rect(200, 200, 200, 200))<<endl;
	}
	static void binarizeSet(vector<Mat>& srcs, vector<Mat>& dsts)
	{
		CV_Assert(srcs.size() == dsts.size());
//		dsts.reserve(srcs.size());
//		dsts.clear();
		for(unsigned int i = 0; i < srcs.size(); i++)
		{
			binarize(srcs[i], dsts[i]);
		}
	}
	static void binarizeDir(string srcDir, string dstDir) {
		vector<string> files = FileUtil::getAllFiles(srcDir);
		for (unsigned int j = 0; j < files.size(); j++) {
			cout << srcDir + "/" + files[j] << endl;
			Mat src = imread(srcDir + "/" + files[j], IMREAD_GRAYSCALE);
			Mat dst;
			binarize(src, dst);
			imwrite(dstDir + "/" + files[j], dst);
		}
	}

};

#endif /* PREPROCESSING_SRC_BINARIZE_H_ */
