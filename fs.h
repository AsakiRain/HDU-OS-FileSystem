#ifndef FS_H_
#define FS_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "time.h"

// 宏定义
#define BLOCKSIZE       1024        // 磁盘块大小
#define SIZE            1024000     // 虚拟磁盘空间大小
#define FEOF             65535       // FAT中的文件结束标志
#define FREE            0           // FAT中盘块空闲标志
#define MAXOPENFILE     10          // 最多同时打开文件个数
#define ROOTBLOCKNUM    2           // 根目录文件所占盘块数
#define MAXOPENFILE     10          // 最多同时打开文件个数

// 文件控制块 FCB
typedef struct FCB {          // 仿照FAT16设置的
    char filename[8];           // 文件名
    char exname[3];             // 文件扩展名
    unsigned char attribute;     // 文件属性字段。0表示目录文件；1表示数据文件
    unsigned short time;        // 文件创建时间
    unsigned short date;        // 文件创建日期
    unsigned short first;       // 文件起始盘块号
    unsigned long length;       // 文件长度（字节数）
    char free;                  // 表示目录项是否为空。若值为0，表示空；值为1，表示已分配
} fcb;

// 文件分配表 FAT
typedef struct FAT {
    unsigned short id;
} fat;

// 用户打开文件表 USEROPEM
typedef struct USEROPEN {
    char filename[8];           // 文件名
    char exname[3];             // 文件扩展名
    unsigned char attribute;    // 文件属性字段。0表示目录文件；1表示数据文件
    unsigned short time;        // 文件创建时间
    unsigned short date;        // 文件创建日期
    unsigned short first;       // 文件起始盘块号
    unsigned long length;      // 文件长度

    char dir[80];               // 相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开
    int count;                 // 读写指针在文件中的位置
    char fcbstate;              // 文件的FCB是否被修改，如果修改了置为1，否则为0
    char topenfile;             // 打开表项是否为空，若值为0，则表示为空；否则，表示已占用
} useropen;

// 引导块 BLOCK0
typedef struct BLOCK0 {
    char magic_number[8];
    char information[200];      // 描述信息
    unsigned short root;        // 根目录文件的起始盘块号
    unsigned char *startblock;  // 虚拟磁盘上数据区开始位置
} block0;


// 全局变量
extern unsigned char *myvhard;             // 指向虚拟磁盘的起始地址
extern useropen openfilelist[MAXOPENFILE]; // 用户打开文件表数组
extern int curdir;                         // 记录当前目录的文件描述符fd
extern char currentdir[80];                // 记录当前目录的目录名，包括目录的路径
extern unsigned char *startp;              // 记录虚拟磁盘上数据区开始位置
extern const char *FILENAME;
extern fat *fat1;                          // FAT表1
//extern unsigned char buffer[BLOCKSIZE];              // 读缓冲区

// 函数签名
int main();

void startsys();                                        // 进入文件系统
void my_format();                                       // 磁盘格式化函数
void my_cd(char *dirname);                              // 用于更改当前目录
void my_mkdir(char *dirname);                           // 创建子目录
void my_rmdir(char *dirname);                           // 删除子目录
void my_ls();                                           // 显示目录中的内容
int my_create(char *filename);                         // 创建文件
void my_rm(char *filename);                             // 删除文件
int my_open(char *filename);                           // 打开文件
int my_close(int fd);                                  // 关闭文件
int my_write(int fd);                                  // 写文件
int do_write(int fd, char *text, int len, char wstyle);// 实际写文件
int my_read(int fd);                                   // 读文件
int do_read(int fd, int len, char *text);              // 实际读文件
void my_exitsys();                                      // 退出文件系统

unsigned short getFreeBLOCK();                          // 获取一个空闲的磁盘块
int get_Free_Openfile();                               // 获取一个空闲的文件打开表项
int find_father_dir(int fd);                           // 寻找一个打开文件的父目录打开文件
void show_help();                                       // 获取命令帮助
void error(char *command);                              // 输出错误信息

#endif