#include <stdio.h>
#include <sys/inotify.h>
#include <cstring>
#include <string.h>
#include <string>

#include "utility.h"
#include "tree.hh"
#include <iostream>
#include <string.h>
#include "tree_util.hh"
#include <dirent.h>


using namespace std;

void printTree(tree<struct Node> tr) {
	// tree<Node>::iterator sib2=tr.begin();
	// tree<Node>::iterator end2=tr.end();

	tree<Node>::pre_order_iterator it = tr.begin();
	tree<Node>::pre_order_iterator end = tr.end();

	int rootdepth=tr.depth(it);
	std::cout << "-----" << std::endl;
	while(it!=end) {
		for(int i=0; i<tr.depth(it)-rootdepth; ++i) {
			std::cout << "  ";
		}
		printNode(*it); 
		++it;
	}
	std::cout << "-----" << std::endl;

	// while(sib2!=end2) {
	// 	for(int i=0; i<tr.depth(sib2); ++i) 
	// 		cout << " ";
	// 	printNode(*sib2);
	// 	++sib2;
	// }
}

void printNode(struct Node node){
	cout<< "Name: " << node.name <<", ";
	cout<< "inode number: " << node.inode->statbuf.st_ino <<endl;
	// printf("Node name: %s Inode name: %s\n", node.name, node.inode.name);
}


/* Display information from inotify_event structure */
static void displayInotifyEvent(struct inotify_event *i)
{
    printf("wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)          printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

void traverseDir(char* dir_path) {
	struct dirent *direntp;
	struct stat statbuf;
	char name [100];
	DIR *dir_ptr;

	dir_ptr = opendir(dir_path);
    if(dir_ptr == nullptr) {
        perror("Open dir");
        exit(1);
    }

	while ( ( direntp=readdir(dir_ptr) ) != NULL ){
		
		if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
			printf(". or ..\n");
		} else{
			memset(name,0,strlen(name));
			strcpy(name, dir_path);
			strcat(name, "/");
			strcat(name, direntp->d_name);
			printf("inode %d of the entry %s \n", \
					(int)direntp->d_ino, name);
			if (stat(name, &statbuf) == -1) { 
				perror("Failed to get file status");
			}
			else {
				if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
					printf("Recursing on %s!\n", name);
					traverseDir(name);
				}
			}
		}
	}
	closedir(dir_ptr);
}

bool isDirectory(struct Node node){
	if ((node.inode->statbuf.st_mode & S_IFMT) == S_IFDIR) {
		return true;
	}
	return false;
}

char* getRelativePath(char* name, char* rootFolder){
	char relatvePath[100];
	memset(relatvePath,0,strlen(relatvePath));
	strcpy(relatvePath, rootFolder);
	strcat(relatvePath, "/");
	strcat(relatvePath, name);
	return relatvePath;
}


//TODO: Fix an extra parameter argument
void makeDirectoryTree(char* dir_path, char* root, tree<Node> * dirTree, tree<Node>::pre_order_iterator it, tree<Node>::pre_order_iterator newIt) {
	
	struct dirent *direntp;
	struct stat statbuf;
	char name [100];
	DIR *dir_ptr;
	struct Inode *inode;
	struct Node node;
	char path[100];
	if(strcmp(dir_path, root) != 0 ){
		memset(path,0,strlen(path));
		strcpy(path,getRelativePath(name, root));
		printf("Name and root %s, %s:\n", root, name);
		cout<<"Line 142 Path: "<< path <<endl;
		dir_ptr = opendir(path);
	} else {
		cout<<"Line 145 dir path, root: "<<dir_path<<" "<<root<<endl;
		dir_ptr = opendir((dir_path, root));
	}
    if(dir_ptr == nullptr) {
        perror("Open dir");
        exit(1);
    }

	while ( ( direntp=readdir(dir_ptr) ) != NULL ){
		inode = new Inode;
    	node = {"", .inode = inode};
		
		if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
			// printf(". or ..\n");
		} else{
			memset(name,0,strlen(name));
			memset(path,0,strlen(path));

			cout<< "Comparing: " << dir_path << " and "<< root<< endl;
			if(strcmp(dir_path, root) != 0 ){
				strcpy(name, dir_path);
				strcat(name, "/");
				strcat(name, direntp->d_name);
				// strcpy(path,getRelativePath(name, root));
				node.name = name;

				cout<<"This is NOT root; name is"<<name<<endl;
			} else{
				// strcat(name, direntp->d_name);
				node.name = path;
				cout<<"This is root "<<name<<endl;
				strcat(name, direntp->d_name);
				// strcat(name, "/");
			}
			printf("At the end %s %s\n", dir_path, root);
			// printf("inode %d of the entry %s \n", \
					(int)direntp->d_ino, name);
			
			
			
			strcpy(path, getRelativePath(name, root));
			cout<<"Path: "<< path<<endl;
			printf("At the end %s %s\n", dir_path, root);


			if (stat(path, &(node.inode->statbuf)) == -1) { 
				perror("Failed to get file status");
				exit(1);
			}
			else {
				cout<<"Name: "<<name<<endl;
				cout<< "Inode: "<< node.inode->statbuf.st_ino<<endl;
				// printf("inode: %d\n", node.inode->statbuf.st_ino);
				newIt = (*dirTree).append_child(it, node);
				if ((node.inode->statbuf.st_mode & S_IFMT) == S_IFDIR) {
					printf("Recursing on a directory with root %s\n", root);
					strcpy(dir_path, name);
					printf("Recursing on a directory with name %s\n", dir_path);

					makeDirectoryTree(dir_path, root, dirTree, newIt, newIt);
				}
			}
		}
	}
	closedir(dir_ptr);
}


Node* findInodeByNum(ino_t inode_number, tree<Node> searchTree){
	tree<Node>::pre_order_iterator it  = searchTree.begin();
	tree<Node>::pre_order_iterator end = searchTree.end();

	while(it!=end) {
		if((*it).inode->statbuf.st_ino == inode_number){
			return (&(*it));
		}
		++it;
	}
	return nullptr;
}

Node* findNodeByName(string name, tree<Node> searchTree){
	tree<Node>::pre_order_iterator it  = searchTree.begin();
	tree<Node>::pre_order_iterator end = searchTree.end();

	while(it!=end) {
		if(name == (*it).name){
			return (&(*it));
		}
		++it;
	}
	return nullptr;
}

/*Simultaneously traverse the two trees in “depth first” fashion and maintain parity*/
/*The pre_order_iterator here traveres DFS*/
void SyncFolders(tree<Node>* sourceTree, tree<Node>* destinationTree){
	tree<Node>::pre_order_iterator s_it  = (*sourceTree).begin();
	tree<Node>::pre_order_iterator s_end = (*sourceTree).end();

	tree<Node>::pre_order_iterator d_it  = (*destinationTree).begin();
	tree<Node>::pre_order_iterator d_end = (*destinationTree).end();

	++s_it;
	++d_it;

	while(s_it!=s_end) {
		if(d_it == d_end){
			if(isDirectory(*s_it)){
				mkdir(((*d_it).name).c_str(), ACCESSPERMS);
			}
		}
		
		++s_it;
	}
}
