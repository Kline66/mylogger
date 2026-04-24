#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../include/collector.h"

void* collector_thread(void* arg){
    SharedBuffer* shared = (SharedBuffer*)arg;
    int count = 0;

    while (!shared->stop_flag)
    {
        //模拟数据准备（耗时一秒）
        sleep(1);

        //加锁并填充数据
        pthread_mutex_lock(&shared->mutex);

        shared->data.sensor_id = 101;
        shared->data.value = (float)(rand() % 1000) / 10.0f;

        //获取当前时间截
        time_t now = time(NULL);
        strftime(shared->data.timestamp, sizeof(shared->data.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        shared->has_data = true;
        printf("[Collector]: 采集到数据：%.2f\n", shared->data.value);

        //通知日志线程并解锁
        pthread_cond_signal(&shared->cond);
        pthread_mutex_unlock(&shared->mutex);

        if (++count >= 10)break;//模拟采集十次后自动结束
        
    }
    return NULL;
}
