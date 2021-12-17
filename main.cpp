#include <QCoreApplication>
#include "spectralcamera.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <Windows.h>

extern SI_U8 *send_buf1;
extern SI_U8 *send_buf2;
extern QSemaphore emptybuff;                   //空缓冲区信号量
extern QSemaphore fullbuff;                    //正在处理的缓冲区信号量
extern bool is_first_buf;
extern SI_64 buf_size;

QSemaphore start_signal(0);  //开始采图并发送信号量
bool start_flag = false;

extern SpectralCamera* camera;

void protocal_analysis(char* data, int data_size)
{
    /* 通信协议： 起始位:|0XAA 0XBB| 触发模式:|0Xxx|(0X00->内触发，0X01->外触发) 曝光时间:|高位0Xxx 低位0Xxx|
        帧率:|高位0Xxx|低位0Xxx| 增益:|0Xxx|(只有四种：0x00,0x01,0x02,0x03) offset_x:|高位0Xxx 低位0Xxx|
        宽度:|高位0Xxx 低位0Xxx| 波段MROI: |b1高位 b1低位 b2高位 b2低位 ...| 结束位:|0XCC 0XDD|
        开始采图："start"  结束采图："stop"
    */

    //对数据进行解析
//  qDebug() << "go into1";
    if(data_size > 10)
    {
//            qDebug() << "go into2";
        if((uint8_t)data[0] == 0xAA && (uint8_t)data[1] == 0xBB &&
        (uint8_t)data[data_size-2] == 0xCC && (uint8_t)data[data_size-1] == 0xDD)  //配置协议
        {
            if((uint8_t)data[2] == 0x00)
            {
                qDebug() << "Internal";
            }
            else if((uint8_t)data[2] == 0x01)
            {
                qDebug() << "External";
            }
            uint16_t temp = (uint8_t)data[3] << 8 | (uint8_t)data[4];
            qDebug() << "exposure time:" <<temp;
            temp = 0;
            temp = (uint8_t)data[5] << 8 | (uint8_t)data[6];
            qDebug() << "frame rate:" <<temp;
            int gain = 0;
            if((uint8_t)data[7] == 0x00)
            {
                gain = 0;
            }
            else if((uint8_t)data[7] == 0x01)
            {
                gain = 1;
            }
            else if((uint8_t)data[7] == 0x02)
            {
                gain = 2;
            }
            qDebug() << "gain:" <<gain;
            temp = 0;
            temp = (uint8_t)data[8] << 8 | (uint8_t)data[9];
            qDebug() << "offset_x:" <<temp;
            temp = 0;
            temp = (uint8_t)data[10] << 8 | (uint8_t)data[11];
            qDebug() << "width:" <<temp;
            temp = 0;
            int i = 12;
            while(i != data_size - 2)
            {
                temp = 0;
                temp = (uint8_t)data[i] << 8 | (uint8_t)data[i+1];
                qDebug() << temp;
                i += 2;
            }
        }
    }
    else if(data_size == 5)
    {
        qDebug() << "aaaaaaaaaaaaaaaaaa";
        if(strcmp(data, "start") == 0)
        {
            start_signal.release();
            qDebug() << "start>>>>>>>>";
        }

    }
    else if(data_size == 4)
    {
        qDebug() << "bbbbbbbbbbbbb";
        if(strcmp(data, "stop") == 0)
        {
            start_flag = false;
            qDebug() << "stop>>>>>>>>";
        }
    }
    return;
}


int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);

    /* 初始化相机 */
    camera = new SpectralCamera();
    camera->init_camera();

    /* 启动服务端 */
    bool is_timeout;
    QTcpServer* server = new QTcpServer();
    QTcpSocket* client_socket = new QTcpSocket();
    server->listen(QHostAddress::Any, 13542);  //监听
    server->waitForNewConnection(30000, &is_timeout);
    if(is_timeout)
    {
        qDebug() << "++++++connect timeout!++++++";
        return -1;
    }
    client_socket = server->nextPendingConnection();
    qDebug() << "====== connect success! ======";

    /* 申请处理使用的缓冲区，确保为0 */
    int n = fullbuff.available();
    if(n > 0)
        fullbuff.acquire(n);

    qDebug() << ">>>>>>>";

    /* 开始采集 */
//    camera->start_acquisition();

    char rec_buf[128];
    int rec_size = 0;
//    bool flag = true;

//    FILE* fp = fopen("./roi", "wb");

//    第一层while，判断配置参数的发送和开始信号的发送
    while(1)
    {
        rec_size = 0;
        client_socket->waitForReadyRead(0);
        rec_size = client_socket->read(rec_buf, sizeof(rec_buf));
        if(rec_size > 0)
        {
            protocal_analysis(rec_buf, rec_size);
            memset(rec_buf, 0, sizeof(rec_buf));
        }

    //第二层while，监测停止信号的发送；图像数据传递客户端

        if(!start_signal.tryAcquire())
            continue;

        while(1)
        {
            rec_size = 0;
            client_socket->waitForReadyRead(0);
            rec_size = client_socket->read(rec_buf, sizeof(rec_buf));
            if(rec_size == 4 && !strcmp(rec_buf, "stop"))
            {
                camera->stop_acquisition();
                qDebug() << "stop acquisition!";
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
                client_socket->write((const char*)send_buf2, buf_size);
                client_socket->flush();
                emptybuff.release();
            }
            else
            {
                //发送缓冲区1
                client_socket->write((const char*)send_buf1, buf_size);
                client_socket->flush();
                emptybuff.release();
            }
        }
    }

    return 0;
}
