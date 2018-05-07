#ifndef SORT_H   /* Include guard */
#define SORT_H

#include <string>
#include <sys/stat.h>
#include "tree.hh"
#include <dirent.h>


using namespace std;

void printTree(tree<struct Node> tr);
void printNode(struct Node node);
static void displayInotifyEvent(struct inotify_event *i);
void traverseDir(char *);
void makeDirectoryTree(char* dir_path, char* root, tree<Node> * dirTree, tree<Node>::pre_order_iterator it, tree<Node>::pre_order_iterator newIt);
Node* findInodeByNum(ino_t inode_number, tree<Node> searchTree);
tree<Node>::pre_order_iterator findNodeByName(string name, tree<Node> *searchTree);
void syncFolders(tree<Node>* sourceTree, tree<Node>* destinationTree);
bool isDirectory(struct Node node);
void copyFile(const char* path, const char* source, const char* backup);
void displayInotifyEvent(struct inotify_event* i);

typedef struct Inode {
	struct stat statbuf;
	int hardLinks;
} Inode;

typedef struct Node {
	string name;
	struct Inode *inode;
} Node;


#endif // SORT_H
