#include <stdint.h>
#include <time.h>
#include <stdio.h>


// ��ӡһ��16λֵ�Ķ����Ʊ�ʾ
void PrintBinary(uint16_t value) {
    for (int i = 15; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0) {
            printf(" "); // ��ÿ4λ֮�����һ���ո�
        }
    }
    printf("\n");
}

uint16_t GetCurrentTime(void)
{
    time_t current_time;
    struct tm *timeinfo;
    uint16_t combined;
    
    // ��ȡ��ǰʱ��
    time(&current_time);
    timeinfo = localtime(&current_time);

    // ��ʱ���ֶ�ת��ΪBCD��ʽ
    combined = 0;
    combined |= (timeinfo->tm_sec / 2) & 0x1F;
    combined |= (timeinfo->tm_min & 0x3F) << 5;
    combined |= (timeinfo->tm_hour & 0x1F) << 11;

#ifdef BEBUG_THIS
    printf("Сʱ: %d\n", timeinfo->tm_hour);
    PrintBinary((timeinfo->tm_hour & 0x1F) << 11);

    printf("����: %d\n", timeinfo->tm_min);
    PrintBinary((timeinfo->tm_min & 0x3F) << 5);

    printf("��: %d\n", timeinfo->tm_sec);
    PrintBinary(timeinfo->tm_sec / 2 & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

uint16_t GetCurrentDate(void)
{
    time_t current_time;
    struct tm *timeinfo;
    uint16_t combined;
    
    // ��ȡ��ǰʱ��
    time(&current_time);
    timeinfo = localtime(&current_time);

    // ��ʱ���ֶ�ת��ΪBCD��ʽ
    combined = 0;
    combined |= (timeinfo->tm_mday) & 0x1F;
    combined |= (timeinfo->tm_mon & 0xF) << 5;
    combined |= ((timeinfo->tm_year - 80) & 0x7F) << 9;

#ifdef BEBUG_THIS
    printf("��: %d\n", timeinfo->tm_year + 1900);
    PrintBinary((timeinfo->tm_year - 80) & 0x7F);

    printf("��: %d\n", timeinfo->tm_mon + 1);
    PrintBinary((timeinfo->tm_mon + 1) & 0xF);

    printf("��: %d\n", timeinfo->tm_mday);
    PrintBinary(timeinfo->tm_mday & 0x1F);

    printf("combined:");
    PrintBinary(combined);
#endif

    return combined;
}

void AssignFCBtoUserOpen(fcb *fcb, useropen *useropen, char *dir)
{
    strcpy(useropen->filename, fcb->filename);
    strcpy(useropen->exname, fcb->exname);
    useropen->attribute = fcb->attribute;
    useropen->time = fcb->time;
    useropen->date = fcb->date;
    useropen->first = fcb->first;
    useropen->length = fcb->length;

    strcpy(useropen->dir, dir);
    useropen->count = 0;
    useropen->fcbstate = 0;
    useropen->topenfile = 1;
}

void GetDateString(uint16_t combined, char *dateStr) {
    struct tm timeinfo;

    // �������ڲ���
    timeinfo.tm_year = ((combined >> 9) & 0x7F) + 80;
    timeinfo.tm_mon = (combined >> 5) & 0xF;
    timeinfo.tm_mday = combined & 0x1F;

    // ������ת�����ַ��������0���
    strftime(dateStr, 11, "%Y-%m-%d", &timeinfo);
}

void GetTimeString(uint16_t combined, char *timeStr) {
    struct tm timeinfo;

    // ����ʱ�䲿��
    timeinfo.tm_hour = (combined >> 11) & 0x1F;
    timeinfo.tm_min = (combined >> 5) & 0x3F;
    timeinfo.tm_sec = (combined & 0x1F) * 2;

    // ��ʱ��ת�����ַ��������0���
    strftime(timeStr, 9, "%H:%M:%S", &timeinfo);
}