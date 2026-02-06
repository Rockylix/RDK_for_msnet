#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>

// RDK SP库
#include <sp_vio.h>
#include <sp_sys.h>

// OpenCV 库
#include <opencv2/opencv.hpp>
#include "dnn/hb_dnn.h"
#include "dnn/hb_dnn_ext.h"
#include "dnn/plugin/hb_dnn_layer.h"
#include "dnn/plugin/hb_dnn_plugin.h"
#include "dnn/hb_sys.h"

//双线程
#include <thread>
#include <chrono> // 用于高精度时间戳
#include <cmath>  // 用于 abs


//返回检查
#define CHECK_SUCCESS(value,errmsg)                             \
    do                                                          \
    {                                                           \
        /*value can be call of function*/                       \
        auto ret_code = value;                                  \
        if (ret_code != 0)                                      \
        {                                                       \
            std::cout << errmsg << ", error code:" << ret_code; \
            exit(-1);                                          \
        }                                                       \
    } while (0);

class MipiCapture
{
public:
    MipiCapture(int width, int height,int infer_size,bool show);
    ~MipiCapture();

    //开启摄像头
    void StartCapture();
    //get one frame
    void GetFrame(const hbSysMem& sys_mem);

private:
    int _width;
    int _height;  
    void *_camera1;
    void *_camera2;
    int _infer_size;

    bool _show;//是否开启显示 
    int _width_show;
    int _height_show;
    int _yuv_show_size;
    char *_show_ptr1;
    char *_show_ptr2;

};