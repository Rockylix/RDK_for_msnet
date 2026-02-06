#include <iostream>
#include <string>

#include "dnn/hb_dnn.h"
#include "dnn/hb_dnn_ext.h"
#include "dnn/plugin/hb_dnn_layer.h"
#include "dnn/plugin/hb_dnn_plugin.h"
#include "dnn/hb_sys.h"

//cv 库
#include <opencv2/opencv.hpp>
#include <opencv2/dnn/dnn.hpp>



// RDK 错误检查宏定义
#define RDK_CHECK_SUCCESS(value, errmsg)                        \
    do                                                          \
    {                                                           \
        /*value can be call of function*/                       \
        auto ret_code = value;                                  \
        if (ret_code != 0)                                      \
        {                                                       \
            std::cout << errmsg << ", error code:" << ret_code; \
            return ret_code;                                    \
        }                                                       \
    } while (0);

class BPU_detect{
public:
    BPU_detect(hbPackedDNNHandle_t packed_dnn_handle, 
        const std::string& model_path,const std::string& output_path,
        int type_of_pre);
    ~BPU_detect();
// private:
    bool GetModelInfo();
    bool LoadModel();
    bool Preprocess(const cv::Mat& input_img1, const cv::Mat& input_img2);
    bool Inference();
    bool Postprocess();
    void bgr2nv12_dual(cv::Mat& bgr_mat,hbSysMem& sys_mem,int index);
    void PostProcessConvexSimple();
    void SaveResult(cv::Mat& disp);


private:
    hbPackedDNNHandle_t _packed_dnn_handle;
    hbDNNHandle_t _dnn_handle;
    std::string _model_path;
    std::string _output_path;
    const char *_model_name;
    int _type_of_pre;
    int32_t _output_count;

    int _input_h;// 输入高度
    int _input_w;// 输入宽度
    hbDNNTensorProperties _input_properties[2]; // 输入tensor属性
    hbDNNTensorProperties _output_properties[2]; // 输出tensor属性

        // 记录原始尺寸，用于后处理还原
    float _origin_h;
    float _origin_w;

    // 1. 计算缩放比例 (模型尺寸 / 原始尺寸)
    float _w_ratio;
    float _h_ratio;
    cv::Mat _preprocess_left;  //左图输入
    cv::Mat _preprocess_right;  //右图输入
  
    hbDNNTensor _input_tensors[2];   //输入张量
    hbDNNTensor* _output_tensor; //输出张量
    hbSysMem _input_sys_mem;  // 统一输入内存

    hbDNNTaskHandle_t _task_handle;          // 推理任务句柄
};