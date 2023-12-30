
#include "fs.h"

// 文件系统保存的文件名
char commands[15][10] = {"mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit",
                         "help"};

int main() {
    startsys();
    my_ls();
    return 0;
}