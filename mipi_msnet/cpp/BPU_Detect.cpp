#include "BPU_Detect.h"

using std::cout;
using std::endl;

#define official 1 // 1 使用官方后处理 0 使用自定义后处理
// 构造函数
BPU_detect::BPU_detect(hbPackedDNNHandle_t packed_dnn_handle,
    const std::string& model_path,const std::string& output_path,int type_of_pre):_packed_dnn_handle(packed_dnn_handle), _model_path(model_path),
    _output_path(output_path),_type_of_pre(type_of_pre)
    {
        std::cout << "BPU_detect " << std::endl;
        std::cout << "model_path: " << model_path << std::endl;
    }

BPU_detect::BPU_detect(hbPackedDNNHandle_t packed_dnn_handle,
    const std::string& model_path):_packed_dnn_handle(packed_dnn_handle), _model_path(model_path)
    {
        std::cout << "BPU_detect for MIPI " << std::endl;
        std::cout << "model_path: " << model_path << std::endl;
    }

// 析构函数
BPU_detect::~BPU_detect(){
    if (_output_tensor) {
        hbSysFreeMem(&_output_tensor[0].sysMem[0]); // 释放BPU物理内存
        // hbSysFreeMem(&_output_tensor[1].sysMem[0]); 
        delete[] _output_tensor; // 释放结构体数组
    }
    hbSysFreeMem(&_input_sys_mem); // 释放输入物理内存
    std::cout << "~BPU_detect " << std::endl;
}

// 加载模型函数
bool BPU_detect::LoadModel(){
    const char * model_name = _model_path.c_str();
    RDK_CHECK_SUCCESS(hbDNNInitializeFromFiles(&_packed_dnn_handle, &model_name, 1), "Load model failed");
    return true;
}

//Info 函数
bool checkinput(hbDNNTensorProperties &_input_properties,int &_input_h,int &_input_w)
{
    _input_h = _input_properties.validShape.dimensionSize[2];
    _input_w = _input_properties.validShape.dimensionSize[3];
    return _input_properties.validShape.numDimensions == 4;
}

