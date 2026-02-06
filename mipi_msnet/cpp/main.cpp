// D-Robotics *.bin 模型路径
// Path of D-Robotics *.bin model.
#define MODEL_PATH "../../model/DStereoV2.6_int8.bin"

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
#include "Mipi_capture.h"

using std::cout;
using std::endl;

struct arguments
{
    std::string model_path;
    bool show;
};

static struct argp_option options[] =
{
    {"model_path", 'm', "MODEL_PATH", 0, "Path of the D-Robotics *.bin model."},
    {"show", 's', 0, 0, "Whether to display the captured images from the cameras，1 is show and 0 is not."},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    arguments *args = (arguments *)state->input;
    switch (key)
    {
        case 'm': args->model_path = std::string(arg); break;
        case 's': args->show = true; break;
        case ARGP_KEY_END: //没有参数话 直接运行
            break;
        default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, 0, 0};

int main(int argc, char **argv)
{
    arguments args = {MODEL_PATH, false}; //默认
    argp_parse(&argp, argc, argv, 0, 0, &args);

    // 1. 初始化handle
    auto begin_time = std::chrono::system_clock::now();
    hbPackedDNNHandle_t dnn_handle = nullptr;
    BPU_detect msnet(dnn_handle,MODEL_PATH);

    // 2. 加载模型
    msnet.LoadModel();
    std::cout << "\033[31m Load D-Robotics Quantize model time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;
    begin_time = std::chrono::system_clock::now();
    // 3. infor
    msnet.GetModelInfo();
    std::cout << "\033[31m Get infor time = " << std::fixed << std::setprecision(2) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin_time).count() / 1000.0 << " ms\033[0m" << std::endl;
    
    //4. 初始化mipi进行采集
    MipiCapture mipi(msnet.Get_input_w(), msnet.Get_input_h(),
        msnet.Get_input_align_size(), args.show);
    mipi.StartCapture();
    //5. 拍照推理后处理  + 显示
    //fps
    int frame_count = 0;
    double fps = 0.0;
    int64 start_time = cv::getTickCount();
    while(1)
    {
        mipi.GetFrame(msnet.Get_input_sys_mem());
        msnet.Inference();
        msnet.Mipi_PostProcess();

        // 3. FPS 计算
        cout<<"--- FPS Calculation ---"<<endl;
        cout << "frame_count before increment: " << frame_count << endl;
        frame_count++;
        if(frame_count >= 30) {
            int64 current_time = cv::getTickCount();
            fps = frame_count / ((current_time - start_time) / cv::getTickFrequency());
            frame_count = 0;
            start_time = cv::getTickCount();
            cout<<"FPS: "<<fps<<endl;
        }
        if(cv::waitKey(1) == 27) // 按 'ESC' 键退出
            break;
    }

    // mipi.GetFrame(msnet.Get_input_sys_mem());
    // msnet.Inference();
    // msnet.Mipi_PostProcess();
}