#include <stdio.h>
#include "fs.h"
#include "utils.c"

unsigned char *my_vhard;             // 指向虚拟磁盘的起始地址
user_open open_file_list[MAX_OPEN_FILE]; // 用户打开文件表数组
int cur_dir_fd;                         // 记录当前目录的文件描述符fd
char cur_dir_path[MAX_PATH_LEN + 1];                // 记录当前目录的目录名，包括目录的路径
unsigned char *start_pos;              // 记录虚拟磁盘上数据区开始位置
const char *FILENAME = "disk.bin";
fat *fat1;                          // FAT表1
fat *fat2;                          // FAT表2

void start_sys() {
    FILE *fp;
    my_vhard = (unsigned char *) malloc(DISK_SIZE);
    memset(my_vhard, 0, DISK_SIZE);

    fp = fopen(FILENAME, "rb");
    if (fp != NULL) // 有本地文件
    {
        fread(my_vhard, DISK_SIZE, 1, fp); // 将文件读入内存
        fclose(fp);
        if (memcmp(my_vhard, "FAT16", 5) != 0) // 文件头不正确，格式化内存
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
    block0 *b0 = (block0 *) my_vhard;
    strcpy(b0->magic_number, "FAT16");
    strcpy(b0->information, "My File System");
    b0->root = 5;
    b0->start_block = my_vhard + 5 * BLOCK_SIZE;

    // 初始化FAT表
    // FAT16 的每个表项占 2 字节，共有 1000 项
    // 每个磁盘块能存 1024 / 2 = 512 项，需要 2 块
    // FAT1 起点位于第 1 块，FAT2 起点位于第 3 块
    fat1 = (fat *) (my_vhard + 1 * BLOCK_SIZE);
    fat2 = (fat *) (my_vhard + 3 * BLOCK_SIZE);

    // 先全部填充 BLOCK_FREE，之后将前 6 项设置为已使用，这里包括了根目录分区
    memset(fat1, BLOCK_FREE, 2 * BLOCK_SIZE);
    memset(fat1, FILE_EOF, 6 * sizeof(fat));

    // 将 FAT1 复制到 FAT2
    memcpy(fat2, fat1, 2 * BLOCK_SIZE);
    // 记录数据区起始位置
    start_pos = my_vhard + 5 * BLOCK_SIZE;

    // 初始化根目录，创建两个默认的目录项
    // 获取当前时间备用
    uint16_t current_time = GetCurrentTime();
    uint16_t current_date = GetCurrentDate();

    // 根目录的起始地址为第 5 块，作为一个文件，是不定长度的
    fcb *cur = (fcb *) (my_vhard + 5 * BLOCK_SIZE);
    // 根目录的第一项为自己
    strcpy(cur->filename, ".");
    strcpy(cur->extname, "");
    cur->attribute = 0; // 0 是文件夹
    cur->time = current_time;
    cur->date = current_date;
    cur->first = 5; // 根目录的起始盘块号为 5
    // 初始的根目录只有两项，分别是自己和上级目录
    cur->length = 2 * sizeof(fcb);
    cur->free = 1; // 1 表示已分配

    cur++;
    // 根目录的上级目录为自己
    strcpy(cur->filename, "..");
    strcpy(cur->extname, "");
    cur->attribute = 0;
    cur->time = current_time;
    cur->date = current_date;
    cur->first = 5;
    cur->length = 2 * sizeof(fcb);
    cur->free = 1;

    // 初始化用户打开文件表
    // 1 - 9 项设置为未占用
    for (int i = 1; i < MAX_OPEN_FILE; i++) {
        open_file_list[i].is_taken = 0;
    }

    // 第 0 项打开根目录
    AssignFCBtoUserOpen((fcb *) (my_vhard + 5 * BLOCK_SIZE), &open_file_list[0], "/");
    cur_dir_fd = 0;
    strcpy(cur_dir_path, "/");

    printf("Format success!\n");
}

int do_write(int fd, char *text, int len, char wr_style) {
    switch (wr_style) {
        case 0: // 截断写，放弃文件的原有内容
            open_file_list[fd].length = 0;
            open_file_list[fd].count = 0;
            break;
        case 1: // 覆盖写，从当前指针位置开始覆盖
            // 不回退游标，不知道为什么网上的代码要回退
            break;
        case 2: // 追加写，在原文件的最后添加新的内容
            open_file_list[fd].count = (int) open_file_list[fd].length;
            // 不回退游标，添加 NULL 作为字符串结束不是这个函数的任务
            break;
        default:
            printf("Error: Invalid write style.\n");
            return -1;
    }

    // 寻找写入位置，此部分和 do_read 一致
    int remaining_offset = open_file_list[fd].count;
    unsigned short current_block_num = open_file_list[fd].first;
    while (remaining_offset >= BLOCK_SIZE) {
        remaining_offset -= BLOCK_SIZE;
        current_block_num = fat1[current_block_num].id;
        if (fat1[current_block_num].id == FILE_EOF) {
            printf("Error: Read out of file, this should not happen.\n");
            return -1;
        }
    }

    int left_len = len;
    while (left_len > 0) {
        int copy_len = BLOCK_SIZE - remaining_offset > left_len ? left_len : BLOCK_SIZE - remaining_offset;
        // 复制到 my_vhard
        memcpy(my_vhard + current_block_num * BLOCK_SIZE + remaining_offset, text, copy_len);
        left_len -= copy_len;
        // 增加 text 游标，指向下一次开始复制的位置
        text += copy_len;
        // 增加 fd 游标的偏移量
        open_file_list[fd].count += copy_len;
        // 增加文件长度
        if (open_file_list[fd].count > open_file_list[fd].length) {
            open_file_list[fd].length = open_file_list[fd].count;
        }
        // 文件修改了，同步 USER_OPEN 表项状态
        open_file_list[fd].fcb_state = 1;

        if (left_len > 0) {
            // 如果还有剩余长度，说明还需要继续写入
            // 读取下一个盘块，这样能复用之前申请的盘块
            current_block_num = fat1[current_block_num].id;
            // 如果当前盘块是最后一个盘块，说明已经读到文件末尾
            if (fat1[current_block_num].id == FILE_EOF) {
                // 申请新的盘块
                unsigned short new_block_num = GetFreeBlock();
                if (new_block_num == FILE_EOF) {
                    printf("Error: No free block.\n");
                    break;
                }
                // 将新的盘块号写入当前盘块的 FAT 表项
                fat1[current_block_num].id = new_block_num;
                // 更新当前盘块号
                current_block_num = new_block_num;
            }
            // 重置偏移量
            remaining_offset = 0;
        }
    }

    // 注意，如果是截断写，要回收多余的盘块：
    // 给当前盘块的 FAT 表项置为 FILE_EOF
    // 从当前盘块开始，将后续所有盘块的 FAT 表项置为 BLOCK_FREE
    if (wr_style == 0) {
        unsigned short cur = fat1[current_block_num].id;
        fat1[current_block_num].id = FILE_EOF;
        while (cur != FILE_EOF) {
            unsigned short next = fat1[cur].id;
            fat1[cur].id = BLOCK_FREE;
            cur = next;
        }
    }

    // 涉及fat表的修改，同步到 fat2
    memcpy(fat2, fat1, 2 * BLOCK_SIZE);

    // 返回的是实际写入的长度
    return len - left_len;
}

int do_read(int fd, int len, char *text) {
    // 文件描述符不能越界或者无效
    if (fd < 0 || fd >= MAX_OPEN_FILE) {
        printf("Error: Invalid file descriptor.\n");
        return -1;
    }
    if (open_file_list[fd].is_taken == 0) {
        printf("Error: File not open.\n");
        return -1;
    }

    // 当前文件的读写游标位置
    int remaining_offset = open_file_list[fd].count;
    // 首个盘块的编号
    unsigned short current_block_num = open_file_list[fd].first;

    // 持续查找当前offset所在的盘块，块内偏移量是0-1023，超过则进入下一个盘块
    while (remaining_offset >= BLOCK_SIZE) {
        // 减去一个盘块的偏移量
        remaining_offset -= BLOCK_SIZE;
        // 找到下一个盘块号
        current_block_num = fat1[current_block_num].id;

        // 如果当前盘块是最后一个盘块，说明已经读到文件末尾
        // 这个判断要放在迭代num之后，否则可能放过最后一个盘块
        if (fat1[current_block_num].id == FILE_EOF) {
            printf("Error: Read out of file, this should not happen.\n");
            return -1;
        }
    }

    int left_len = len;
    while (left_len > 0) {
        // 算出当前块需要复制的长度
        int copy_len = BLOCK_SIZE - remaining_offset > left_len ? left_len : BLOCK_SIZE - remaining_offset;
        // 复制到 text
        memcpy(text, my_vhard + current_block_num * BLOCK_SIZE + remaining_offset, copy_len);
        // 减少剩余长度
        left_len -= copy_len;
        // 增加 text 游标，指向下一个要写入的位置
        text += copy_len;
        // 增加 fd 游标的偏移量
        open_file_list[fd].count += copy_len;
        if (left_len > 0) {
            // 如果还有剩余长度，说明还需要继续读取
            // 读取下一个盘块
            current_block_num = fat1[current_block_num].id;
            // 如果当前盘块是最后一个盘块，说明已经读到文件末尾
            if (fat1[current_block_num].id == FILE_EOF) {
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
    open_file_list[cur_dir_fd].count = 0;
    fcb *buf = (fcb *) malloc((int) open_file_list[cur_dir_fd].length);
    do_read(cur_dir_fd, (int) open_file_list[cur_dir_fd].length, (char *) buf);

    // 末位是/0，所以长度要预留 +1
    char dateStr[11];
    char timeStr[9];
    printf("Current dir: %s\n", cur_dir_path);
    printf("name\t\tsize\ttype\tdate\t\ttime\t\t\n");

    for (int i = 0; i < open_file_list[cur_dir_fd].length / sizeof(fcb); i++) {
        if (buf[i].free == 1) {
            GetDateString(buf[i].date, dateStr);
            GetTimeString(buf[i].time, timeStr);
            if (buf[i].attribute == 0) {
                printf("%s\t%lu\t%s\t%s\t%s\n", buf[i].filename, buf[i].length, "dir", dateStr, timeStr);
            } else if (buf[i].attribute == 1) {
                printf("%s.%s\t%lu\t%s\t%s\t%s\n", buf[i].filename, buf[i].extname, buf[i].length, "file", dateStr,
                       timeStr);
            } else {
                printf("Warning: Got invalid attribute %d of fcb at index %d, this should not happen.\n",
                       buf[i].attribute, i);
                return;
            }
        } else {
            printf("Warning: meet an empty fcb, this should not happen.\n");
        }
    }
}

void my_mkdir(char *dirname) {
    char filename[MAX_FILENAME_LEN] = "";
    char extname[MAX_EXTNAME_LEN] = "";
    // 检测是否是合法的目录名
    if (!ParseName(dirname, 0, filename, extname)) {
        return;
    }

    open_file_list[cur_dir_fd].count = 0;
    fcb *buf = (fcb *) malloc(open_file_list[cur_dir_fd].length);
    do_read(cur_dir_fd, (int) open_file_list[cur_dir_fd].length, (char *) buf);
    // 检测是否重名
    if (!IsNameExist(buf, (int) (open_file_list[cur_dir_fd].length / sizeof(fcb)), dirname)) {
        return;
    }

    // 检测路径是否过长
    char path[MAX_PATH_LEN + 1] = "";
    if (!ConcatPath(cur_dir_path, dirname, path)) {
        return;
    }

    // 预先申请 open_file 资源
    int new_fd = GetFreeOpenfile();
    if (new_fd < 0){
        printf("Error: No spare open file table entry, close some file and retry.\n");
        return;
    }

    // 创建一个临时的 fcb，之后会被回收
    fcb tmp_fcb = {
        .filename = "",
        .extname = "",
        .attribute = 0,
        .time = GetCurrentTime(),
        .date = GetCurrentDate(),
        .first = GetFreeBlock(),
        .length = 0,
        .free = 1,
    };
    strcpy(tmp_fcb.filename, filename);
    strcpy(tmp_fcb.extname, "");

    // 预先保存 fcb 的 index
    int fcb_index = (int)(open_file_list[cur_dir_fd].length / sizeof(fcb));
    // 写入父目录
    do_write(cur_dir_fd, (char *)&tmp_fcb, sizeof(fcb), 2);

    // 打开新目录
    AssignFCBtoUserOpen(&tmp_fcb, &open_file_list[new_fd], path);

    // 复用 tmp_fcb 写入两个默认的目录项
    strcpy(tmp_fcb.filename, ".");
    tmp_fcb.length = 2 * sizeof(fcb);
    do_write(new_fd, (char *)&tmp_fcb, sizeof(fcb), 2);
    strcpy(tmp_fcb.filename, "..");
    tmp_fcb.time = open_file_list[cur_dir_fd].time;
    tmp_fcb.date = open_file_list[cur_dir_fd].date;
    tmp_fcb.first = open_file_list[cur_dir_fd].first;
    tmp_fcb.length = open_file_list[cur_dir_fd].length;
    do_write(new_fd, (char *)&tmp_fcb, sizeof(fcb), 2);

    // 关闭新目录文件
    if(!my_close(new_fd)){
        return;
    }
}

int my_close(int fd){
    if (fd < 0 || fd >= MAX_OPEN_FILE) {
        printf("Error: Invalid file descriptor.\n");
        return -1;
    }
    if (open_file_list[fd].is_taken == 0) {
        printf("Error: File not open.\n");
        return -1;
    }

    // 如果文件被修改过，同步到磁盘
    if (open_file_list[fd].fcb_state == 1) {
        // 创建临时的 fcb 作为写入参数
        fcb tmp_fcb;
        ExportUserOpenToFCB(&tmp_fcb, &open_file_list[fd]);

        open_file_list[cur_dir_fd].count = 0;
        fcb *buf = (fcb *) malloc(open_file_list[cur_dir_fd].length);
        do_read(cur_dir_fd, (int) open_file_list[cur_dir_fd].length, (char *) buf);

        int fcb_index = FindMyIndex(buf, (int) (open_file_list[cur_dir_fd].length / sizeof(fcb)), &tmp_fcb);
        if(fcb_index < 0){
            printf("Error: Cannot find the file fcb in parent dir.\n");
            return -1;
        }
        open_file_list[cur_dir_fd].count = fcb_index * (int)sizeof(fcb);
        do_write(cur_dir_fd, (char*)&tmp_fcb, sizeof(fcb), 1);
    }

    // 释放 open_file
    open_file_list[fd].is_taken = 0;
    return 0;
}