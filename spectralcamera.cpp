#include "spectralcamera.h"
//#define abcd


SI_U8 *send_buf1 = nullptr;
SI_U8 *send_buf2 = nullptr;
QSemaphore emptybuff(2);                   //空缓冲区信号量
QSemaphore fullbuff(0);                    //正在处理的缓冲区信号量
bool is_first_buf = true;
SI_64 buf_size = 0;

SpectralCamera* camera;

int onDataCallback(SI_U8* buffer, SI_64 frame_size, SI_64 frame_number, void* userdata);

SpectralCamera::SpectralCamera()
{

}

void SpectralCamera::load_param(camera_parameter *para)
{
    memcpy(mode, para->mode, sizeof(mode));  //模式设置
    memset(para->mode, 0, sizeof(para->mode));
    exposure_time = para->exposure_time / 1000.0;  //曝光时间设置
    frame_rate = para->frame_rate;   //帧率设置
    gain = para->gain;  //增益设置
    offset_x = para->offset_x;    //offset设置
    image_width = para->image_width;  //宽度设置
    memset(bands, 0, 128);
    std::copy(para->band.begin(), para->band.end(), bands);  //波段设置
    para->band.clear();

}

void SpectralCamera::enum_camera()
{
    SI_Load(LICENSE_PATH);
    SI_GetInt(SI_SYSTEM, L"DeviceCount", &device_count);
    if(is_show)
    {
        for (int n = 0; n < device_count; n++)
        {
            SI_GetEnumStringByIndex(SI_SYSTEM, L"DeviceName", n, szDeviceName, 4096);
            SI_GetEnumStringByIndex(SI_SYSTEM, L"DeviceDescription", n, szDeviceDescription, 4096);
            qDebug() << "Device: " << n;
            wprintf(L"\tName: %s\n", szDeviceName);
            wprintf(L"\tDescription: %s\n", szDeviceDescription);
        }
    }
}

void SpectralCamera::open_camera()
{
    SI_Open(CAMERA_INDEX, &device);
    SI_64 res = SI_Command(device, L"Initialize");
    if(res) qDebug() << "open camera failed!, error code: " << res;
}

void SpectralCamera::init_camera()
{
    enum_camera();
    if(device_count == 0)
    {
        qDebug() << "no device found";
        return;
    }
    open_camera();
    if(device == nullptr)
    {
        qDebug() << "====== init camera failed ======";
        return;
    }
//    config_camera();
    get_image_info();

    register_data_callback();
}

void SpectralCamera::config_camera()
{
    SI_64 res = 0;

    //设置触发模式
    res = SI_SetEnumIndexByString(device, L"Camera.Trigger.Mode", const_cast<SI_WC*>(mode));
    if(res) qDebug() << "set trigger mode failed, error code: " << res;

    //曝光时间
    res = SI_SetFloat(device, L"Camera.ExposureTime", exposure_time);
    if(res) qDebug() << "set exposure time failed, error code: " << res;

    //帧率
    res = SI_SetFloat(device, L"Camera.FrameRate", frame_rate);
    if(res) qDebug() << "set frame rate failed, error code: " << res;

    //设置采样波段
    res = SI_SetBool(device, L"Camera.MROI.Enable", false);
    res = SI_SetString(device, L"Camera.MROI.MultibandString", const_cast<SI_WC*>(bands));
    if(res) qDebug() << "set MROI failed, error code: " << res;
    res = SI_SetBool(device, L"Camera.MROI.Enable", true);

//    qDebug() << SI_SetEnumIndex(device, L"Camera.Gain.Digital", 0);
//    if(res) qDebug() << "set gain failed, error code: " << res;


}

