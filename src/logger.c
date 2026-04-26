#include <stdio.h>
#include <time.h>
#include <sys/stat.h>  // 用于 stat 函数
#include <unistd.h>    // 用于 rename 函数
#include "../include/logger.h"

// 定义 10MB 的常量
#define MAX_LOG_SIZE (10 * 1024 * 1024)

void* logger_thread(void*arg){
    SharedBuffer* shared = (SharedBuffer*)arg;

    //按日期生成文件名， 例如 log_20260420.csv
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
        
        // --- 日志切割逻辑 ---
        struct stat st;
        if (stat(filename, &st) == 0 && st.st_size >= MAX_LOG_SIZE) {
            char old_name[128];
            char date_buf[16];
            
            // 1. 先用 strftime 格式化日期部分 (例如 20260426)
            strftime(date_buf, sizeof(date_buf), "%Y%m%d", tmp);
            
            // 2. 再用 sprintf 拼接完整备份文件名 (加入时间戳防止同天内多次切割冲突)
            sprintf(old_name, "sensor_log_%s_%ld.csv.bak", date_buf, (long)time(NULL));
            
            if (rename(filename, old_name) == 0) {
                printf("[Logger]: 日志已满 10MB, 重命名为: %s\n", old_name);
            } else {
                perror("Rename failed");
            }
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
