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
#include <map>


using namespace std;

void printTree(tree<struct Node> tr) {

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
}

void printNode(struct Node node){
	cout<< "Name: " << node.name <<", ";
	cout<< "inode number: " << node.inode->statbuf.st_ino <<endl;
	// printf("Node name: %s Inode name: %s\n", node.name, node.inode.name);
}

char* getRelativePath(const char* name, const char* rootFolder){
	char *relatvePath = (char*)malloc(sizeof(char)*100);
	memset(relatvePath,0,strlen(relatvePath));
	strcpy(relatvePath, rootFolder);
	strcat(relatvePath, "/");
	strcat(relatvePath, name);
	return relatvePath;
}

map<int, tree<Node>::pre_order_iterator> assignWatchers(tree<Node>* sourceTree, int inotifyFd) {

	tree<Node>::pre_order_iterator it  = (*sourceTree).begin();
	tree<Node>::pre_order_iterator end = (*sourceTree).end();
	map<int, tree<Node>::pre_order_iterator>  watchDescriptors;
	char *path = (char*)malloc(512*sizeof(char));
	string root = (*it).name;
	int wd;

	while(it!=end) {

		if(isDirectory(*it)) {

			if(root == (*it).name){
				strcpy(path,(*it).name.c_str());
			} else {
				strcpy(path, getRelativePath((*it).name.c_str(), root.c_str()));
			}

			printf("DIR: Assign watcher to %s\n", path);

			// Assign inotify watchers to all directories
			wd = inotify_add_watch(inotifyFd, path, IN_ALL_EVENTS);
			   if (wd == -1)
			       perror("inotify_add_watch");

			printf("Watching %s using wd %d\n", path, wd);
			watchDescriptors[wd] = it;
			// return watchDescriptors;
		}
		++it;
	}
	return watchDescriptors;
}

bool isDirectory(struct Node node){
	if ((node.inode->statbuf.st_mode & S_IFMT) == S_IFDIR) {
		return true;
	}
	return false;
}


//TODO: Fix an extra parameter argument
void makeDirectoryTree(char* dir_path, char* root, tree<Node> * dirTree, tree<Node>::pre_order_iterator it, tree<Node>::pre_order_iterator newIt) {
	
	struct dirent *direntp;
	struct stat statbuf;
	char *name = (char*)malloc(sizeof(char)*100);
	DIR *dir_ptr;
	struct Inode *inode;
	struct Node node;
	if(strcmp(dir_path, root) != 0 ){
		dir_ptr = opendir(getRelativePath(dir_path, root));
	} else {
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
			// memset(path,0,strlen(path));

			if(strcmp(dir_path, root) != 0 ){
				strcpy(name, dir_path);
				strcat(name, "/");
				strcat(name, direntp->d_name);
				node.name = name;

			} else{
				node.name = direntp->d_name;
				strcat(name, direntp->d_name);
			}
			
			// strcpy(path, getRelativePath(name, root));

			if (stat(getRelativePath(name, root), &(node.inode->statbuf)) == -1) { 
				perror("Failed to get file status");
				exit(1);
			}
			newIt = (*dirTree).append_child(it, node);
			if (isDirectory(node)) {
				makeDirectoryTree(name, root, dirTree, newIt, newIt);
			}
		}
	}
	closedir(dir_ptr);
}


tree<Node>::pre_order_iterator findInodeByNum(ino_t inode_number, tree<Node> *searchTree) {
	tree<Node>::pre_order_iterator it  = (*searchTree).begin();
	tree<Node>::pre_order_iterator end = (*searchTree).end();

	while(it!=end) {
		if((*it).inode->statbuf.st_ino == inode_number) {
			return it;
		}
		++it;
	}
	return nullptr;
}

tree<Node>::pre_order_iterator findNodeByName(string name, tree<Node> *searchTree) {
	tree<Node>::pre_order_iterator it  = (*searchTree).begin();
	tree<Node>::pre_order_iterator end = (*searchTree).end();

	while(it!=end) {
		if(name == (*it).name) {
			return it;
		}
		++it;
	}
	return nullptr;
}

