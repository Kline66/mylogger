#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "../include/collector.h"

// 串口初始化函数 - 使用阻塞模式 + 读取超时
static int init_serial(const char* port) {
    // 打开串口设备，使用阻塞模式替代 O_NDELAY
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios options;
    // 获取当前串口参数
    if (tcgetattr(fd, &options) != 0) {
        perror("Failed to get serial attributes");
        close(fd);
        return -1;
    }
    
    // 设置波特率，与 STM32 保持一致
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // 8N1 模式 (8位数据位, 无校验, 1位停止位)
    options.c_cflag &= ~PARENB; // 无校验
    options.c_cflag &= ~CSTOPB; // 1位停止位
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8位数据位
    
    // 关闭规范模式 (关闭行缓冲)，允许立即读取
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // 设置原始输入模式
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // 关闭软件流控
    options.c_oflag &= ~OPOST; // 关闭输出后处理
    
    // 设置读取超时和最小字符数
    // VMIN = 0 表示不需要最少字符数
    // VTIME = 10 表示等待时间 1 秒 (单位: 0.1秒)
    options.c_cc[VMIN] = 0;   // 最少读取字符数
    options.c_cc[VTIME] = 10; // 读取超时时间 (单位: 0.1秒)
    
    // 应用新的串口参数
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("Failed to set serial attributes");
        close(fd);
        return -1;
    }
    
    printf("Serial port initialized successfully\n");
    return fd;
}

/**
 * @brief 采集器线程函数，用于从串口接收数据并解析存储到共享缓冲区
 * 
 * 该函数初始化串口连接，持续监听来自STM32的数据，解析温度、湿度和风扇转速信息，
 * 并将其存储到共享缓冲区中供其他线程使用。
 * 
 * @param arg 指向SharedBuffer结构体的指针，包含数据存储和同步相关字段
 * @return void* 线程退出状态，成功返回NULL
 */
void* collector_thread(void* arg) {
    SharedBuffer* shared = (SharedBuffer*)arg;
    int fd = init_serial("/dev/ttyUSB0"); // 串口设备节点

    if (fd < 0) {
        perror("Failed to open serial port");
        shared->stop_flag = true;
        return NULL;
    }

    char buf[128];
    printf("[Collector] Started, waiting for data...\n");
    
    // 主循环：持续监听串口数据直到收到停止信号
    while (!shared->stop_flag) {
        // 读取串口数据
        // 利用 init_serial 中设置的 VTIME 超时机制，若无数据则在超时后返回 0，避免 CPU 空转
        int n = read(fd, buf, sizeof(buf) - 1);
        
        if (n > 0) {
            // 确保字符串正确终止
            buf[n] = '\0';
            
            // 锁定互斥锁以保护共享数据
            pthread_mutex_lock(&shared->mutex);
            
            // 尝试解析标准格式的数据包: "T:%d,H:%d,S:%d"
            if (sscanf(buf, "T:%d,H:%d,S:%d", 
                       &shared->data.temperature, 
                       &shared->data.humidity, 
                       &shared->data.speed) == 3) {
                
                // 数据解析成功，更新元数据
                time_t now = time(NULL);
                strftime(shared->data.timestamp, 32, "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                shared->has_data = true;
                printf("[Collector] Data received -> T:%d H:%d S:%d Time:%s\n", 
                        shared->data.temperature, 
                        shared->data.humidity, 
                        shared->data.speed, 
                        shared->data.timestamp);
                
                // 通知其他等待数据的线程（如显示器或记录器线程）
                pthread_cond_signal(&shared->cond);
            } else {
                // 数据格式不匹配，记录调试信息
                printf("[Collector] Parse failed for: %s", buf);
            }
            
            // 解锁互斥锁
            pthread_mutex_unlock(&shared->mutex);
        } 
        else if (n < 0) {
            // 读取错误处理
            printf("Error reading from serial port: %s\n", strerror(errno));
        }
        // 若 n == 0，表示读取超时，循环将继续并检查 stop_flag
        
        // 短暂休眠，进一步降低 CPU 负载并确保线程调度友好
        usleep(10000); // 10ms
    }
    
    printf("[Collector] Stopping thread gracefully...\n");
    close(fd);
    return NULL;
}
