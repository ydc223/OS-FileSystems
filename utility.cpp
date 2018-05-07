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
			wd = inotify_add_watch(inotifyFd, path, IN_CREATE |  IN_ATTRIB | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
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
			
			if (isDirectory(node)) {
				newIt = (*dirTree).append_child(it, node);
				makeDirectoryTree(name, root, dirTree, newIt, newIt);
			}
			else{
				// if there already exists an inode with the stat'd inode number, this tree node should just point to the same inode
				Inode *existing = existingInode(dirTree, node.inode->statbuf.st_ino);
				// !NameLinksToInodeNumber(getRelativePath(name, root), existing)
				if(existing != nullptr ) {
					cout<<"Already existing Inode for this file: "<<existing->linked_files[0]<<". Linking to it..."<<endl;
					node.inode = existing; 
					node.inode->linked_files[node.inode->hardLinks] = name;
					node.inode->hardLinks++;
				} else{
					node.inode->hardLinks = 1;
					node.inode->linked_files[0] = name;
					cout<<"Had to make a new inode. It has "<<node.inode->hardLinks<<" hardLinks"<<endl;
				}

				newIt = (*dirTree).append_child(it, node);
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

int linkIfInodeExists(Node sourceNode, tree<Node>* backup, Node* backupNode) {
	if(sourceNode.inode->hardLinks == 1) {
		return 1;
	} 

	tree<Node>::pre_order_iterator it  = (*backup).begin();
	tree<Node>::pre_order_iterator end = (*backup).end();
	tree<Node>::pre_order_iterator found;
	string backupName = (*it).name;

	for (int i = 0; i < sourceNode.inode->hardLinks; ++i)
	{
		if (sourceNode.inode->linked_files[i] != sourceNode.name) {
			found = findNodeByName(sourceNode.inode->linked_files[i], backup);
			if(found != nullptr) {
				printf("Found a link with %s name, which is %u\n", sourceNode.inode->linked_files[i].c_str(), (*found).inode->statbuf.st_ino);
				link(getRelativePath((*found).name.c_str(), backupName.c_str()), getRelativePath(sourceNode.name.c_str(), backupName.c_str()));
				(*found).inode->linked_files[(*found).inode->hardLinks] = backupNode->name;
				(*found).inode->hardLinks++;
				backupNode->inode = (*found).inode;
				return 0;
			}
		}
		/* code */
	}
	return 1;
}

/*Simultaneously traverse the two trees in “depth first” fashion and maintain parity*/
/*The pre_order_iterator here traveres DFS*/
void syncFolders(tree<Node>* sourceTree, tree<Node>* destinationTree) {
	tree<Node>::pre_order_iterator s_it  = (*sourceTree).begin();
	tree<Node>::pre_order_iterator s_end = (*sourceTree).end();

	tree<Node>::pre_order_iterator d_it  = (*destinationTree).begin();
	tree<Node>::pre_order_iterator d_end = (*destinationTree).end();

	tree<Node>::pre_order_iterator parentIter;
	string nameOfParent;
	tree<Node>::pre_order_iterator currentParent;
	string rootSourceName = (*s_it).name;
	string rootDestName = (*d_it).name;
	struct Inode *inode;
	struct Node node;
	cout<<rootSourceName<<" "<<rootDestName<<endl;
	char* dir_path;
	char* file_path;
	int ret;

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
				
				

				//Does this file we just created have the same inode number as another file in Source?
				// Inode *existing = existingInode(sourceTree, (*s_it).inode->statbuf.st_ino);
				ret = linkIfInodeExists(*s_it, destinationTree, &node);

				// if() {
				// 	cout<<"Already existing Inode for this file. Storing info in Inode..."<<endl;
				// 	node.inode = existing; 
				// 	node.inode->linked_files[node.inode->hardLinks] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
				// 	node.inode->hardLinks++;
				// } else {
				// 	node.inode->hardLinks = 1;
				// 	node.inode->linked_files[0] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
				// 	cout<<"Created new inode. It has "<<node.inode->hardLinks<<" hardLinks"<<endl;
				// }
				// printNode(node);

				if(ret) {
					copyFile((*s_it).name.c_str(), rootSourceName.c_str(), rootDestName.c_str());
					// printf("copy complete. File-node details: ");
					file_path = getRelativePath((*s_it).name.c_str(), rootDestName.c_str());
					if(stat(file_path, &(node.inode->statbuf)) == -1){
					  perror("Failed to get file status");
					  exit(1);
					}

					node.inode->hardLinks = 1;
					node.inode->linked_files[0] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
					cout<<"Created new inode. It has "<<node.inode->hardLinks<<" hardLinks"<<endl;
				}
			}
			// printNode(*currentParent);
			(*destinationTree).append_child(currentParent, node);
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

Inode* existingInode(tree<Node>* sourceTree, ino_t inode_number){
	tree<Node>::pre_order_iterator it  = (*sourceTree).begin();
	tree<Node>::pre_order_iterator end = (*sourceTree).end();

	while(it!=end) {
		if((*it).inode->statbuf.st_ino == inode_number){
			cout<<"EXISTING INODE SAY TRUE"<<(*it).inode->linked_files[0]<<endl;
			return (*it).inode;
		}
		// printNode(*it); 
		++it;
	}
	cout<<"EXISTING INODE SAY NOPE"<<endl;
	return nullptr;
}

bool NameLinksToInodeNumber(string name, Inode* inode){
	int links = inode->hardLinks;
	for(int i = 0; i < links; i++){
		if(inode->linked_files[i] == name){
			return true;
		}
	}
	return false;
}