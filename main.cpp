#include <QCoreApplication>
#include "spectralcamera.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <Windows.h>

extern float* send_buf1;
extern float* send_buf2;
extern QSemaphore emptybuff;                   //空缓冲区信号量
extern QSemaphore fullbuff;                    //正在处理的缓冲区信号量
extern bool is_first_buf;

QSemaphore start_signal(0);  //开始采图并发送信号量

extern SpectralCamera* camera;

camera_parameter CAMERA_PARAMETER;

void protocal_analysis(char* data, int data_size)
{
    /* 通信协议： 起始位:|0XAA 0XBB| 触发模式:|0Xxx|(0X00->内触发，0X01->外触发) 曝光时间:|高位0Xxx 低位0Xxx|
        帧率:|高位0Xxx|低位0Xxx| 增益:|0Xxx|(只有四种：0x00,0x01,0x02,0x03) offset_x:|高位0Xxx 低位0Xxx|
        宽度:|高位0Xxx 低位0Xxx| 波段MROI: |b1高位 b1低位 b2高位 b2低位 ...| 结束位:|0XCC 0XDD|
        开始采图："start"  结束采图："stop"
    */

    //对数据进行解析
//  qDebug() << "go into1";
    if(data_size > 1024)
    {
//            qDebug() << "go into2";
        if((uint8_t)data[0] == 0xAA && (uint8_t)data[1] == 0xBB &&
        (uint8_t)data[data_size-2] == 0xCC && (uint8_t)data[data_size-1] == 0xDD)  //配置协议
        {
            if((uint8_t)data[2] == 0x00)
            {
                memcpy(CAMERA_PARAMETER.mode, INTERNAL, sizeof(INTERNAL));
                qDebug() << "*** TriggerMode seted: <Internal> ***";
            }
            else if((uint8_t)data[2] == 0x01)
            {
                memcpy(CAMERA_PARAMETER.mode, EXTERNAL, sizeof(EXTERNAL));
                qDebug() << "*** TriggerMode seted: <External> ***";
            }
            uint16_t temp = (uint8_t)data[3] << 8 | (uint8_t)data[4];
            CAMERA_PARAMETER.exposure_time = temp;
            qDebug() << "*** Exposure Time seted: <" << temp << "> ***";
            temp = 0;
            temp = (uint8_t)data[5] << 8 | (uint8_t)data[6];
            CAMERA_PARAMETER.frame_rate = temp;
            qDebug() << "*** Frame Rate seted: <" << temp << "> ***";
            if((uint8_t)data[7] == 0x00)
            {
                CAMERA_PARAMETER.gain = 0;
                qDebug() << "*** Gain seted: <" << CAMERA_PARAMETER.gain << "> ***";
            }
            else if((uint8_t)data[7] == 0x01)
            {
                CAMERA_PARAMETER.gain = 1;
                qDebug() << "*** Gain seted: <" << CAMERA_PARAMETER.gain << "> ***";
            }
            else if((uint8_t)data[7] == 0x02)
            {
                CAMERA_PARAMETER.gain = 2;
                qDebug() << "*** Gain seted: <" << CAMERA_PARAMETER.gain << "> ***";
            }
            else if((uint8_t)data[7] == 0x03)
            {
                CAMERA_PARAMETER.gain = 3;
                qDebug() << "*** Gain seted: <" << CAMERA_PARAMETER.gain << "> ***";
            }
            temp = 0;
            temp = (uint8_t)data[8] << 8 | (uint8_t)data[9];
            CAMERA_PARAMETER.offset_x = temp;
            qDebug() << "*** Offset_x seted: <" << temp << "> ***";
            temp = 0;
            temp = (uint8_t)data[10] << 8 | (uint8_t)data[11];
            CAMERA_PARAMETER.image_width = temp;
            qDebug() << "*** Width seted: <" << temp << "> ***";
            temp = 0;
            int i = 12;
            while(i != data_size - 2)
            {
                temp = 0;
                temp = (uint8_t)data[i] << 8 | (uint8_t)data[i+1];
                qDebug() << "*** Band Selected: <" << temp << "> ***";
                CAMERA_PARAMETER.band = CAMERA_PARAMETER.band + std::to_wstring(temp)
                                        + L" " + std::to_wstring(1);
                if(i != data_size - 4)
                    CAMERA_PARAMETER.band += L";";
                i += 2;
                //赋值完记得清空
            }
            camera->load_param(&CAMERA_PARAMETER);
            camera->config_camera();
            camera->get_image_info();
        }
    }
    else if(data_size == 5)
    {
        if(strcmp(data, "start") == 0)
        {

            qDebug() << "***** start *****";

            /* 申请处理使用的缓冲区，确保为0 */
            int n = fullbuff.available();
            if(n > 0)
                fullbuff.acquire(n);
            n = emptybuff.available();
            if(n < 2)
                emptybuff.release(2 - n);
            start_signal.release();
<<<<<<< HEAD

=======
            camera->start_acquisition();
            qDebug() << "***** start *****";
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd
        }

    }
    else if(data_size == 4)
    {
        if(strcmp(data, "stop") == 0)
        {
            camera->stop_acquisition();
            qDebug() << "***** stop *****";
        }
    }
    return;
}