bool BPU_detect::GetModelInfo()
{
    //获取模型列表，并检查有无错误
    const char **model_name_list;
    int model_count = 0;
    RDK_CHECK_SUCCESS(hbDNNGetModelNameList(&model_name_list,&model_count,_packed_dnn_handle),"Get model list failed");
    if(model_count > 1) {
        std::cout << "Model count: " << model_count << std::endl;
        std::cout << "Please check the model count!" << std::endl;
        return false;
    }
    _model_name = model_name_list[0];
    std::cout << "Model name : " << _model_name << std::endl;

    //解包，获得模型的单一句柄
    RDK_CHECK_SUCCESS(hbDNNGetModelHandle(&_dnn_handle,_packed_dnn_handle,_model_name),"Get single model handle failed");
    
    //输入检查 输入 NCHW
    int32_t input_count  = 0;
    //得到输入数量
    RDK_CHECK_SUCCESS(hbDNNGetInputCount(&input_count,_dnn_handle),"Get input failed");
    std::cout << "input_count = " << input_count <<std::endl;
    if (input_count != 2) {
        std::cerr << "Error: Expected 2 inputs, but model has " 
                  << input_count << " inputs!" << std::endl;
        return false;
    }
    
    for(int i=0; i<2;i++){
        RDK_CHECK_SUCCESS(hbDNNGetInputTensorProperties(&_input_properties[i],_dnn_handle,i),"Get input tensor properties failed");
        //初始化输入属性
        _input_tensors[i].properties = _input_properties[i]; // 必须赋值 properties!
        if(!checkinput(_input_properties[i],_input_h,_input_w)) 
        {
            std::cout << "input error" << std::endl;
            return false;
        }
        cout << "ValidShape = " <<_input_properties[i].tensorLayout <<endl;
        // 4 就是NV12
        cout << "validShape.numDimensions = " << _input_properties[i].validShape.numDimensions << endl;
        cout << "tensorType = " <<_input_properties[i].tensorType <<endl;
        cout << "alignedByteSize = " <<_input_properties[i].alignedByteSize <<endl;
        std::cout << "输入的尺寸为: (" << _input_properties[i].validShape.dimensionSize[0];
        std::cout << ", " << _input_properties[i].validShape.dimensionSize[1];
        std::cout << ", " << _input_properties[i].validShape.dimensionSize[2];
        std::cout << ", " << _input_properties[i].validShape.dimensionSize[3]<< ")" << std::endl;
    }
    //分配输入内存
    // 4. 提前分配输入物理内存 (两倍大小) NV 
    int single_img_size = _input_properties[0].alignedByteSize;
    RDK_CHECK_SUCCESS(hbSysAllocCachedMem(&_input_sys_mem, 2 * single_img_size), "...");
    // 更新 Tensor 结构体的地址和大小
    for(int i = 0; i < 2; i++) {
        _input_tensors[i].sysMem[0].virAddr = (uint8_t*)_input_sys_mem.virAddr + i * _input_properties[i].alignedByteSize;
        _input_tensors[i].sysMem[0].phyAddr = _input_sys_mem.phyAddr + i * _input_properties[i].alignedByteSize;
        _input_tensors[i].sysMem[0].memSize = _input_properties[i].alignedByteSize; 
        
        printf("[DEBUG TENSOR %d] VirAddr: %p, memSize: %d\n", i, _input_tensors[i].sysMem[0].virAddr, (int)_input_tensors[i].sysMem[0].memSize);
    }

    //输出检查 输出的 NHWC
    _output_count = 0;
    RDK_CHECK_SUCCESS(hbDNNGetOutputCount(&_output_count,_dnn_handle),"Get output count failed");
    std::cout << "output_count = " << _output_count <<std::endl;
    _output_tensor = new hbDNNTensor[_output_count];
    memset (_output_tensor, 0, sizeof(hbDNNTensor)*_output_count);
    for(int i=0; i< _output_count;++i)
    {
        RDK_CHECK_SUCCESS(hbDNNGetOutputTensorProperties(&_output_properties[i],_dnn_handle,i),"Get output tensor properties failed");//分配输出内存
        
        _output_tensor[i].properties = _output_properties[i];
        cout << "_output_properties.alignedByteSize = "<< _output_properties[i].alignedByteSize << endl;

        RDK_CHECK_SUCCESS(
        hbSysAllocCachedMem(&_output_tensor[i].sysMem[0], _output_properties[i].alignedByteSize),
        "Allocate output memory failed");
        std::cout << "输出的尺寸为: (" << _output_properties[i].validShape.dimensionSize[0];
        std::cout << ", " << _output_properties[i].validShape.dimensionSize[1];
        std::cout << ", " << _output_properties[i].validShape.dimensionSize[2];
        std::cout << ", " << _output_properties[i].validShape.dimensionSize[3]<< ")" << std::endl;
    }

    return true;
}

//yuv -> nv12函数
// 将两张图片转换并放入同一块连续内存中
// 修改函数签名，传入 tensor 数组 (通常是 input_tensor[0] 和 input_tensor[1])
// 将 BGR Mat 转换为 NV12 格式并写入目标物理地址
void BPU_detect::bgr2nv12_dual(cv::Mat& bgr_mat, hbSysMem& sys_mem, int index) {
    int h = _input_h;
    int w = _input_w;
    int y_size = h * w;
    // int uv_size = y_size / 2;

    // 获取当前 Tensor 的起始虚拟地址
    // 偏移量严格按照 alignedByteSize 计算，防止两张图地址重叠
    uint8_t* dst_ptr = (uint8_t*)sys_mem.virAddr + (index * _input_properties[index].alignedByteSize);

    // 1. 使用 OpenCV 转换为 YUV420P (I420) 格式: YYYYYYYY UU VV
    cv::Mat yuv_i420;
    cv::cvtColor(bgr_mat, yuv_i420, cv::COLOR_BGR2YUV_I420);
    uint8_t* src_ptr = yuv_i420.ptr<uint8_t>();

    // 2. 拷贝 Y 分量 (直接拷贝)
    memcpy(dst_ptr, src_ptr, y_size);

    // 3. 转换并拷贝 UV 分量 (I420 的 U,V 是分开的，NV12 需要交叉：UVUVUV)
    uint8_t* uv_dst = dst_ptr + y_size;
    uint8_t* u_src = src_ptr + y_size;
    uint8_t* v_src = u_src + (y_size / 4);

    for (int i = 0; i < y_size / 4; ++i) {
        *(uv_dst++) = *(u_src++);
        *(uv_dst++) = *(v_src++);
    }
}