//此函数计算bands_size和MROI数据大小，所以必须执行
void SpectralCamera::get_image_info()
{
    SI_64 height = 0;
    SI_64 width = 0;
    SI_64 bytes = 0;
    SI_64 bit_depth = 0;
    SI_64 frame_size;
    double frame_rate;
    double exposure_time;

    SI_GetInt(device, L"Camera.Image.Width", &width);
    SI_GetInt(device, L"Camera.Image.Height", &height);
    SI_GetInt(device, L"Camera.Image.SizeBytes", &bytes);
    SI_GetInt(device, L"Camera.BitDepth",&bit_depth);
    SI_GetInt(device, L"Camera.Image.SizeBytes", &frame_size);
    SI_GetFloat(device, L"Camera.ExposureTime", &exposure_time);
    SI_GetFloat(device, L"Camera.FrameRate", &frame_rate);

    bands_size = frame_size / RAW_WIDTH / 2;
    buf_size = image_width * bands_size * 2;

    send_buf1 = new SI_U8[buf_size];
    send_buf2 = new SI_U8[buf_size];

    /*int nFeatureCount = 0;
    SI_GetEnumCount(device, L"FeatureList", &nFeatureCount);
     Iterate through each feature
    for (int n = 0; n < nFeatureCount; n++)
    {

    SI_WC sFeature[255] = {0};
    SI_GetEnumStringByIndex(device, L"FeatureList", n, sFeature,255);
    wprintf(L"%s\n", sFeature);
    }*/

    std::wcout << "mode selected: " << mode << std::endl;
    qDebug() << "img_w:" << width << " | img_h:" << height;
    qDebug() << "img_sizebytes:" << bytes << " | nBitDepth:" << bit_depth << " | framesize:" << frame_size;
    qDebug() << "exposure time:" << exposure_time << "ms | frame rate:" << frame_rate;
    qDebug() << "bands size:" << bands_size << " | buf_size:" << buf_size;
    std::wcout << "band selected: " << bands << std::endl;
}

void SpectralCamera::register_data_callback()
{
    SI_64 res = 0;
    res = SI_RegisterDataCallback(device, onDataCallback, nullptr);
    if(res) qDebug() << "register data callback failed, error code: " << res;
}

void SpectralCamera::start_acquisition()
{
    SI_64 res = 0;
    res = SI_Command(device, L"Acquisition.Start");
    if(res) qDebug() << "start command failed, error code: " << res;
}

void SpectralCamera::stop_acquisition()
{
    SI_64 res = 0;
    SI_Command(device, L"Acquisition.Stop");
    if(res) qDebug() << "stop command failed, error code: " << res;
}

void SpectralCamera::close_camera()
{
    SI_64 res = 0;
    res = SI_Close(device);
    if(res) qDebug() << "close camera failed, error code: " << res;
    SI_Unload();
}

int onDataCallback(SI_U8* buffer, SI_64 frame_size, SI_64 frame_number, void* userdata)
{
    /* 打印帧号，帧大小 */
    if(frame_number % 1 == 0)
    {
        qDebug() << "frame: " << frame_number;
        qDebug() << "frame_size: " << frame_size;
    }

//    if(frame_number == 10)
//    {
//        FILE* fp = fopen("./abcd", "wb");
//        fwrite(buffer, 1, frame_size, fp);
//        fclose(fp);
//    }
#ifndef abcd
    if(!emptybuff.tryAcquire())      //申请空缓冲区
    {
        qDebug()<<"====== loss ======";
        return -1;
    }

    SI_U8* cut = new SI_U8[1024*camera->bandss.size()*2];
    SI_U8* cut_head = cut;
    for(auto i : camera->bands)
    {
        memcpy(cut, buffer + 2048*(317 + i), 2048);
        cut += 2048;
    }
    cut = cut_head;

    if(is_first_buf == true)
    {
        is_first_buf = false;
        SI_U8* send_buf1_head = send_buf1;
        memset(send_buf1, 0, buf_size);
        for(int i = 0; i<camera->bandss.size(); i++)
        {
            memcpy(send_buf1, cut + camera->offset_x * 2, camera->image_width * 2);
            send_buf1 = send_buf1 + camera->image_width * 2;
            cut = cut + RAW_WIDTH * 2;
        }
        send_buf1 = send_buf1_head;
        qDebug() <<"aaaaa";
        fullbuff.release();
    }
    else
    {
        is_first_buf = true;
        SI_U8* send_buf2_head = send_buf2;
        memset(send_buf2, 0, buf_size);
        for(int i = 0; i<camera->bandss.size(); i++)
        {
            memcpy(send_buf2, cut + camera->offset_x * 2, camera->image_width * 2);
            send_buf2 = send_buf2 + camera->image_width * 2;
            cut = cut + RAW_WIDTH * 2;
        }
        send_buf2 = send_buf2_head;
        qDebug() <<"bbbbb";
        fullbuff.release();
    }
    delete[] cut_head;
#endif

    return 0;
}

