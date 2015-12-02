/*
 * WapOcrApi.h
 *
 *  Created on: Nov 30, 2015
 *      Author: michael
 */

#ifndef SRC_API_WAP_OCR_API_H_
#define SRC_API_WAP_OCR_API_H_

#include <tesseract/baseapi.h>

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "../dto/ocr_result_dto.h"
class WapOcrApi {
public:
	WapOcrApi();
	static std::string recognitionToText(cv::Mat img, const std::string lang = "eng+jpn+chi_sim", const int cutLevel = 0, OcrDetailResult* result = NULL);
	static void release();
	virtual ~WapOcrApi();
private:
	static tesseract::TessBaseAPI *api;
	// merge and split
	static void optimize(OcrDetailResult*);
	static void mergeAndSplit(vector<ResultUnit>&);
	static void mapToLine(vector<ResultUnit> &symbols);
	static bool overlap(pair<int, int> a, pair<int, int> b);
	static void handle(vector<ResultUnit> &segment);
};

#endif /* SRC_API_WAP_OCR_API_H_ */