//前处理函数

bool BPU_detect::Preprocess(const cv::Mat& input_img1, const cv::Mat& input_img2)
{
    if (input_img1.empty() || input_img2.empty()) {
        std::cerr << "Error: Input images are empty!" << std::endl;
        return false;
    }

    int target_h = _input_h; 
    int target_w = _input_w; 

    if (_type_of_pre) {
        /* --- Cut (裁剪/填充模式) --- */
        cout << "--------- Mode: Cut ---------" << endl;
        
        // 计算实际能裁剪的最大尺寸
        int cut_w = std::min(target_w, input_img1.cols);
        int cut_h = std::min(target_h, input_img1.rows);
        cv::Rect roi(550, 100, cut_w, cut_h);

        // 获取裁剪区域（注意：此时内存可能不连续）
        cv::Mat cropped_l = input_img1(roi);
        cv::Mat cropped_r = input_img2(roi);

        // 创建一个标准尺寸的“画布”，初始化为灰色(127)或黑色(0)
        // 这样即使原图很小，没填满的地方也是干净的填充色
        _preprocess_left = cv::Mat(target_h, target_w, CV_8UC3, cv::Scalar(127, 127, 127));
        _preprocess_right = cv::Mat(target_h, target_w, CV_8UC3, cv::Scalar(127, 127, 127));


        // 将裁剪的内容拷贝到画布的左上角（copyTo 会自动处理内存不连续问题）
        cropped_l.copyTo(_preprocess_left(cv::Rect(0, 0, cut_w, cut_h)));
        cropped_r.copyTo(_preprocess_right(cv::Rect(0, 0, cut_w, cut_h)));
    }
    else {
        /* --- Resize (LetterBox 模式) --- */
        cout << "--------- Mode: Resize ---------" << endl;
        int target_h = _input_h; // 模型要求的 H (如 192)
        int target_w = _input_w; // 模型要求的 W (如 384)

        // 记录原始尺寸，用于后处理还原
        _origin_h = input_img1.rows;
        _origin_w = input_img1.cols;

        // 1. 计算缩放比例 (模型尺寸 / 原始尺寸)
        _w_ratio = (float)target_w / _origin_w;
        _h_ratio = (float)target_h / _origin_h;

        // 2. 直接直接拉伸拉伸
        cout << "--------- Mode: Direct Resize ---------" << endl;
        cv::resize(input_img1, _preprocess_left, cv::Size(target_w, target_h));
        cv::resize(input_img2, _preprocess_right, cv::Size(target_w, target_h));
    }
    std::string left_save_path = "../../img/debug_left_.png";
    std::string right_save_path = "../../img/debug_right_.png";
    
    cv::imwrite(left_save_path, _preprocess_left);
    cv::imwrite(right_save_path, _preprocess_right);
    cout << "[DEBUG] Preprocessed images saved to ../../img/" << endl;


    bgr2nv12_dual(_preprocess_left, _input_sys_mem, 0);
    bgr2nv12_dual(_preprocess_right, _input_sys_mem, 1);

    int img_data_size = target_h * target_w * 3; 
    uint8_t *base_vir_addr = (uint8_t*)_input_sys_mem.virAddr;

    // 拷贝数据到 BPU 内存
    // 注意：即便上面处理过，这里也加一个安全检查
    if (!_preprocess_left.isContinuous() || (int)(_preprocess_left.total() * _preprocess_left.elemSize()) != img_data_size) {
        printf("[ERROR] Mat logic error! Size mismatch or not continuous.\n");
        return false;
    }
    // 打印首像素校验数据
    printf("[DEBUG] Left Start BGR: %02x %02x %02x\n", base_vir_addr[0], base_vir_addr[1], base_vir_addr[2]);

    // 刷新缓存同步到显存
    hbSysFlushMem(&_input_sys_mem, HB_SYS_MEM_CACHE_CLEAN);

    return true;
}

