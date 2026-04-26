#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include "../include/collector.h"

// 串口初始化函数
static int init_serial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) return -1;

    struct termios options;
    tcgetattr(fd, &options);
    
    // 设置波特率，与 STM32 一致
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 模式
    options.c_cflag &= ~PARENB; // 无校验
    options.c_cflag &= ~CSTOPB; // 1位停止位
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8位数据位
    
    options.c_lflag |= ICANON;  // 开启规范模式，遇到 \n 才返回 read
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

void* collector_thread(void* arg) {
    SharedBuffer* shared = (SharedBuffer*)arg;
    int fd = init_serial("/dev/ttyUSB0"); // 设备节点

    if (fd < 0) {
        perror("Failed to open serial port");
        shared->stop_flag = true;
        return NULL;
    }

    char buf[128];
    while (!shared->stop_flag) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            
            pthread_mutex_lock(&shared->mutex);
            // 解析 STM32 发送的字符串: "T:25,H:60,S:80"
            if (sscanf(buf, "T:%d,H:%d,S:%d", 
                       &shared->data.temperature, 
                       &shared->data.humidity, 
                       &shared->data.speed) == 3) {
                
                time_t now = time(NULL);
                strftime(shared->data.timestamp, 32, "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                shared->has_data = true;
                printf("[Collector] Recv -> T:%d H:%d S:%d\n", 
                        shared->data.temperature, shared->data.humidity, shared->data.speed);
                
                pthread_cond_signal(&shared->cond);
            }
            pthread_mutex_unlock(&shared->mutex);
        }
    }
    close(fd);
    return NULL;
}

