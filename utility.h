#ifndef SORT_H   /* Include guard */
#define SORT_H

#include <string>
#include <sys/stat.h>

#include <dirent.h>
#include "tree.hh"

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
#include<string>
#include <map>

#define MAX_HARDLINKS 512
using namespace std;



void printTree(tree<struct Node> tr);
void printNode(struct Node node);
static void displayInotifyEvent(struct inotify_event *i);
void traverseDir(char *);
void makeDirectoryTree(char* dir_path, char* root, tree<Node> * dirTree, tree<Node>::pre_order_iterator it, tree<Node>::pre_order_iterator newIt);
tree<Node>::pre_order_iterator findInodeByNum(ino_t inode_number, tree<Node> *searchTree);
tree<Node>::pre_order_iterator findNodeByName(string name, tree<Node> *searchTree);
void syncFolders(tree<Node>* sourceTree, tree<Node>* destinationTree);
bool isDirectory(struct Node node);
void copyFile(const char* path, const char* source, const char* backup);
map<int, tree<Node>::pre_order_iterator> assignWatchers(tree<Node>* sourceTree, int inotifyFd);
void handleIN_ATTRIB(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot);
void handleIN_CREATE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot);
void handleIN_MODIFY(tree<Node>::pre_order_iterator it, tree<Node> *sourceTree, char* modifiedFileName);
void handleIN_CLOSE_WRITE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot);
void handleIN_DELETE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot);
void handleIN_DELETE_SELF(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot);
//u_int32_t handleIN_MOVED_FROM(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot);
void handleIN_MOVED_TO(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot);
char* getRelativePath(const char* name, const char* rootFolder);

typedef struct Inode {
	struct stat statbuf;
    string linkedFiles[MAX_HARDLINKS];
	int hardLinks;
	bool unsaved_changes = false;
} Inode;

typedef struct Node {
	string name;
	struct Inode *inode;
} Node;

typedef struct MOVEEventInfo{
    bool lastEventWasMOVEDFROM = false;
    u_int32_t cookie;
} MOVEEventInfo;

Inode* existingInode(tree<Node>* sourceTree, ino_t inode_number);
bool NameLinksToInodeNumber(string name, Inode* inode);
#endif // SORT_H
