#pragma once
#include <string>
#include "opencv2/opencv.hpp"

// 跨平台动态库导出宏自动切换
#if defined(_WIN32)
    #if defined(myOcr_EXPORTS)
        #define MYOCR_API __declspec(dllexport)
    #else
        #define MYOCR_API __declspec(dllimport)
    #endif
#else
    #define MYOCR_API
#endif

namespace myOcr {
  struct Result{
    std::string text;
    float score;
    cv::Rect rect;
    cv::Mat img;
  };

  MYOCR_API bool init(
    const std::string& detName,const std::string& detDir,
    const std::string& recName,const std::string& recDir,
    int limitSideLen=256,const std::string& limitType="min",
    float thresh=0.3f, float boxThresh=0.4f, float unclipRatio=1.6f
  );
  MYOCR_API std::vector<Result> runDet(cv::Mat& mat);
  MYOCR_API void runRec(Result& result);

}