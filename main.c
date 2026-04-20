#include <stdio.h>
#include <unistd.h>

//每隔一秒写一条日志
int main()
{
	int count = 0;
	while(1)
	{
	//打印时间和计数
	printf("[LOG]程序运行中... count = %d\n", count++);

	//休息一秒
	sleep(1);

	}	
	return 0;
}
