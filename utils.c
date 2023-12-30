#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include "fs.h"

#ifndef UTILS_H_
#define UTILS_H_

void PrintBinary(uint16_t value); // ��ӡһ��16λֵ�Ķ����Ʊ�ʾ
uint16_t GetCurrentDate(void);  // ��ȡ 7-4-5 λ������
uint16_t GetCurrentTime(void);  // ��ȡ 5-6-5 λ�ĵ�ǰʱ��
void GetDateString(uint16_t combined, char *date_str); // ���������ȡ������������ַ���
void GetTimeString(uint16_t combined, char *time_str); // ���������ȡ�������ʱ���ַ���

void AssignFCBtoUserOpen(fcb *ptr, user_open *uo, char *dir); // �� FCB �����ݿ����� USER_OPEN
void ExportUserOpenToFCB(fcb *ptr, user_open *uo); // ���������USER_OPEN �е� FCB ����
int ParseName(const char *name, unsigned short type, char *filename, char *extname); // ����һ���ļ�����ͬʱ�ж����Ƿ�Ϸ�
int IsDirNameExist(fcb *buf, int len, const char *name); // �ж��ļ��������Ƿ����
void GetFullName(fcb *ptr, char *fullname); // ��ȡ�ļ���ȫ��
int ConcatPath(const char *path, const char *name, char *result); // ƴ���ļ���·��
int GetFreeOpenfile();          // ��ȡһ�����е��ļ��򿪱���
unsigned short GetFreeBlock();  // ��ȡһ�����еĴ��̿�
int FindFCBIndexByName(fcb *buf, int len, char *filename, char *extname); // �����ļ�������FCB��Ŀ¼�е��±�
int FindFDIndexByPath(const char *path); // ����·�����Ҵ򿪵��ļ���
void GetParentDirName(char *dirname); // ��ȡ��Ŀ¼������
void GetParentDirPath(char *path); // ��ȡ��Ŀ¼��·��
#endif

void PrintBinary(uint16_t value) {
    for (int i = 15; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0) {
            printf(" "); // ��ÿ4λ֮�����һ���ո�
        }
    }
    printf("\n");
}

