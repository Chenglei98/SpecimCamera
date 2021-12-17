#ifndef SPECTRALCAMERA_H
#define SPECTRALCAMERA_H

#include "SI_errors.h"
#include "SI_sensor.h"
#include "SI_types.h"
#include <QDebug>
#include <QTcpSocket>
#include <QThread>
#include <QSemaphore>

#define LICENSE_PATH L"C:/Program Files (x86)/Specim/SDKs/SpecSensor/SpecSensor SDK.lic"
#define CAMERA_INDEX 2    //相机索引,通过枚举选择FX10e with Pleora
#define BAND_SELECTED L"1000 3"  //光谱谱段选择，"x1 x2"表示:从x1开始到x1+x2共x2个波段

typedef struct
{
    double exposure_time;  //曝光时间
    double frame_rate;     //帧率
    SI_64 image_width;
    SI_WC bands[128];  //代表300~300+100波段范围，详情见SDK P251
    SI_WC mode[128];
    SI_64 gain;             //增益
}camera_parameter;


class SpectralCamera
{
    friend int onDataCallback(SI_U8* buffer, SI_64 frame_size, SI_64 frame_number, void* userdata);
public:
    SpectralCamera();
    void load_param();       //*************还没有写
    void enum_camera();      //枚举相机并打印相机信息
    void init_camera();
    void open_camera();      //初始化相机，打开相机
    void config_camera();    //配置相机参数
    void get_image_info();
    void register_data_callback();
    void start_acquisition();
    void stop_acquisition();
    void close_camera();
    SI_U8* set_roi(SI_U8* buffer);

    //static int onDataCallback(SI_U8* buffer, SI_64 frame_size, SI_64 frame_number, void* userdata);

private:
    SI_64 device_count = 0;
    SI_WC szDeviceName[4096];
    SI_WC szDeviceDescription[4096];
    SI_H device = 0;
    bool is_show = true;   //是否显示枚举信息


    /*相机配置参数*/


    double exposure_time = 0.6;  //曝光时间
    double frame_rate = 10;     //帧率
    SI_WC bands[128] = BAND_SELECTED;  //代表300~300+100波段范围，详情见SDK P251
    SI_WC mode[128] = L"Internal";
    SI_64 gain = 0;             //增益
    int offset_x = 0;
    SI_64 image_width = 1024;   //图像宽度
    int bands_size;


};



#endif // SPECTRALCAMERA_H
