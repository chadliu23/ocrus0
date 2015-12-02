/*
 * Recognition functionalities
 *
 * Copyright (C) 2015 Works Applications, all rights reserved
 *
 * Written by Chang Sun
 */

#include "ocrus/recognition.h"

#include <stdio.h>
#include <stdlib.h>

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <tesseract/ltrresultiterator.h>

#include "../dto/ocr_result_dto.h"

namespace ocrus {

void ocrPrintBoundingBox(const cv::Mat& src, tesseract::PageIteratorLevel level,
                         const std::string& lang) {
  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
  api->Init(NULL, lang.c_str());
  api->SetImage(reinterpret_cast<uchar*>(src.data), src.cols, src.rows, 1,
                src.cols);
  api->Recognize(NULL);

  tesseract::ResultIterator* ri = api->GetIterator();
  if (ri != 0) {
    do {
      const char* word = ri->GetUTF8Text(level);
      float conf = ri->Confidence(level);
      int x1, y1, x2, y2;
      ri->BoundingBox(level, &x1, &y1, &x2, &y2);       // here is the postion
      printf("word: '%s';  \tconf: %.2f; bounding_box: %d,%d,%d,%d;\n", word,
             conf, x1, y1, x2, y2);
      delete[] word;
    } while (ri->Next(level));
  }
  api->End();
  delete api;
}

void drawOcrResult(const cv::Mat &in_img, const OcrDetailResult &result,
                   cv::Mat *out_img) {
  const char *path_text = "/tmp/temp.txt";
  const char *path_temp_img = "/tmp/temp_img.png";
  const char *path_out_img = "/tmp/out_img.png";

  FILE *f_text = fopen(path_text, "w");
  auto res = result.getResult();
  for (auto r : res) {
    int x1 = r.bounding_box[0].x, y1 = r.bounding_box[0].y;
    int x2 = r.bounding_box[1].x, y2 = r.bounding_box[1].y;
    const char *word = r.content.c_str();
    float conf = r.confidence;

    fprintf(f_text, "word: '%s';  \tconf: %.2f; bounding_box: %d,%d,%d,%d;\n",
            word, conf, x1, y1, x2, y2);
  }
  fclose(f_text);

  cv::Mat temp_img = in_img.clone();
  if (temp_img.channels() == 1) {
    cv::cvtColor(temp_img, temp_img, cv::COLOR_GRAY2BGR);
  }
  cv::imwrite(path_temp_img, temp_img);

  char cmd[512];
  sprintf(cmd, "ocrus_draw_bbox.py %s %s %s", path_temp_img, path_text,
          path_out_img);
  system(cmd);

  *out_img = cv::imread(path_out_img);
}

}  // namespace ocrus
