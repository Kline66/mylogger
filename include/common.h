#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <stdbool.h>

//接收stm32发送的数据
typedef struct {
    int temperature;    // 温度
    int humidity;       // 湿度
    int speed;          // 电机转速
    char timestamp[32]; // 采集时间
} DataPayload;

//共享缓冲区结构（单元素缓冲）
typedef struct 
{
    DataPayload data;
    bool has_data; //标识是否有新数据产生
    bool stop_flag; //线程退出标志
    pthread_mutex_t mutex; //互斥锁
    pthread_cond_t cond; //条件变量，用于线程同步
} SharedBuffer;



#endif
