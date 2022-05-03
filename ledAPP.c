#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/*
*argc:应用程序参数个数
*argv[]:具体的参数内容，字符串形式
*./ledAPP <filename> data
*ep:    ./ledAPP /dev/dts_led 0     关闭LED
*ep:    ./ledAPP /dev/dts_led 1     打开LED
*/
int main(int argc,char *argv[])
{
    int ret = 0;
    int fd;
    char *filename;
    unsigned char databuf[1];
    if(argc!=3){
        printf("error usage!\r\n");
        return -1;       
    }
    filename = argv[1];
    /*打开LED驱动*/
    fd = open(filename,O_RDWR);
    if(fd<0){
        printf("file %s open failed!\r\n",filename);
        return -1;
    }
    databuf[0] = atoi(argv[2]);
    /*向设备驱动文件写数据*/
    ret = write(fd,databuf,sizeof((databuf)));
    if(ret<0){
        printf("LED control failed!\r\n");
        return -1;
    }
    /*关闭驱动文件*/
    ret = close(fd);
    if(ret<0)
    {
        printf("close file %s failed!\r\n",argv[1]);
        return -1;
    }
    return 0;
}

