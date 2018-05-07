#include <stdio.h>
#include <sys/inotify.h>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <string>
#include "utility.h"
#include "tree.hh"
#include <iostream>
#include <string.h>
#include "tree_util.hh"
#include <dirent.h>
/*4KB file copy buffer*/
#define BUFSIZE 4096
 
void cp_test(char* filepath1, char* filepath2){

    FILE *readFrom = open(filepath1, O_CREAT, 0777);
    FILE *writeTo = open(filepath2, O_CREAT, 0777);

    while (0 < (bytes = read(buffer, 1, sizeof(buffer), readFrom))){
        write(buffer, 1, bytes, writeTo);
    }
}


int main(){
    cp_test("NeverAsked.docx", "ForThis.docx");
    return 0;
}