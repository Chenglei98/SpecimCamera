#ifndef SPECTRALCAMERA_H
#define SPECTRALCAMERA_H

#include "SI_errors.h"
#include "SI_sensor.h"
#include "SI_types.h"
#include <QDebug>
#include <QTcpSocket>
#include <QThread>
#include <QSemaphore>
#include <iostream>
#include "opencv.hpp"

#define LICENSE_PATH L"C:/Program Files (x86)/Specim/SDKs/SpecSensor/SpecSensor SDK.lic"
#define CAMERA_INDEX 2    //相机索引,通过枚举选择FX10e with Pleora

#define REGION1_START (291 + 100)
#define REGION1_OFFSET 10
#define REGION2_START (291 + 200)
#define REGION2_OFFSET 10
#define REGION3_START (291 + 300)
#define REGION3_OFFSET 20

#define BAND_SELECTED L"REGION1_START REGION1_OFFSET;REGION2_START REGION2_OFFSET;REGION3_START REGION3_OFFSET"  //光谱谱段选择，"x1 x2"表示:从x1开始到x1+x2共x2个波段  基准点291

#define REALWIDTH 1024
#define REALHEIGHT 100   //拼接的帧数
#define BANDSIZE (REGION1_OFFSET + REGION2_OFFSET + REGION1_OFFSET)     // BANDSIZE 由 BAND_SELECTED 决定

#define CALIBRATION_FRAMES 100

#define SINGLE_FRAME (REALWIDTH * BANDSIZE)
#define MUL_FRAME (SINGLE_FRAME * REALHEIGHT)

//参数配置宏定义
#define EXTERNAL L"External"
#define INTERNAL L"Internal"



typedef struct
{
    SI_64 exposure_time;  //曝光时间 us
    double frame_rate;     //帧率
    SI_64 image_width;
    SI_WC bands[128];  //代表300~300+100波段范围，详情见SDK P251
    SI_WC mode[64];
    SI_64 gain;             //增益
    int offset_x;
    std::wstring band;
}camera_parameter;


class SpectralCamera
{
    friend int onDataCallback(SI_U8* buffer, SI_64 frame_size, SI_64 frame_number, void* userdata);
public:
    SpectralCamera();
    ~SpectralCamera();
    void load_param(camera_parameter* para);       //*************还没有写
    void enum_camera();      //枚举相机并打印相机信息
    void init_camera();
    void open_camera();      //初始化相机，打开相机
    void config_camera();    //配置相机参数
    void get_image_info();
    void register_data_callback();
    void start_acquisition();
    void stop_acquisition();
    void close_camera();

private:
    SI_64 device_count = 0;
    SI_WC szDeviceName[4096];
    SI_WC szDeviceDescription[4096];
    SI_H device = 0;
    bool is_show = true;   //是否显示枚举信息

    /* 相机配置参数  强烈建议通过软件设置或者配置文件设置*/
    double exposure_time = 0.6;  //曝光时间 ms
    double frame_rate = 10;     //帧率
    SI_WC bands[128] = BAND_SELECTED;  //代表300~300+100波段范围，详情见SDK P251
    SI_WC mode[32] = L"Internal";
    SI_64 gain = 1;             //增益
    int offset_x = 0;
    SI_64 image_width = 1024;   //图像宽度
    int bands_size;

    uint16_t* white_buf;  //校正白帧
    uint16_t* black_buf;  //校正黑帧
    uint32_t* temp_calibration_buf;

    bool capture_white_flag = false;
    bool capture_black_flag = false;
};



#endif // SPECTRALCAMERA_H
