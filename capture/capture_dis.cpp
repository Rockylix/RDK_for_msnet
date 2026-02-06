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

using namespace cv;

// // 保持你原来的 get_stride 不变，用来做raw图的
// double get_stride(int width, int bit) {
//     double temp = (width * bit / 8.0 / 16.0);
//     double frac, intpart;
//     frac = modf(temp, &intpart);
//     return (frac > 0) ? (ceil(temp) * 16) : (temp * 16);
// }

struct arguments {
    int width;
    int height;
    int bit;
    int count;
};

static struct argp_option options[] = {
    {"width", 'w', "W", 0, "sensor output width"},
    {"height", 'h', "H", 0, "sensor output height"},
    {"bit", 'b', "B", 0, "raw bit depth"},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    arguments *args = (arguments *)state->input;
    switch (key) {
        case 'w': args->width = atoi(arg); break;
        case 'h': args->height = atoi(arg); break;
        case 'b': args->bit = atoi(arg); break;
        case ARGP_KEY_END:
            if (state->argc < 4)
            {
                argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            }
        break;
        default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, 0, 0};

int main(int argc, char **argv) {
    arguments args = {0,0,0}; 
    argp_parse(&argp, argc, argv, 0, 0, &args);

    // VIO 初始化参数
    int ret;
    //camera sensor 参数设置
    sp_sensors_parameters parms;
    parms.fps = -1;
    parms.raw_height = 1080;
    parms.raw_width = 1920;

    void *camera_1 = sp_init_vio_module();
    void *camera_2 = sp_init_vio_module();

    int width_show = 160;
    int height_show = 120;

    int widths[] = {args.width,width_show};
    int heights[] = {args.height,height_show};
    
    // 打开摄像头 (管道0，camera0，一种分辨率图像）
    int ret_1 = sp_open_camera_v2(camera_1, 0, -1, 2, &parms, widths, heights);
    // 打开摄像头 (管道1，camera1，一种分辨率图像）
    int ret_2 = sp_open_camera_v2(camera_2, 1, -1, 2, &parms, widths, heights);
    if ( ret_1 != 0 || ret_2 != 0) {
        printf("Open camera failed\n");
        return -1;
    }

    sleep(5);

    // 分配缩放后的内存 (640x352)
    int scaled_yuv_size = FRAME_BUFFER_SIZE(args.width, args.height);
    int yuv_show_size = FRAME_BUFFER_SIZE(width_show, height_show);
    char *show_ptr1 = (char *)malloc(yuv_show_size);
    char *show_ptr2 = (char *)malloc(yuv_show_size);
    char *scaled_ptr1 = (char *)malloc(scaled_yuv_size);
    char *scaled_ptr2 = (char *)malloc(scaled_yuv_size);

    //FPS测量
    double fps = 0.0;
    int frame_count = 0;
    int64 start_time = getTickCount();
    char fps_text[50];

    printf("Starting display... Press 'q' to exit.\n");

    while (1) {
        // 1. 得到处理后图像
        int r1 = sp_vio_get_frame(camera_1, scaled_ptr1, args.width, args.height, 2000);
        int r2 = sp_vio_get_frame(camera_2, scaled_ptr2, args.width, args.height, 2000);
        if(!r1&&!r2) {
            //显示图片
            ret = sp_vio_get_frame(camera_1, show_ptr1, width_show, height_show, 2000);
            ret = sp_vio_get_frame(camera_2, show_ptr2, width_show, height_show, 2000);
            // 2. 使用 OpenCV 显示图像
            Mat yuv_nv12_1(height_show * 3 / 2, width_show, CV_8UC1, show_ptr1);
            Mat yuv_nv12_2(height_show * 3 / 2, width_show, CV_8UC1, show_ptr2);
            Mat bgr_1, bgr_2;
            cvtColor(yuv_nv12_1, bgr_1, COLOR_YUV2BGR_NV12);
            cvtColor(yuv_nv12_2, bgr_2, COLOR_YUV2BGR_NV12);
            putText(bgr_2, fps_text, Point(20, 40), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
            putText(bgr_1, fps_text, Point(20, 40), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
            imshow("Camera 2 (RDK)", bgr_2);
            imshow("Camera 1 (RDK)", bgr_1);
        }
        else{
            printf("Get frame failed! r1:%d r2:%d\n", r1, r2);
        }
         // 3. FPS 计算
        frame_count++;
        if(frame_count >= 30) {
            int64 current_time = getTickCount();
            fps = frame_count / ((current_time - start_time) / getTickFrequency());
            frame_count = 0;
            start_time = getTickCount();
            printf("FPS: %.2f\n", fps);
        // --- 将 FPS 绘制到画面上 ---
        sprintf(fps_text, "FPS: %.2f", fps);
        // 在左上角绘制文字 (图像, 文字, 位置, 字体, 大小, 颜色, 粗细)
        }

        // 6. 响应键盘，等待 1ms
        char key = (char)waitKey(1);
        if (key == 'q' || key == 27) break;
    }

    free(scaled_ptr1);
    free(scaled_ptr2);
    sp_vio_close(camera_1);
    sp_vio_close(camera_2);
    sp_release_vio_module(camera_1);
    sp_release_vio_module(camera_2);

    return 0;
}