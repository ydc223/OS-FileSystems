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

#define MAX_INODES 1024
// #include "tlpi_hdr.h"

/* Display information from inotify_event structure and call the appropriate handler*/
static void handleInotifyEvents(struct inotify_event *i, map<int, tree<Node>::pre_order_iterator>, tree<Node>* destinationTree,  tree<Node>* sourceTree, char* sourceRoot, char* backupRoot, struct MOVEEventInfo *mInfo);



#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main(int argc, char *argv[])
{
    int inotifyFd, wd, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;
    struct inotify_event *event2;
    char *backup;
    char root[100];
    char *source;
    DIR * dir_ptr;
    DIR * dest_dir_ptr;
    char* name;
    tree<Node>::pre_order_iterator nameFind;
    Node *find;
    struct Inode *inode1 = new Inode;
    struct Node node1 = {"", inode1};
    struct Inode *inode2 = new Inode;
    struct Node node2 = {"", inode2};
    tree<Node>::pre_order_iterator treeIt;
    map<int, tree<Node>::pre_order_iterator>  watchDescriptors;
    MOVEEventInfo mInfo;



    if (argc < 3 || strcmp(argv[1], "--help") == 0){
        printf("%s source target...\n", argv[0]);
        return 0;
    }

    source = argv[1];
    // memcpy(source, argv[1], strlen(argv[1])+1);

    // memcpy(root, source, strlen(source)+1);

    backup = argv[2];
    printf("Source %s, backup %s\n", source, backup);

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

    if (stat(source, &(node2.inode->statbuf)) == -1) { 
        perror("Failed to get file status");
        exit(1);
    } else{
        node2.name = source;
        treeIt = sourceTree.insert(sourceTree.begin(), node2);
    }
    // printTree(sourceTree);
    
    makeDirectoryTree(source, source, &sourceTree, treeIt, treeIt);

    printf("------Printing source tree-----------\n");
    printTree(sourceTree);


    syncFolders(&sourceTree, &destinationTree);

    printTree(destinationTree);




   inotifyFd = inotify_init();                 /* Create inotify instance */
   if (inotifyFd == -1)
       perror("inotify_init");
   /* For each command-line argument, add a watch for all events */

    watchDescriptors = assignWatchers(&sourceTree, inotifyFd);

    for (std::map<int,tree<Node>::pre_order_iterator>::iterator it=watchDescriptors.begin(); it!=watchDescriptors.end(); ++it){
        printNode(*(it->second));
    }

   for (;;) {                                  /* Read events forever */
       numRead = read(inotifyFd, buf, BUF_LEN);
       if (numRead == 0)
           perror("read() from inotify fd returned 0!");

       if (numRead == -1) {
           perror("read");
       }
       printf("Read %ld bytes from inotify fd\n", (long) numRead);
       /* Process all of the events in buffer returned by read() */
       for (p = buf; p < buf + numRead; ) {
           event = (struct inotify_event *) p;
            // since the MOVED FROM and TO events have some special cases about order, we handle them first
           if (event->mask & IN_MOVED_FROM) {
                mInfo.lastEventWasMOVEDFROM = true;
                printf("IN_MOVED_FROM handler. Making a note of cookie and waiting for next event");
                mInfo.cookie = event->cookie;
                event2 = (struct inotify_event *) p;
               // is next event after IN_MOVED_FROM an IN_MOVED_TO?
                if(event2->mask & IN_MOVED_TO) {
                    mInfo.lastEventWasMOVEDFROM = false;
                    // handle IN_MOVED_TO
                    if(event2->cookie == mInfo.cookie){
                        printf("IN_MOVED_TO after IN_MOVED_FROM handler\n");
                        printf("We are just renaming\n");
                    } else{
                        // from the spec, "act as in IN_CREATE"
                        handleIN_CREATE(watchDescriptors[event2->wd], &destinationTree, &sourceTree, event2->name, source,  backup);
                    }
                } else{
                    // unlink the file in question
                    printf("IN_MOVED_FROM that is not followed by IN_MOVED_TO. Simply unlink the file in question\n We do this by handling as if it was an IN_DELETE\n");
                    handleIN_DELETE(watchDescriptors[event->wd], &destinationTree, &sourceTree, event->name, source, backup);
                }

           } else {
               // if it wasn't one of our special events we can just handle them regularly
                handleInotifyEvents(event, watchDescriptors, &destinationTree, &sourceTree, source, backup, &mInfo);
           }

           p += sizeof(struct inotify_event) + event->len;
       }
   }

}

static void handleInotifyEvents(struct inotify_event *i, map<int, tree<Node>::pre_order_iterator> watchDescriptors, tree<Node>* destinationTree, tree<Node>* sourceTree, char* sourceRoot, char* backupRoot, struct MOVEEventInfo *mInfo) {
    printf("wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ATTRIB){
        handleIN_ATTRIB(watchDescriptors[i->wd], destinationTree, sourceTree, i->name, sourceRoot);
        mInfo->lastEventWasMOVEDFROM = false;
    }
    if (i->mask & IN_CLOSE_WRITE){
        handleIN_CLOSE_WRITE(watchDescriptors[i->wd], destinationTree, sourceTree, i->name, sourceRoot, backupRoot);
        mInfo->lastEventWasMOVEDFROM = false;
    }
    if (i->mask & IN_CREATE){
        handleIN_CREATE(watchDescriptors[i->wd], destinationTree, sourceTree, i->name, sourceRoot,  backupRoot);
        mInfo->lastEventWasMOVEDFROM = false;
    }
    if (i->mask & IN_DELETE){
        handleIN_DELETE(watchDescriptors[i->wd], destinationTree, sourceTree, i->name, sourceRoot, backupRoot);
        mInfo->lastEventWasMOVEDFROM = false;
    }
    if (i->mask & IN_DELETE_SELF){
        mInfo->lastEventWasMOVEDFROM = false;
    }
    if (i->mask & IN_MODIFY){
        handleIN_MODIFY(watchDescriptors[i->wd], sourceTree, i->name);
        mInfo->lastEventWasMOVEDFROM = false;
    }
//    if (i->mask & IN_MOVED_FROM){
//        printf("IN_MOVED_FROM handler\n");
//        mInfo->lastEventWasMOVEDFROM = true;
//        mInfo->cookie = i->cookie;
//    }
    if (i->mask & IN_MOVED_TO){
        printf("IN_MOVED_TO handler\n");
        if(!mInfo->lastEventWasMOVEDFROM){
            // from the spec, "act as in IN_CREATE"
            handleIN_CREATE(watchDescriptors[i->wd], destinationTree, sourceTree, i->name, sourceRoot,  backupRoot);
        }
        mInfo->lastEventWasMOVEDFROM = false;
    }
//    if (i->mask & IN_OPEN)          printf("IN_OPEN ");
//    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
//    if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}





