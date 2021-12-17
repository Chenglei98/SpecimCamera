#include "spectralcamera.h"

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
    config_camera();
    get_image_info();

    register_data_callback();
}

void SpectralCamera::config_camera()
{
    SI_64 res = 0;

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

    qDebug() << "fdsfsdf" << SI_SetEnumIndex(device, L"Camera.Gain.Digital", 2);


    //设置触发模式
//    res = SI_SetEnumIndexByString(device, L"Camera.Trigger.Mode", const_cast<SI_WC*>(mode));
//    if(res) qDebug() << "set trigger mode failed, error code: " << res;
}

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

    /*int nFeatureCount = 0;
    SI_GetEnumCount(device, L"FeatureList", &nFeatureCount);
     Iterate through each feature
    for (int n = 0; n < nFeatureCount; n++)
    {

    SI_WC sFeature[255] = {0};
    SI_GetEnumStringByIndex(device, L"FeatureList", n, sFeature,255);
    wprintf(L"%s\n", sFeature);
    }*/


    bands_size = frame_size / 2048;
    buf_size = image_width * bands_size * 2;

    send_buf1 = new SI_U8[buf_size];
    send_buf2 = new SI_U8[buf_size];

    qDebug() << "img_w:" << width << " img_h:" << height;
    qDebug() << "img_sizebytes:" << bytes << " nBitDepth:" << bit_depth << " framesize:" << frame_size;
    qDebug() << "exposure time:" << exposure_time << "ms frame rate:" << frame_rate;
    qDebug() << "bands size:" << bands_size << "buf_size:" << buf_size;
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
    if(!emptybuff.tryAcquire())      //申请空缓冲区
    {
        qDebug()<<"====== loss ======";
        return -1;
    }
    if(is_first_buf == true)
    {
        is_first_buf = false;
        SI_U8* temp = new SI_U8[buf_size];     //取ROI后的buff
        SI_U8* head = temp;
        memset(temp, 0, buf_size);
        for(int i = 0; i < camera->bands_size; i++)
        {
            memcpy(temp, buffer + camera->offset_x * 2, camera->image_width * 2);
            temp = temp + camera->image_width * 2;
            buffer = buffer + 1024 * 2;
        }
        memcpy(send_buf1, head, buf_size);
        qDebug() <<"aaaaa";
        delete[] head;
        fullbuff.release();
    }
    else
    {
        is_first_buf = true;
        SI_U8* temp = new SI_U8[buf_size];
        SI_U8* head = temp;
        memset(temp, 0, buf_size);
        for(int i = 0; i < camera->bands_size; i++)
        {
            memcpy(temp, buffer + camera->offset_x * 2, camera->image_width * 2);
            temp = temp + camera->image_width * 2;
            buffer = buffer + 1024 * 2;
        }
        memcpy(send_buf2, head, buf_size);
        delete[] head;
        fullbuff.release();
    }
//        if(is_first_buf == true)
//        {
//            is_first_buf = false;
//            memcpy(send_buf1, buffer, frame_size);
//            fullbuff.release();
//        }
//        else
//        {
//            is_first_buf = true;
//            memcpy(send_buf2, buffer, frame_size);
//            fullbuff.release();
//        }
    return 0;
}