bool BPU_detect::Inference()
{
    //
    cout << "----------Flush-------"<< endl;
    hbSysFlushMem(&_input_sys_mem, HB_SYS_MEM_CACHE_CLEAN);

    cout << "----------Infer-------"<< endl;
    //初始化任务句柄
    _task_handle = nullptr;
    
    //创建推理参数
    hbDNNInferCtrlParam infer_ctrl_param;
    HB_DNN_INITIALIZE_INFER_CTRL_PARAM(&infer_ctrl_param);
    RDK_CHECK_SUCCESS(
        hbDNNInfer(&_task_handle, &_output_tensor, _input_tensors, _dnn_handle, &infer_ctrl_param),
        "Model inference failed");
    RDK_CHECK_SUCCESS(
        hbDNNWaitTaskDone(_task_handle, 0),
        "Wait task done failed");
    //释放任务
    RDK_CHECK_SUCCESS(hbDNNReleaseTask(_task_handle), "hbDNNReleaseTask failed");
    //刷新Cache
    for(int i = 0; i < _output_count; ++i)
        hbSysFlushMem(&(_output_tensor[i].sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);
    
    // std::cout << "=== Output Tensor Info ===" << std::endl;
    // //13 是 F32
    // for(int i = 0; i < _output_count; ++i){
    //     std::cout << "Tensor Type: " << _output_properties[i].tensorType << std::endl;
    //     std::cout << "Quantize Type: " << _output_properties[i].quantiType << std::endl;
    //     std::cout << "Layout: " << _output_properties[i].tensorLayout << std::endl;
    //     //std::cout << "scale:" << _output_properties.scale.scaleData[0] << std::endl;
        
    //     std::cout << "Valid Shape: [";
    //     for (int j = 0; j < _output_properties[i].validShape.numDimensions; j++) {
    //         std::cout << _output_properties[i].validShape.dimensionSize[j];
    //         if (j < _output_properties[i].validShape.numDimensions - 1) std::cout << ", ";
    //     }
    //     std::cout << "]" << std::endl;
        
    //     std::cout << "Aligned Shape: [";
    //     for (int j = 0; j < _output_properties[i].alignedShape.numDimensions; j++) {
    //         std::cout << _output_properties[i].alignedShape.dimensionSize[j];
    //         if (j < _output_properties[i].alignedShape.numDimensions - 1) std::cout << ", ";
    //     }
    //     std::cout << "]" << std::endl;
    // }

    return true;
}

void BPU_detect::PostProcessConvexSimple()
{
    // Placeholder for additional post-processing if needed
     // 1. 获取基础信息
    hbDNNTensor& t0 = _output_tensor[0];
    hbDNNTensor& t1 = _output_tensor[1];

    cout << "Output Tensor 0 Aligned Byte Size: " << t0.properties.alignedByteSize << endl;
    cout << "Output Tensor 1 Aligned Byte Size: " << t1.properties.alignedByteSize << endl;
    float scale0 = (t0.properties.quantiType == SCALE) ? t0.properties.scale.scaleData[0] : 1.0f;
    cout << "Output Tensor 0 quantiType: " << t0.properties.quantiType << endl;
    float scale1 = (t1.properties.quantiType == SCALE) ? t1.properties.scale.scaleData[0] : 1.0f;
    cout << "Output Tensor 1 quantiType: " << t1.properties.quantiType << endl;
    float final_scale = scale0 * scale1;

    // 2. 获取尺寸和比例信息
    int low_h = t0.properties.validShape.dimensionSize[2];  // 88
    int low_w = t0.properties.validShape.dimensionSize[3];  // 160
    int high_h = t1.properties.validShape.dimensionSize[2]; // 352
    int high_w = t1.properties.validShape.dimensionSize[3]; // 640
    int channels = 9;

    // 计算缩放倍数 (352/88 = 4)
    int upscale_h = high_h / low_h;
    int upscale_w = high_w / low_w;

    cv::Mat disp_map = cv::Mat::zeros(high_h, high_w, CV_32FC1);

    // 指针强转
    int32_t* disp_base = (int32_t*)t0.sysMem[0].virAddr;
    int16_t* spx_base = (int16_t*)t1.sysMem[0].virAddr;

     for (int y = 0; y < high_h; ++y) {
        int low_y = y / upscale_h; // 对应低分辨率图的 y 坐标
        
        for (int x = 0; x < high_w; ++x) {
            int low_x = x / upscale_w; // 对应低分辨率图的 x 坐标
            
            float pixel_sum = 0.0f;
            
            // 在 9 个通道上进行求和
            // 每一个高分辨率点 (y, x) 对应一个权重通道的值
            // 以及低分辨率图上对应块 (low_y, low_x) 的视差值
            for (int c = 0; c < channels; ++c) {
                // NCHW 内存布局索引
                int low_idx = c * (low_h * low_w) + low_y * low_w + low_x;
                int high_idx = c * (high_h * high_w) + y * high_w + x;
                
                float d_val = (float)disp_base[low_idx];
                float w_val = (float)spx_base[high_idx];
                
                pixel_sum += d_val * w_val;
            }
            
            // 存入结果
            disp_map.at<float>(y, x) = pixel_sum * final_scale;
        }
    }

    cv::Mat disp_origin_size;
    cv::resize(disp_map, disp_origin_size, cv::Size(_origin_w, _origin_h));
    float inv_w_ratio = 1.0f / _w_ratio;
    disp_origin_size *= inv_w_ratio;
    // 7. 保存结果
    SaveResult(disp_origin_size);
}

void BPU_detect::SaveResult(cv::Mat& disp) {
    cv::Mat disp_8u;
    double minVal, maxVal;
    cv::minMaxLoc(disp, &minVal, &maxVal);
    
    // 归一化到 0-255
    disp.convertTo(disp_8u, CV_8UC1, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    
    cv::Mat color_map;
    // 使用 COLORMAP_JET 伪彩色，冷色远，暖色近
    cv::applyColorMap(disp_8u, color_map, cv::COLORMAP_JET);
    
    cv::imwrite(_output_path, color_map);
    printf("Saved! Max Disp: %.2f\n", maxVal);
}

bool BPU_detect::Postprocess()
{
    cout << "--------PostProcess--------" << endl;
    #if official
        PostProcessConvexSimple();
    #else
        hbDNNTensor& t0 = _output_tensor[0];
        
        // MSNet 输出通常是 float32 (Type 13) 或 int32 (Type 14)
        float scale0 = (t0.properties.quantiType == SCALE) ? t0.properties.scale.scaleData[0] : 1.0f;
        cout << "quantiType" << t0.properties.quantiType << endl;
        
        int h = t0.properties.validShape.dimensionSize[1]; // 192
        int w = t0.properties.validShape.dimensionSize[2]; // 384
        
        cv::Mat disp_map(h, w, CV_32FC1);

        // 假设类型是 float32
        if (t0.properties.tensorType == HB_DNN_TENSOR_TYPE_F32) {
            float* raw_ptr = (float*)t0.sysMem[0].virAddr;
            memcpy(disp_map.data, raw_ptr, h * w * sizeof(float));
        } 
        // 假设类型是 int32 (量化模型)
        else if (t0.properties.tensorType == HB_DNN_TENSOR_TYPE_S32) {
            int32_t* raw_ptr = (int32_t*)t0.sysMem[0].virAddr;
            for(int i=0; i<h*w; ++i) {
                ((float*)disp_map.data)[i] = (float)raw_ptr[i] * scale0;
            }
        }
        cv::Mat disp_origin_size;
        cv::resize(disp_map, disp_origin_size, cv::Size(_origin_w, _origin_h));
        float inv_w_ratio = 1.0f / _w_ratio;
        disp_origin_size *= inv_w_ratio;

        SaveResult(disp_origin_size);
    #endif
    return true;
}

void BPU_detect::Mipi_PostProcess()
{
   //显示推理图像
   // Placeholder for additional post-processing if needed
    // 1. 获取基础信息
    hbDNNTensor& t0 = _output_tensor[0];
    hbDNNTensor& t1 = _output_tensor[1];

    float scale0 = (t0.properties.quantiType == SCALE) ? t0.properties.scale.scaleData[0] : 1.0f;
    float scale1 = (t1.properties.quantiType == SCALE) ? t1.properties.scale.scaleData[0] : 1.0f;
    float final_scale = scale0 * scale1;

    // 2. 获取尺寸和比例信息
    int low_h = t0.properties.validShape.dimensionSize[2];  // 88
    int low_w = t0.properties.validShape.dimensionSize[3];  // 160
    int high_h = t1.properties.validShape.dimensionSize[2]; // 352
    int high_w = t1.properties.validShape.dimensionSize[3]; // 640
    int channels = 9;

    // 计算缩放倍数 (352/88 = 4)
    int upscale_h = high_h / low_h;
    int upscale_w = high_w / low_w;

    cv::Mat disp_map = cv::Mat::zeros(high_h, high_w, CV_32FC1);

    // 指针强转
    int32_t* disp_base = (int32_t*)t0.sysMem[0].virAddr;
    int16_t* spx_base = (int16_t*)t1.sysMem[0].virAddr;

     for (int y = 0; y < high_h; ++y) {
        int low_y = y / upscale_h; // 对应低分辨率图的 y 坐标
        
        for (int x = 0; x < high_w; ++x) {
            int low_x = x / upscale_w; // 对应低分辨率图的 x 坐标
            
            float pixel_sum = 0.0f;
            
            // 在 9 个通道上进行求和
            // 每一个高分辨率点 (y, x) 对应一个权重通道的值
            // 以及低分辨率图上对应块 (low_y, low_x) 的视差值
            for (int c = 0; c < channels; ++c) {
                // NCHW 内存布局索引
                int low_idx = c * (low_h * low_w) + low_y * low_w + low_x;
                int high_idx = c * (high_h * high_w) + y * high_w + x;
                
                float d_val = (float)disp_base[low_idx];
                float w_val = (float)spx_base[high_idx];
                
                pixel_sum += d_val * w_val;
            }
            
            // 存入结果
            disp_map.at<float>(y, x) = pixel_sum * final_scale;
        }
    }
    //显示
    cv::Mat disp_8u;
    double minVal, maxVal;
    cv::minMaxLoc(disp_map, &minVal, &maxVal);
    // 归一化到 0-255
    disp_map.convertTo(disp_8u, CV_8UC1, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    
    cv::Mat color_map;
    // 使用 COLORMAP_JET 伪彩色，冷色远，暖色近
    cv::applyColorMap(disp_8u, color_map, cv::COLORMAP_JET);
    cout << "MIPI disp_map size: " << disp_map.size() << endl;
    cv::imshow("Disparity Map", color_map);
    cv::waitKey(1);
    // cv::imwrite("./mipi_disp.png", color_map);
    // cout << "Disparity map saved to ./mipi_disp.png" << endl;
}

