#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <stdbool.h>

//模拟传感器数据结构
typedef struct 
{
    int sensor_id;
    float value;
    char timestamp[32];
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
