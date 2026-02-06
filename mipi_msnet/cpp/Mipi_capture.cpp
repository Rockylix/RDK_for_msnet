#include "Mipi_capture.h"

using std::cout;
using std::endl;

MipiCapture::MipiCapture(int width, int height,int infer_size,bool show):
        _width(width), _height(height), _infer_size(infer_size), _show(show)
{
    //打印信息
    cout << "MipiCapture: width=" << width << ", height=" << height 
         << ", infer_size=" << infer_size << ", show=" << show << endl;
    _camera1 = sp_init_vio_module();
    _camera2 = sp_init_vio_module();
}
MipiCapture::~MipiCapture()
{
    sp_vio_close(_camera1);
    sp_vio_close(_camera2);
    sp_release_vio_module(_camera1);
    sp_release_vio_module(_camera2);
    if(_show)
    {
        free(_show_ptr1);
        free(_show_ptr2);
    }
}

void MipiCapture::StartCapture()
{
   //camera sensor 参数设置（固化）
    sp_sensors_parameters parms;
    parms.fps = -1;
    parms.raw_height = 1080;
    parms.raw_width = 1920;

    if(_show)
    {
        _width_show = 320;
        _height_show = 240;
        int widths[] = {_width,_width_show};
        int heights[] = {_height,_height_show};
        // 打开摄像头 (管道0，camera0，一种分辨率图像）
        CHECK_SUCCESS(sp_open_camera_v2(_camera1, 0, -1, 2, &parms, widths, heights),"OPEN CAMERA1 FAILED");
        // 打开摄像头 (管道1，camera1，一种分辨率图像）
        CHECK_SUCCESS(sp_open_camera_v2(_camera2, 1, -1, 2, &parms, widths, heights),"OPEN CAMERA2 FAILED");
        
        _yuv_show_size = FRAME_BUFFER_SIZE(_width_show, _height_show);
        _show_ptr1 = (char *)malloc(_yuv_show_size);
        _show_ptr2 = (char *)malloc(_yuv_show_size);
    }
    else
    {
        int widths[] = {_width};
        int heights[] = {_height};
        // 打开摄像头 (管道0，camera0，一种分辨率图像）
        CHECK_SUCCESS(sp_open_camera_v2(_camera1, 0, -1, 1, &parms, widths, heights),"OPEN CAMERA1 FAILED");
        // 打开摄像头 (管道1，camera1，一种分辨率图像）
        CHECK_SUCCESS(sp_open_camera_v2(_camera2, 1, -1, 1, &parms, widths, heights),"OPEN CAMERA2 FAILED");
    }
    sleep(5); // 等待ISP 稳定
}

void MipiCapture::GetFrame(const hbSysMem& sys_mem)
{
    printf("Starting display... Press 'ESC' to exit.\n");
    int64_t timestamp_l, timestamp_r;
    bool synced = false;
    int max_retries = 5; // 如果时间对不上，最多尝试 5 次
    while(max_retries--)
    {
        //双线程
        std::thread t1([&]() {
            CHECK_SUCCESS(sp_vio_get_frame(_camera1, (char*)sys_mem.virAddr, _width, _height, 2000),"GET FRAME1 FAILED");
            // 记录获取完成时的时间戳（单位：毫秒）
            timestamp_l = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()).count();
        });
        std::thread t2([&]() {
            CHECK_SUCCESS(sp_vio_get_frame(_camera2, (char*)sys_mem.virAddr + _infer_size, _width, _height, 2000),"GET FRAME2 FAILED");
            // 记录获取完成时的时间戳（单位：毫秒）
            timestamp_r = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()).count();
        });
        t1.join();
        t2.join();
       
        // 计算两路图像返回的时间差
        int64_t diff = std::abs(timestamp_l - timestamp_r);

        if (diff <= 5) {
            // printf("Frames Synced: Delta = %ld ms\n", diff);
            synced = true;
            break; // 成功找到同步帧，跳出循环
        } else {
            printf("Warning: Frames out of sync! Delta = %ld ms. Retrying...\n", diff);
            // 这里不需要显式释放，下次调用 sp_vio_get_frame 会自动覆盖旧数据
        }
        if (!synced) {
            printf("Final Warning: Proceeding with potentially unsynced frames after retries.\n");
        }
    }
    if(_show) {
        //显示图片
        cv::Mat bgr_1, bgr_2;
        std::thread ts1([&]() {
            CHECK_SUCCESS(sp_vio_get_frame(_camera1, _show_ptr1, _width_show, _height_show, 2000),"GET FRAME1 SHOW FAILED");
            cv::Mat yuv_nv12_1(_height_show * 3 / 2, _width_show, CV_8UC1, _show_ptr1);
            cvtColor(yuv_nv12_1, bgr_1, cv::COLOR_YUV2BGR_NV12);
            imshow("Camera 2 (RDK)", bgr_2);
        });
        std::thread ts2([&]() {
            CHECK_SUCCESS(sp_vio_get_frame(_camera2, _show_ptr2, _width_show, _height_show, 2000),"GET FRAME2 SHOW FAILED");
            cv::Mat yuv_nv12_2(_height_show * 3 / 2, _width_show, CV_8UC1, _show_ptr2);
            cvtColor(yuv_nv12_2, bgr_2, cv::COLOR_YUV2BGR_NV12);
            imshow("Camera 1 (RDK)", bgr_1);
        });
        ts1.join();
        ts2.join();    
    }
}