uint16_t GetCurrentDate(void) {
    time_t current_time;
    struct tm *time_info;
    uint16_t combined;

    // ��ȡ��ǰʱ��
    time(&current_time);
    time_info = localtime(&current_time);

    // ��ʱ���ֶ�ת��ΪBCD��ʽ
    combined = 0;
    combined |= (time_info->tm_mday) & 0x1F;
    combined |= (time_info->tm_mon & 0xF) << 5;
    combined |= ((time_info->tm_year - 80) & 0x7F) << 9;

#ifdef BEBUG_THIS
    printf("��: %d\n", time_info->tm_year + 1900);
    PrintBinary((time_info->tm_year - 80) & 0x7F);

    printf("��: %d\n", time_info->tm_mon + 1);
    PrintBinary((time_info->tm_mon + 1) & 0xF);

    printf("��: %d\n", time_info->tm_mday);
    PrintBinary(time_info->tm_mday & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

uint16_t GetCurrentTime(void) {
    time_t current_time;
    struct tm *time_info;
    uint16_t combined;

    // ��ȡ��ǰʱ��
    time(&current_time);
    time_info = localtime(&current_time);

    // ��ʱ���ֶ�ת��ΪBCD��ʽ
    combined = 0;
    combined |= (time_info->tm_sec / 2) & 0x1F;
    combined |= (time_info->tm_min & 0x3F) << 5;
    combined |= (time_info->tm_hour & 0x1F) << 11;

#ifdef BEBUG_THIS
    printf("Сʱ: %d\n", time_info->tm_hour);
    PrintBinary((time_info->tm_hour & 0x1F) << 11);

    printf("����: %d\n", time_info->tm_min);
    PrintBinary((time_info->tm_min & 0x3F) << 5);

    printf("��: %d\n", time_info->tm_sec);
    PrintBinary(time_info->tm_sec / 2 & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

void GetDateString(uint16_t combined, char *date_str) {
    struct tm time_info;

    // �������ڲ���
    time_info.tm_year = ((combined >> 9) & 0x7F) + 80;
    time_info.tm_mon = (combined >> 5) & 0xF;
    time_info.tm_mday = combined & 0x1F;

    // ������ת�����ַ��������0���
    strftime(date_str, 11, "%Y-%m-%d", &time_info);
}

void GetTimeString(uint16_t combined, char *time_str) {
    struct tm time_info;

    // ����ʱ�䲿��
    time_info.tm_hour = (combined >> 11) & 0x1F;
    time_info.tm_min = (combined >> 5) & 0x3F;
    time_info.tm_sec = (combined & 0x1F) * 2;

    // ��ʱ��ת�����ַ��������0���
    strftime(time_str, 9, "%H:%M:%S", &time_info);
}

void AssignFCBtoUserOpen(fcb *ptr, user_open *uo, char *dir) {
    strncpy(uo->filename, ptr->filename, MAX_FILENAME_LEN);
    strncpy(uo->extname, ptr->extname, MAX_EXTNAME_LEN);
    uo->attribute = ptr->attribute;
    uo->time = ptr->time;
    uo->date = ptr->date;
    uo->first = ptr->first;
    uo->length = ptr->length;

    strncpy(uo->dir, dir, MAX_PATH_LEN);
    uo->count = 0;
    uo->fcb_state = 0;
    uo->is_taken = 1;
}

void ExportUserOpenToFCB(fcb *ptr, user_open *uo) {
    strncpy(ptr->filename, uo->filename, MAX_FILENAME_LEN);
    strncpy(ptr->extname, uo->extname, MAX_EXTNAME_LEN);
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
            // �Ե㿪ͷ���β�ǲ��Ϸ���
            if (i == 0 || i == strlen(name) - 1) {
                printf("Error: Filename cannot start or end with dot.\n");
                return 0;
            } else if (delimiter != -1) { // ����������ǲ��Ϸ���
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

int IsDirNameExist(fcb *buf, int len, const char *name) {
    for (int i = 0; i < len; i++) {
        if (buf[i].free == 1) {
            if (buf[i].attribute == 0) {
                if (strncmp(buf[i].filename, name, MAX_FILENAME_LEN) == 0) {
                    printf("Error: Directory already exists.\n");
                    return 0;
                }
            } else if (buf[i].attribute == 1) {
                char fullname[MAX_FILENAME_LEN + MAX_EXTNAME_LEN + 1] = "";
                GetFullName(&buf[i], fullname);
                if (strncmp(fullname, name, sizeof(fullname)) == 0) {
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
    // DISK_SIZE / BLOCK_SIZE ���������ʵ��ӵ�е��̿������
    for (int i = 5; i < DISK_SIZE / BLOCK_SIZE; i++) {
        if (fat1[i].id == BLOCK_FREE) {
            fat1[i].id = FILE_EOF;
            memcpy(fat2, fat1, 2 * BLOCK_SIZE);
            return i;
        }
    }
    return FILE_EOF;
}

int FindFCBIndexByName(fcb *buf, int len, char *filename, char *extname) {
    char fullname[MAX_FILENAME_LEN + MAX_EXTNAME_LEN + 1] = "";
    char target_name[MAX_FILENAME_LEN + MAX_EXTNAME_LEN + 1] = "";
    snprintf(fullname, sizeof(fullname), "%s.%s", filename, extname);

    for (int i = 0; i < len; i++) {
        GetFullName(&buf[i], target_name);
        if (strncmp(fullname, target_name, sizeof(fullname)) == 0) {
            return i;
        }
    }
    return -1;
}

int FindFDIndexByPath(const char *path) {
    for (int i = 0; i < MAX_OPEN_FILE; i++) {
        if (open_file_list[i].is_taken == 1) {
            if (strncmp(open_file_list[i].dir, path, MAX_PATH_LEN) == 0) {
                return i;
            }
        }
    }
    return -1;
}

void GetParentDirName(char *dirname) {
    // ���һ���ַ��� '/'��ʹ�� - 1�ܿ�
    int len = strlen(cur_dir_path) - 1;
    int last_separator = -1;
    for (int i = 0; i < len ; i++) {
        if (cur_dir_path[i] == '/') {
            last_separator = i;
        }
    }
    strncpy(dirname, cur_dir_path + last_separator, len - last_separator);
}

void GetParentDirPath(char *path) {
    // ���һ���ַ��� '/'��ʹ�� - 1�ܿ�
    int len = (int)strlen(cur_dir_path) - 1;
    int last_separator = -1;
    for (int i = 0; i < len ; i++) {
        if (cur_dir_path[i] == '/') {
            last_separator = i;
        }
    }
    strncpy(path, cur_dir_path, last_separator + 1);
}