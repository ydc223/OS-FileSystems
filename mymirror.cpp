#include <sys/inotify.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h> /* printf() */
#include <iostream>
#include "utility.h"
#include "tree.hh"
#include "tree_util.hh"
#include <sys/types.h> 
#include <dirent.h>


// #include "tlpi_hdr.h"

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main(int argc, char *argv[])
{
    int inotifyFd, wd, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;
    char *backup;
    char root[100];
    char *source;
    DIR * dir_ptr;
    DIR * dest_dir_ptr;
    char* name;
    Node *nameFind;
    Node *find;
    struct Inode *inode1 = new Inode;
    struct Node node1 = {"", inode1};
    tree<Node>::pre_order_iterator treeIt;

    if (argc < 3 || strcmp(argv[1], "--help") == 0){
        printf("%s source target...\n", argv[0]);
        return 0;
    }

    source = argv[1];
    // memcpy(source, argv[1], strlen(argv[1])+1);

    // memcpy(root, source, strlen(source)+1);

    backup = argv[2];
    printf("Source %s, backup%s\n", source, backup);

    tree<Node> sourceTree;
    tree<Node> destinationTree;

    // creating dest tree
    dest_dir_ptr = opendir(backup);
    if(dest_dir_ptr == nullptr) {
        cout << "Backup does not exist. Creating...\n";
        mkdir(backup, ACCESSPERMS);
    }

    if (stat(backup, &(node1.inode->statbuf)) == -1) { 
        perror("Failed to get file status");
        exit(1);
    } 

    node1.name = backup;
    destinationTree.insert(destinationTree.begin(), node1);
    

    printf("------Printing destination tree-----------\n");
    printTree(destinationTree);

    // strcat(name, "/");
    dir_ptr = opendir(source);
    if(dir_ptr == nullptr) {
        perror("Open dir");
        exit(1);
    }
    // struct stat statbuf;

    if (stat(source, &(node1.inode->statbuf)) == -1) { 
        perror("Failed to get file status");
        exit(1);
    } else{
        node1.name = source;
        treeIt = sourceTree.insert(sourceTree.begin(), node1);
    }
    // printTree(sourceTree);
    
    makeDirectoryTree(source, source, &sourceTree, treeIt, treeIt);

    printf("------Printing source tree-----------\n");
    printTree(sourceTree);







    // Finding node by name and inode number
    find = findInodeByNum(1310723602, sourceTree);
    if(find != nullptr){
        printNode(*find);
    }
    string path = find->name;
    // printNode(*find);
    // printTree(sourceTree);
    nameFind = findNodeByName(path, sourceTree);
    // printTree(sourceTree);
    // printTree(sourceTree);

    if(nameFind != nullptr){
        // cout<< "NAME: " << nameFind->name<<endl;
        printNode(*nameFind);
    }

    syncFolders(&sourceTree, &destinationTree);




 //    inotifyFd = inotify_init();                 /* Create inotify instance */
 //    if (inotifyFd == -1)
 //        perror("inotify_init");

 //    /* For each command-line argument, add a watch for all events */

 //    for (j = 1; j < argc; j++) {
 //        wd = inotify_add_watch(inotifyFd, argv[j], IN_ALL_EVENTS);
 //        if (wd == -1)
 //            perror("inotify_add_watch");

 //        printf("Watching %s using wd %d\n", argv[j], wd);
 //    }

 //    for (;;) {                                  /* Read events forever */
 //        numRead = read(inotifyFd, buf, BUF_LEN);
 //        if (numRead == 0)
 //            perror("read() from inotify fd returned 0!");

 //        if (numRead == -1)
 //            perror("read");

 //        printf("Read %ld bytes from inotify fd\n", (long) numRead);

 //        /* Process all of the events in buffer returned by read() */

 //        for (p = buf; p < buf + numRead; ) {
 //            event = (struct inotify_event *) p;
 //            displayInotifyEvent(event);

 //            p += sizeof(struct inotify_event) + event->len;
 //        }
 //    }

	return 0;
}
