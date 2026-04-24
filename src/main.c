#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "../include/common.h"
#include "../include/logger.h"
#include "../include/collector.h"

SharedBuffer g_buffer;//全局共享资源

void handle_sigint(int sig){
	printf("\n[Main]: 捕捉到信号 %d, 正在安全关闭...\n", sig);
	g_buffer.stop_flag = true;
	pthread_cond_broadcast(&g_buffer.cond);//唤醒所有等待的线程
}

int main(){
	//1.初始化同步原语
	pthread_mutex_init(&g_buffer.mutex, NULL);
	pthread_cond_init(&g_buffer.cond, NULL);
	g_buffer.has_data = false;
	g_buffer.stop_flag = false;

	//2.注册信号（ctrl+c安全退出）
	signal(SIGINT, handle_sigint);

	pthread_t t1, t2;

	//3.创建线程
	if (pthread_create(&t1, NULL, collector_thread, &g_buffer) != 0)
	{
		perror("创建采集线程失败");
		return 1;
	}
	if (pthread_create(&t2, NULL, logger_thread, &g_buffer) != 0)
	{
		perror("创建日志线程失败");
		return 1;
	}

	printf("[Main]: 多线程日志采集系统已经启动...\n");

	//4. 等待线程结束
	pthread_join(t1, NULL);

	//告知日志线程数据采集已经结束
	pthread_mutex_lock(&g_buffer.mutex);
	g_buffer.stop_flag = true;
	pthread_cond_signal(&g_buffer.cond);
	pthread_mutex_unlock(&g_buffer.mutex);

	pthread_join(t2, NULL);

	//5.销毁资源
	pthread_mutex_destroy(&g_buffer.mutex);
	pthread_cond_destroy(&g_buffer.cond);

	printf("[Main]: 程序正常退出。\n");
	return 0;
	
	
}
