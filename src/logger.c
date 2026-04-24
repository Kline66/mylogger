#include <stdio.h>
#include <time.h>
#include "../include/logger.h"

void* logger_thread(void*arg){
    SharedBuffer* shared = (SharedBuffer*)arg;

    //按日期生成文件名， 例如 log_20260420.txt
    char filename[64];
    time_t t = time(NULL);
    strftime(filename, sizeof(filename), "log_%Y%m%d.txt", localtime(&t));

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
            fprintf(fp, "[&%s] Sensor ID: %d | Value: %.2f\n", 
            shared->data.timestamp, shared->data.sensor_id, shared->data.value);
            fclose(fp);
            printf("[Logger]: 数据已存入文件：%s\n", filename);
        }
        
        shared->has_data = false;
        pthread_mutex_unlock(&shared->mutex);
    }
    return NULL;
}
