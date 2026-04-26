#include <stdio.h>
#include <time.h>
#include "../include/logger.h"

void* logger_thread(void*arg){
    SharedBuffer* shared = (SharedBuffer*)arg;

    //按日期生成文件名， 例如 log_20260420.txt
    char filename[64];
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    strftime(filename, sizeof(filename), "sensor_log_%Y%m%d.csv", tmp);

    while(1){
        pthread_mutex_lock(&shared->mutex);

        //使用while循环配合cond_wait 防止虚假唤醒
        while (!shared->has_data && !shared->stop_flag)
        {
            pthread_cond_wait(&shared->cond, &shared->mutex);
        }
        
        //如果收到停止信号且没有剩余，则退出
        if (shared->stop_flag  &&  !shared->has_data)
        {
            pthread_mutex_unlock(&shared->mutex);
            break;
        }
        
        //写入文件
        FILE* fp = fopen(filename, "a");
        if (fp)
        {
            // 格式：[时间],温度,湿度,转速
            fprintf(fp, "%s,%d,%d,%d\n", 
                    shared->data.timestamp, 
                    shared->data.temperature, 
                    shared->data.humidity, 
                    shared->data.speed);
            fclose(fp);
            printf("[Logger]: 数据已记录 -> T:%d H:%d S:%d\n", 
                    shared->data.temperature, 
                    shared->data.humidity, 
                    shared->data.speed);
        }
        
        shared->has_data = false;
        pthread_mutex_unlock(&shared->mutex);
    }
    return NULL;
}
