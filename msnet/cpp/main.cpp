// D-Robotics *.bin 模型路径
// Path of D-Robotics *.bin model.
#define MODEL_PATH "../../model/DStereoV2.6_int8.bin"

// 推理使用的测试图片路径
// Path of the test image used for inference.
// #define TESR_IMG_PATH "../../../../../resource/assets/kite.jpg"
#define LEFT_PATH  "../../img/img_left.png"
#define RIGHT_PATH "../../img/img_right.png"


// 推理结果保存路径
// Path where the inference result will be saved
#define IMG_SAVE_PATH "../../img/result.png"

// C/C++ Standard Librarys
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unistd.h>

// Thrid Party Librarys
#include <opencv2/opencv.hpp>
#include <opencv2/dnn/dnn.hpp>


// RDK BPU libDNN API
#include "dnn/hb_dnn.h"
#include "dnn/hb_dnn_ext.h"
#include "dnn/plugin/hb_dnn_layer.h"
#include "dnn/plugin/hb_dnn_plugin.h"
#include "dnn/hb_sys.h"

//BPU_detect 类
#include "BPU_Detect.h"

//裁切
#define Preprocess_Type 0  // 1是Cut ， 0 是resize

int main()
{
    // 0. 打印相关版本信息，加载图片
    // std::cout << "OpenCV build details: " << cv::getBuildInformation() << std::endl;
    std::cout << "[OpenCV] Version: " << CV_VERSION << std::endl;
    setenv("NO_AT_BRIDGE", "1", 1); // 警用GUI警告

    //加载图片 BGR
    cv::Mat left_img = cv::imread(LEFT_PATH); 
    if (left_img.empty()) { 
        std::cout << "Failed to load image!" << std::endl;
        return -1;
    }

    cv::Mat right_img = cv::imread(RIGHT_PATH);
    if (right_img.empty()) { 
        std::cout << "Failed to load image!" << std::endl;
        return -1;
    }
    // 1. 初始化handle
    auto begin_time = std::chrono::system_clock::now();
    hbPackedDNNHandle_t dnn_handle = nullptr;
    BPU_detect msnet(dnn_handle,MODEL_PATH,IMG_SAVE_PATH,Preprocess_Type);
    // 2. 加载模型
    msnet.LoadModel();
    std::cout << "\033[31m Load D-Robotics Quantize model time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;

    //3. info
    begin_time = std::chrono::system_clock::now();
    msnet.GetModelInfo();
    std::cout << "\033[31m GetModelInfo and allocate mem time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;
    
    //4. preprocess
    begin_time = std::chrono::system_clock::now();
    msnet.Preprocess(left_img,right_img);
    std::cout << "\033[31m Preprocess time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;
    
    //5.Infer
    begin_time = std::chrono::system_clock::now();
    msnet.Inference();
    std::cout << "\033[31m Inference time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;    
    
    //6.postprocess
    begin_time = std::chrono::system_clock::now();
    msnet.Postprocess();
    std::cout << "\033[31m Postprocess time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;
    return 0;
}