
#include "fs.h"

// 文件系统保存的文件名
char commands[15][10] = {"mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit",
                         "help"};

int main() {
    start_sys();
    my_ls();
    my_mkdir("hello");
//    my_mkdir(".dot");
//    my_mkdir("dot.");
//    my_mkdir("dot.dot");
//    my_mkdir("toolongfilename.toolongextname");
    my_ls();

    my_cd("hello");
    my_ls();
    my_mkdir("world");
    my_ls();
    my_cd("..");
    my_ls();
    return 0;
}