int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);

    char rec_buf[128];
    int rec_size = 0;

    /* 初始化相机 */
    camera = new SpectralCamera();
    camera->init_camera();


    /* 启动服务端 */
<<<<<<< HEAD

=======
#if 1
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd
    bool is_timeout;
    QTcpServer* server = new QTcpServer();
    QTcpSocket* client_socket = new QTcpSocket();
    server->listen(QHostAddress::Any, 13542);  //监听
    qDebug() << "====== waiting for connection ======";
<<<<<<< HEAD
    server->waitForNewConnection(30000, &is_timeout);
=======
    server->waitForNewConnection(10000, &is_timeout);
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd
    if(is_timeout)
    {
        qDebug() << "++++++connect timeout!++++++";
        return -1;
    }
    client_socket = server->nextPendingConnection();
    qDebug() << "====== connect success! ======";
    qDebug() << "====== waiting for signal... ======";
<<<<<<< HEAD

  //第一层while，判断配置参数的发送和开始信号的发送
=======


    /* 申请处理使用的缓冲区，确保为0 */
    int n = fullbuff.available();
    if(n > 0)
        fullbuff.acquire(n);
//  第一层while，判断配置参数的发送和开始信号的发送
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd
    while(1)
    {
        rec_size = 0;
        client_socket->waitForReadyRead(0);
        rec_size = client_socket->read(rec_buf, sizeof(rec_buf));
        if(rec_size > 0)
        {
<<<<<<< HEAD
            qDebug() << "RECEIVE SIGNAL";
            protocal_analysis(rec_buf, rec_size);
=======
//            qDebug() << "aaaaaaaaaaaaaa";
//            protocal_analysis(rec_buf, rec_size);
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd
            memset(rec_buf, 0, sizeof(rec_buf));
        }

    //第二层while，监测停止信号的发送；图像数据传递客户端
<<<<<<< HEAD
        if(!start_signal.tryAcquire())
            continue;
=======
//        if(!start_signal.tryAcquire())
//            continue;
#endif
        camera->start_acquisition();
        while(1)
        {
            ;
        }
>>>>>>> a956b5f7e075439800b2c39cfca05df65e86fbcd

        camera->start_acquisition();

        while(1)
        {
            rec_size = 0;
            client_socket->waitForReadyRead(0);
            rec_size = client_socket->read(rec_buf, sizeof(rec_buf));
            if(rec_size == 4 && !strcmp(rec_buf, "stop"))
            {
                camera->stop_acquisition();
                qDebug() << "***** stop *****";
                memset(rec_buf, 0, sizeof(rec_buf));
                break;
            }
            memset(rec_buf, 0, sizeof(rec_buf));

            if(!fullbuff.tryAcquire())
            {
               continue;
            }

            if(is_first_buf)
            {
                //发送缓冲区2
                client_socket->write((const char*)send_buf2, MUL_FRAME*sizeof(float));
                client_socket->flush();
                emptybuff.release();
                qDebug() << "send buf1";
            }
            else
            {
                //发送缓冲区1
                client_socket->write((const char*)send_buf1, MUL_FRAME*sizeof(float));
                client_socket->flush();
                emptybuff.release();
                qDebug() << "send buf2";
            }

        }

    }

    return 0;
}
