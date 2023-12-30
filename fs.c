#include <stdio.h>
#include "fs.h"
#include "utils.c"

unsigned char *myvhard;             // 指向虚拟磁盘的起始地址
useropen openfilelist[MAXOPENFILE]; // 用户打开文件表数组
int curdir;                         // 记录当前目录的文件描述符fd
char currentdir[80];                // 记录当前目录的目录名，包括目录的路径
unsigned char *startp;              // 记录虚拟磁盘上数据区开始位置
const char *FILENAME = "disk.bin";
fat *fat1;                          // FAT表1
//unsigned char buffer[BLOCKSIZE];              // 读缓冲区

void startsys() {
    FILE *fp;
    myvhard = (unsigned char *) malloc(SIZE);
    memset(myvhard, 0, SIZE);

    fp = fopen(FILENAME, "rb");
    if (fp != NULL) // 有本地文件
    {
        fread(myvhard, SIZE, 1, fp); // 将文件读入内存
        fclose(fp);
        if (memcmp(myvhard, "FAT16", 5) != 0) // 文件头不正确，格式化内存
        {
            my_format();
        }
    } else  // 没有本地文件，直接格式化内存
    {
        my_format();
    }
}

void my_format() {
    // 初始化引导块
    block0 *b0 = (block0 *) myvhard;
    strcpy(b0->magic_number, "FAT16");
    strcpy(b0->information, "My File System");
    b0->root = 5;
    b0->startblock = myvhard + 5 * BLOCKSIZE;

    // 初始化FAT表
    // FAT16 的每个表项占 2 字节，共有 1000 项
    // 每个磁盘块能存 1024 / 2 = 512 项，需要 2 块
    // FAT1 起点位于第 1 块，FAT2 起点位于第 3 块
    fat *fat1 = (fat *) (myvhard + 1 * BLOCKSIZE);
    fat *fat2 = (fat *) (myvhard + 3 * BLOCKSIZE);

    // 先全部填充 FREE，之后将前 5 项设置为已使用
    memset(fat1, FREE, 2 * BLOCKSIZE);
    memset(fat1, FEOF, 5 * sizeof(fat));

    // 将 FAT1 复制到 FAT2
    memcpy(fat2, fat1, 2 * BLOCKSIZE);
    // 记录数据区起始位置
    startp = myvhard + 5 * BLOCKSIZE;

    // 初始化根目录，创建两个默认的目录项
    // 获取当前时间备用
    uint16_t current_time = GetCurrentTime();
    uint16_t current_date = GetCurrentDate();

    // 根目录的起始地址为第 5 块，作为一个文件，是不定长度的
    fcb *cur = (fcb *) (myvhard + 5 * BLOCKSIZE);

    // 根目录的第一项为自己
    strcpy(cur->filename, ".");
    strcpy(cur->exname, "");
    cur->attribute = 0; // 0 是文件夹
    cur->time = current_time;
    cur->date = current_date;
    cur->first = 5; // 根目录的起始盘块号为 5
    // 初始的根目录只有两项，分别是自己和上级目录
    cur->length = 2 * sizeof(fcb);
    cur->free = 1; // 1 表示已分配

    // 根目录的上级目录为自己
    cur++;
    strcpy(cur->filename, "..");
    strcpy(cur->exname, "");
    cur->attribute = 0;
    cur->time = current_time;
    cur->date = current_date;
    cur->first = 5;
    cur->length = 2 * sizeof(fcb);
    cur->free = 1;

    // 初始化用户打开文件表
    // 1 - 9 项设置为未占用
    for (int i = 1; i < MAXOPENFILE; i++) {
        openfilelist[i].topenfile = 0;
    }

    // 第 0 项打开根目录
    AssignFCBtoUserOpen((fcb *) (myvhard + 5 * BLOCKSIZE), &openfilelist[0], "/");
    curdir = 0;
    strcpy(currentdir, "/");

    printf("Format success!\n");
}

int do_write(int fd, char *text, int len, char wstyle) {
    unsigned short block_num = openfilelist[fd].first;
    //
}

int do_read(int fd, int len, char *text) {
    // 文件描述符不能越界或者无效
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("Error: Invalid file descriptor.\n");
        return -1;
    }
    if (openfilelist[fd].topenfile == 0) {
        printf("Error: File not open.\n");
        return -1;
    }

    // 准备缓冲区用于盘块读取
//    unsigned char* buf = (unsigned char*)malloc(BLOCKSIZE);
//    if(buf == NULL){
//        printf("Error: Cannot allocate %dB memory for read buffer.\n", BLOCKSIZE);
//        return -1;
//    }

    // 当前文件的读写游标位置
    int remaining_offset = openfilelist[fd].count;
    // 首个盘块的编号
    int current_block_num = openfilelist[fd].first;

    // 持续查找当前offset所在的盘块，块内偏移量是0-1023，超过则进入下一个盘块
    while (remaining_offset >= BLOCKSIZE) {
        // 减去一个盘块的偏移量
        remaining_offset -= BLOCKSIZE;
        // 找到下一个盘块号
        current_block_num = fat1[current_block_num].id;

        // 如果当前盘块是最后一个盘块，说明已经读到文件末尾
        // 这个判断要放在迭代num之后，否则可能放过最后一个盘块
        if (fat1[current_block_num].id == FEOF) {
            printf("Error: Read out of file.\n");
            return -1;
        }
    }

    int left_len = len;
    while (left_len > 0) {
        // 算出当前块需要复制的长度
        int copy_len = BLOCKSIZE - remaining_offset > left_len ? left_len : BLOCKSIZE - remaining_offset;
        // 复制到 text
        memcpy(text, myvhard + current_block_num * BLOCKSIZE + remaining_offset, copy_len);
        // 减少剩余长度
        left_len -= copy_len;
        // 增加 text 游标，指向下一个要写入的位置
        text += copy_len;
        // 增加 fd 游标的偏移量
        openfilelist[fd].count += copy_len;
        if (left_len > 0) {
            // 如果还有剩余长度，说明还需要继续读取
            // 读取下一个盘块
            current_block_num = fat1[current_block_num].id;
            // 如果当前盘块是最后一个盘块，说明已经读到文件末尾
            if (fat1[current_block_num].id == FEOF) {
                printf("Warning: Reaching end of file, requested reading length too long.\n");
                break;
            }
            // 重置偏移量
            remaining_offset = 0;
        }
    }

    // 返回的是实际读取的长度
    return len - left_len;
}

void my_ls() {
    // 重置游标为 0
    openfilelist[curdir].count = 0;
    fcb *buf = (fcb *) malloc(openfilelist[curdir].length);
    do_read(curdir, openfilelist[curdir].length, (char *) buf);

    // 末位是/0，所以长度要预留 +1
    char dateStr[11];
    char timeStr[9];
    printf("name\tsize\ttype\tdate\ttime\n");
    for (int i = 0; i < openfilelist[curdir].length / sizeof(fcb); i++) {
        if (buf[i].free == 1) {
            GetDateString(buf[i].date, dateStr);
            GetTimeString(buf[i].time, timeStr);
            if (buf[i].attribute == 0) {
                printf("%s\t%lu\t%s\t%s\t%s\n", buf[i].filename, buf[i].length, "dir", dateStr, timeStr);
            } else if (buf[i].attribute == 1) {
                printf("%s.%s\t%lu\t%s\t%s\t%s\n", buf[i].filename, buf[i].exname, buf[i].length, "file", dateStr,
                       timeStr);
            }
        } else {
            printf("Warning: meet an empty fcb, this should not happen.\n");
        }
    }
}