/*Simultaneously traverse the two trees in “depth first” fashion and maintain parity*/
/*The pre_order_iterator here traveres DFS*/
void syncFolders(tree<Node>* sourceTree, tree<Node>* destinationTree) {
	tree<Node>::pre_order_iterator s_it  = (*sourceTree).begin();
	tree<Node>::pre_order_iterator s_end = (*sourceTree).end();

	tree<Node>::pre_order_iterator d_it  = (*destinationTree).begin();
	tree<Node>::pre_order_iterator d_end = (*destinationTree).end();

	tree<Node>::pre_order_iterator parentIter;
	tree<Node>::pre_order_iterator newIt;
	string nameOfParent;
	tree<Node>::pre_order_iterator currentParent;
	string rootSourceName = (*s_it).name;
	string rootDestName = (*d_it).name;
	struct Inode *inode;
	struct Node node;
	cout<<rootSourceName<<" "<<rootDestName<<endl;
	char* dir_path;
	char* file_path;

	++s_it;
	++d_it;

	while(s_it!=s_end) {
		inode = new Inode;
    	node = {"", .inode = inode};
		if(d_it == d_end) {
			parentIter = sourceTree->parent(s_it);
			nameOfParent = (*parentIter).name;
			//printf("Printing parent: ");
			printNode(*parentIter);
			if(nameOfParent==rootSourceName){
			  //printf("parent == root\n");
				currentParent = (*destinationTree).begin();
				
			} else {
			  //printf("parent IS NOT root\n");
				currentParent = findNodeByName(nameOfParent, destinationTree);
			}
			 
			if(currentParent != nullptr){
				printNode(*currentParent);
			} else {
			  printf("Parent does not exist. Exiting...\n");
				break;
			}

			node.name = (*s_it).name;
			if(isDirectory(*s_it)) {
				//Make directory
				// printf("Naame: %s Root: %s\n", (*s_it).name.c_str(), rootDestName.c_str());
				//printf("TODO make a directory at %s\n", getRelativePath((*s_it).name.c_str(), rootDestName.c_str()));
				// printf("TODO Insert node with append child to currentParent\n");
				dir_path = getRelativePath((*s_it).name.c_str(), rootDestName.c_str());
				// printf("MAKING DIRECTOTY AT: %s\n", dir_path);
				mkdir(dir_path, ACCESSPERMS);
				if (stat(dir_path, &(node.inode->statbuf)) == -1) { 
					perror("Failed to get dir status");
					exit(1);
				}
			} else {
				//TODO: Create a file, copy the content, assign the stat of the new file to stat struct
			        // printf("Creating copy of file in backup %s at %s...", (*s_it).name.c_str(), rootDestName.c_str());
				copyFile((*s_it).name.c_str(), rootSourceName.c_str(), rootDestName.c_str());
				// printf("copy complete. File-node details: ");
				file_path = getRelativePath((*s_it).name.c_str(), rootDestName.c_str());
				if(stat(file_path, &(node.inode->statbuf)) == -1){
				  perror("Failed to get file status");
				  exit(1);
				}
				// printNode(node);
			}
			// printNode(*currentParent);
			newIt = (*destinationTree).append_child(currentParent, node);
			//printf("CREATED NEW NODE, PRINTING: ");
			//printNode(*newIt);
			//printTree(*destinationTree);
		}
		++s_it;
	}
}


void copyFile(const char* path, const char* source, const char* backup){
	char* copyFrom = (char*)malloc(sizeof(char)*512);
	char* copyTo = (char*)malloc(sizeof(char)*512);
	strcpy(copyFrom, getRelativePath(path, source));
	strcpy(copyTo, getRelativePath(path, backup));
	//cout<< "File read: "<<copyFrom<<", file write: "<<copyTo<<endl;
	
	char buffer[4096];
	size_t bytes;
	FILE *readFrom = fopen(copyFrom,"r");
	FILE *writeTo = fopen(copyTo, "w");
	if(readFrom == NULL || writeTo == NULL){
	  //cout<<"Opening File Pointers Failed"<<endl;
		return;
	}

	while ((bytes = fread(buffer, 1, sizeof(buffer), readFrom)) != 0){
		fwrite(buffer, 1, bytes, writeTo);
	}
	//cout<<"Got here too"<<endl;
	fclose(readFrom);
	fclose(writeTo);
}
