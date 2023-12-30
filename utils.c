#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include "fs.h"

// 打印一个16位值的二进制表示
void PrintBinary(uint16_t value) {
    for (int i = 15; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0) {
            printf(" "); // 在每4位之间添加一个空格
        }
    }
    printf("\n");
}

uint16_t GetCurrentTime(void) {
    time_t current_time;
    struct tm *time_info;
    uint16_t combined;

    // 获取当前时间
    time(&current_time);
    time_info = localtime(&current_time);

    // 将时间字段转换为BCD格式
    combined = 0;
    combined |= (time_info->tm_sec / 2) & 0x1F;
    combined |= (time_info->tm_min & 0x3F) << 5;
    combined |= (time_info->tm_hour & 0x1F) << 11;

#ifdef BEBUG_THIS
    printf("小时: %d\n", time_info->tm_hour);
    PrintBinary((time_info->tm_hour & 0x1F) << 11);

    printf("分钟: %d\n", time_info->tm_min);
    PrintBinary((time_info->tm_min & 0x3F) << 5);

    printf("秒: %d\n", time_info->tm_sec);
    PrintBinary(time_info->tm_sec / 2 & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

uint16_t GetCurrentDate(void) {
    time_t current_time;
    struct tm *time_info;
    uint16_t combined;

    // 获取当前时间
    time(&current_time);
    time_info = localtime(&current_time);

    // 将时间字段转换为BCD格式
    combined = 0;
    combined |= (time_info->tm_mday) & 0x1F;
    combined |= (time_info->tm_mon & 0xF) << 5;
    combined |= ((time_info->tm_year - 80) & 0x7F) << 9;

#ifdef BEBUG_THIS
    printf("年: %d\n", time_info->tm_year + 1900);
    PrintBinary((time_info->tm_year - 80) & 0x7F);

    printf("月: %d\n", time_info->tm_mon + 1);
    PrintBinary((time_info->tm_mon + 1) & 0xF);

    printf("日: %d\n", time_info->tm_mday);
    PrintBinary(time_info->tm_mday & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

void GetDateString(uint16_t combined, char *date_str) {
    struct tm time_info;

    // 解析日期部分
    time_info.tm_year = ((combined >> 9) & 0x7F) + 80;
    time_info.tm_mon = (combined >> 5) & 0xF;
    time_info.tm_mday = combined & 0x1F;

    // 将日期转换成字符串，添加0填充
    strftime(date_str, 11, "%Y-%m-%d", &time_info);
}

void GetTimeString(uint16_t combined, char *time_str) {
    struct tm time_info;

    // 解析时间部分
    time_info.tm_hour = (combined >> 11) & 0x1F;
    time_info.tm_min = (combined >> 5) & 0x3F;
    time_info.tm_sec = (combined & 0x1F) * 2;

    // 将时间转换成字符串，添加0填充
    strftime(time_str, 9, "%H:%M:%S", &time_info);
}

void AssignFCBtoUserOpen(fcb *ptr, user_open *uo, char *dir) {
    strcpy(uo->filename, ptr->filename);
    strcpy(uo->extname, ptr->extname);
    uo->attribute = ptr->attribute;
    uo->time = ptr->time;
    uo->date = ptr->date;
    uo->first = ptr->first;
    uo->length = ptr->length;

    strcpy(uo->dir, dir);
    uo->count = 0;
    uo->fcb_state = 0;
    uo->is_taken = 1;
}

void ExportUserOpenToFCB(fcb *ptr, user_open *uo) {
    strcpy(ptr->filename, uo->filename);
    strcpy(ptr->extname, uo->extname);
    ptr->attribute = uo->attribute;
    ptr->time = uo->time;
    ptr->date = uo->date;
    ptr->first = uo->first;
    ptr->length = uo->length;
    ptr->free = 1;
}

int ParseName(const char *name, unsigned short type, char *filename, char *extname) {
    int delimiter = -1;
    int filename_count = 0;
    int extname_count = 0;

    for (int i = 0; i < strlen(name); i++) {
        if (name[i] == '.') {
            // 以点开头或结尾是不合法的
            if (i == 0 || i == strlen(name) - 1) {
                printf("Error: Filename cannot start or end with dot.\n");
                return 0;
            } else if (delimiter != -1) { // 有两个点号是不合法的
                printf("Error: Filename cannot contain more than one dot.\n");
                return 0;
            } else {
                delimiter = i;
            }
        } else if (delimiter == -1) {
            if (filename_count < 8) {
                filename[filename_count++] = name[i];
            } else {
                printf("Error: Filename too long.\n");
                return 0;
            }
        } else {
            if (type == 0) {
                printf("Error: Directory name cannot contain dot.\n");
                return 0;
            } else {
                if (extname_count < 3) {
                    extname[extname_count++] = name[i];
                } else {
                    printf("Error: Extension name too long.\n");
                    return 0;
                }
            }
        }
    }

    return 1;
}

int IsNameExist(fcb *buf, int len, const char *name) {
    for (int i = 0; i < len; i++) {
        if (buf[i].free == 1) {
            if (buf[i].attribute == 0) {
                if (strcmp(buf[i].filename, name) == 0) {
                    printf("Error: Directory already exists.\n");
                    return 0;
                }
            } else if (buf[i].attribute == 1) {
                char fullname[11] = "";
                GetFullName(&buf[i], fullname);
                if (strcmp(fullname, name) == 0) {
                    printf("Error: Cannot create directory with the same name as a file.\n");
                    return 0;
                }
            } else {
                printf("Warning: Got invalid attribute %d of fcb at index %d, this should not happen.\n",
                       buf[i].attribute, i);
            }
        } else {
            printf("Warning: meet an empty fcb, this should not happen.\n");
        }
    }

    return 1;
}

void GetFullName(fcb *ptr, char *fullname) {
    int cur = 0;
    for (int i = 0; i < 8; i++) {
        if (ptr->filename[i] == '\0') {
            break;
        } else {
            fullname[cur++] = ptr->filename[i];
        }
    }
    fullname[cur] = '.';
    for (int i = 0; i < 3; i++) {
        if (ptr->extname[i] == '\0') {
            break;
        } else {
            fullname[cur++] = ptr->extname[i];
        }
    }

}

int ConcatPath(const char *path, const char *name, char *result) {
    if (strlen(path) + strlen(name) + 1 > MAX_PATH_LEN) {
        printf("Error: Path too long, try reduce dirname length\n");
        return 0;
    }
    snprintf(result, MAX_PATH_LEN + 1, "%s%s/", path, name);

    return 1;
}

int GetFreeOpenfile() {
    for (int i = 0; i < MAX_OPEN_FILE; i++) {
        if (open_file_list[i].is_taken == 0) {
            return i;
        }
    }
    return -1;
}

unsigned short GetFreeBlock() {
    // DISK_SIZE / BLOCK_SIZE 是虚拟磁盘实际拥有的盘块的数量
    for (int i = 5; i < DISK_SIZE / BLOCK_SIZE; i++) {
        if (fat1[i].id == BLOCK_FREE) {
            fat1[i].id = FILE_EOF;
            memcpy(fat2, fat1, 2 * BLOCK_SIZE);
            return i;
        }
    }
    return FILE_EOF;
}

int FindMyIndex(fcb *buf, int len, fcb *ptr) {
    char fullname[11] = "";
    GetFullName(ptr, fullname);

    for (int i = 0; i < len; i++) {
        char target_name[11] = "";
        GetFullName(&buf[i], target_name);
        if (strcmp(fullname, target_name) == 0){
            return i;
        }
    }

    return -1;
}