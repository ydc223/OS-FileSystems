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
#include <time.h>
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
	char *relativePath = (char*)malloc(sizeof(char)*100);
	memset(relativePath,0,strlen(relativePath));
	strcpy(relativePath, rootFolder);
	strcat(relativePath, "/");
	strcat(relativePath, name);
	return relativePath;
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
					cout<<"Already existing Inode for this file: "<<existing->linkedFiles[0]<<". Linking to it..."<<endl;
					node.inode = existing; 
					node.inode->linkedFiles[node.inode->hardLinks] = name;
					node.inode->hardLinks++;
				} else{
					node.inode->hardLinks = 1;
					node.inode->linkedFiles[0] = name;
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
		if (sourceNode.inode->linkedFiles[i] != sourceNode.name) {
			found = findNodeByName(sourceNode.inode->linkedFiles[i], backup);
			if(found != nullptr) {
				printf("Found a link with %s name, which is %u\n", sourceNode.inode->linkedFiles[i].c_str(), (*found).inode->statbuf.st_ino);
				link(getRelativePath((*found).name.c_str(), backupName.c_str()), getRelativePath(sourceNode.name.c_str(), backupName.c_str()));
				(*found).inode->linkedFiles[(*found).inode->hardLinks] = backupNode->name;
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
				dir_path = getRelativePath((*s_it).name.c_str(), rootDestName.c_str());
				// printf("MAKING DIRECTOTY AT: %s\n", dir_path);
				mkdir(dir_path, ACCESSPERMS);
				if (stat(dir_path, &(node.inode->statbuf)) == -1) { 
					perror("Failed to get dir status");
					exit(1);
				}
			} else {
			        // printf("Creating copy of file in backup %s at %s...", (*s_it).name.c_str(), rootDestName.c_str());
				
				

				//Does this file we just created have the same inode number as another file in Source?
				// Inode *existing = existingInode(sourceTree, (*s_it).inode->statbuf.st_ino);
                // if() {
                // 	cout<<"Already existing Inode for this file. Storing info in Inode..."<<endl;
                // 	node.inode = existing;
                // 	node.inode->linkedFiles[node.inode->hardLinks] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
                // 	node.inode->hardLinks++;
                // } else {
                // 	node.inode->hardLinks = 1;
                // 	node.inode->linkedFiles[0] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
                // 	cout<<"Created new inode. It has "<<node.inode->hardLinks<<" hardLinks"<<endl;
                // }
                // printNode(node);
				ret = linkIfInodeExists(*s_it, destinationTree, &node);
				if(ret) {
					copyFile((*s_it).name.c_str(), rootSourceName.c_str(), rootDestName.c_str());
					// printf("copy complete. File-node details: ");
					file_path = getRelativePath((*s_it).name.c_str(), rootDestName.c_str());
					if(stat(file_path, &(node.inode->statbuf)) == -1){
					  perror("Failed to get file status");
					  exit(1);
					}

					node.inode->hardLinks = 1;
					node.inode->linkedFiles[0] = getRelativePath((*s_it).name.c_str(), rootSourceName.c_str());
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
	cout<< "File read: "<<copyFrom<<", file write: "<<copyTo<<endl;
	
	char buffer[4096];
	size_t bytes;
	FILE *readFrom = fopen(copyFrom,"r");
	FILE *writeTo = fopen(copyTo, "w");
	if(readFrom == NULL || writeTo == NULL){
	  cout<<"Opening File Pointers Failed"<<endl;
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
			cout<<"EXISTING INODE SAY TRUE"<<(*it).inode->linkedFiles[0]<<endl;
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
		if(inode->linkedFiles[i] == name){
			return true;
		}
	}
	return false;
}

void handleIN_ATTRIB(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot) {
    cout<<"IN_ATTRIB HANDLER"<<endl;
    char modFilePath[512];
    strcpy(modFilePath, modifiedFileName);
    tree<Node>::pre_order_iterator modifiedNode = findNodeByName(modFilePath, sourceTree);
    tree<Node>::pre_order_iterator backupNode = findNodeByName(modFilePath, backupTree);
    if(modifiedNode == nullptr || backupNode == nullptr){
        cout<<"Node is not at root level, checking deep"<<endl;
        strcpy(modFilePath, getRelativePath(modifiedFileName, (*it).name.c_str()));
        modifiedNode = findNodeByName(modFilePath, sourceTree);
        backupNode = findNodeByName(modFilePath, backupTree);
        if(modifiedNode == nullptr || backupNode == nullptr){
            cout<<"We exit disgracefully";
            exit(1);
        } else{
            cout<<"Found it. Nodes: "<<(*modifiedNode).name<<" and "<<(*backupNode).name<<endl;
        }

    }

    if(isDirectory((*modifiedNode))){
        cout<<"Call was to a directory, we do nothing"<<endl;
        return;
    }
    strcpy(modFilePath, getRelativePath(modFilePath, sourceRoot));

    cout<<"Handling IN_ATTRIB call which happened on file "<<(*modifiedNode).name<<" in the source folder: "<<sourceRoot;


	/*Call statbuf again on the node that was passed in, as we know it has a new time of modification.
	 * Take that statbuf's last-modified date and put it into the statbuf of the corresponding node in backup if bigger*/

	if(stat(modFilePath, &(*modifiedNode).inode->statbuf) == -1){
	    perror("Failed to statbuf");
	    exit(1);
	}


    if((*backupNode).inode->statbuf.st_mtim.tv_sec < (*modifiedNode).inode->statbuf.st_mtim.tv_sec){
		cout<<"Time of last modification is more recent in source. Updating in backup..."<<endl;
		(*backupNode).inode->statbuf.st_mtim.tv_sec = (*modifiedNode).inode->statbuf.st_mtim.tv_sec ;
		return;
	}
	cout<<"Modification times did not differ, we do nothing"<<endl;
}

void handleIN_CREATE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot){
    // IN_CREATE event is created when there's a new file. So we need to take care of this new file accordingly.
    cout<<"IN_CREATE HANDLER"<<endl;
    int ret;
    string nameOfParent = (*it).name;
    char modFilePath[512];
    Node createdNode;
    Node createdBackupNode;
    Inode* inode = new Inode;
    Inode* backupInode = new Inode;
    createdNode = {"", .inode = inode};
    createdBackupNode = {"", .inode = backupInode};
    tree<Node>::pre_order_iterator backupParent;
    if((*it).name == sourceRoot){
        backupParent = findNodeByName(backupRoot, backupTree);
    }
    else{
        backupParent = findNodeByName((*it).name, backupTree);
    }
    if(backupParent == nullptr){
        cout<<"You actually failed a sanity check, well done"<<endl;
        exit(1);
    }
    else{
        cout<<"Sanity check passed, phew"<<endl;
    }
    if((*it).name.c_str() == sourceRoot){
        createdNode.name = modifiedFileName;
        createdBackupNode.name = modifiedFileName;
    }
    else{
        createdNode.name = getRelativePath(modifiedFileName, (*it).name.c_str());
        createdBackupNode.name = getRelativePath(modifiedFileName, (*backupParent).name.c_str());
    }
    cout<<"created node with name: "<<createdNode.name<<endl;
    if (stat(getRelativePath(createdNode.name.c_str(), sourceRoot), &(createdNode.inode->statbuf)) == -1) {
        perror("Failed to get file status");
        exit(1);
    }
    Inode *existing = existingInode(sourceTree, createdNode.inode->statbuf.st_ino);
    // !NameLinksToInodeNumber(getRelativePath(name, root), existing)
    if(existing != nullptr ) {
        cout<<"Already existing Inode for this file: "<<existing->linkedFiles[0]<<". Linking to it..."<<endl;
        createdNode.inode = existing;
        createdNode.inode->linkedFiles[createdNode.inode->hardLinks] = createdNode.name;
        createdNode.inode->hardLinks++;
    } else{
        createdNode.inode->hardLinks = 1;
        createdNode.inode->linkedFiles[0] = createdNode.name;
        cout<<"Had to make a new inode. It has "<<createdNode.inode->hardLinks<<" hardLinks"<<endl;
    }
    sourceTree->append_child(it, createdNode);
    ret = linkIfInodeExists(createdNode, backupTree, &createdBackupNode);
    if(ret) {
        copyFile(createdNode.name.c_str(), sourceRoot, backupRoot);
        // printf("copy complete. File-node details: ");
        strcpy(modFilePath,getRelativePath(createdBackupNode.name.c_str(), backupRoot));
        if(stat(modFilePath, &(createdBackupNode.inode->statbuf)) == -1){
            perror("Failed to get file status");
            exit(1);
        }

        createdBackupNode.inode->hardLinks = 1;
        createdBackupNode.inode->linkedFiles[0] = getRelativePath(createdNode.name.c_str(), sourceRoot);
        cout<<"Created new inode. It has "<<createdBackupNode.inode->hardLinks<<" hardLinks"<<endl;
    }

    backupTree->append_child(backupParent, createdBackupNode);
    if(findNodeByName(createdBackupNode.name, backupTree) == nullptr ){
        cout<<"DIDN'T ATTACH TO BACKUP YO"<<endl;
    }
    if(findNodeByName(createdNode.name, sourceTree) == nullptr){
        cout<<"DIDN'T ATTACK TO SOURCE YO"<<endl;
    }
    // If isDirectory, create a new directory; add a node to the tree for this directory; add the directory to the list of watched objects by inotify
    // Implement the same logic as up in syncfolders to deal with inode stuff
}

void handleIN_MODIFY(tree<Node>::pre_order_iterator it, tree<Node> *sourceTree, char* modifiedFileName){
	cout<<"IN_MODIFY HANDLER"<<endl;
    char modFilePath[512];
//    strcpy(modFilePath, getRelativePath(modifiedFileName, (*it).name.c_str()));
    tree<Node>::pre_order_iterator modifiedNode = findNodeByName(modifiedFileName, sourceTree);
    if(modifiedNode == nullptr){
        cout<<"Node is not at root level, checking deep"<<endl;
        strcpy(modFilePath, getRelativePath(modifiedFileName, (*it).name.c_str()));
	    cout<<"Searching for the node"<<modFilePath<<endl;
         modifiedNode = findNodeByName(modFilePath, sourceTree);
            if(modifiedNode == nullptr) {
                cout << "We exit disgracefully";
                exit(1);
            }
    }
//    if(isDirectory((*modifiedNode))){
//        cout<<"Call was to a directory, we do nothing"<<endl;
//        return;
//    }
    cout<<"Handled IN_MODIFY. Flag set to true and changes will be saved on IN_CLOSE_WRITE"<<endl;
    (*modifiedNode).inode->unsaved_changes = true;
    cout<<(*modifiedNode).inode->unsaved_changes<<endl;
    return;
}

void handleIN_CLOSE_WRITE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot){
    cout<<"IN_CLOSE_WRITE HANDLER"<<endl;
    char modFilePath[512];
    strcpy(modFilePath, modifiedFileName);
    tree<Node>::pre_order_iterator modifiedNode = findNodeByName(modFilePath, sourceTree);
    tree<Node>::pre_order_iterator backupNode = findNodeByName(modFilePath, backupTree);
    if(modifiedNode == nullptr || backupNode == nullptr){
        cout<<"Node is not at root level, checking deep"<<endl;
        strcpy(modFilePath, getRelativePath(modifiedFileName, (*it).name.c_str()));
        modifiedNode = findNodeByName(modFilePath, sourceTree);
        backupNode = findNodeByName(modFilePath, backupTree);
        if(modifiedNode == nullptr || backupNode == nullptr){
            cout<<"We exit disgracefully";
            exit(1);
        } else{
	  cout<<"Found it. Nodes: "<<(*modifiedNode).name<<" and "<<(*backupNode).name<<endl;
        }

    }
    //if(isDirectory((*modifiedNode))){
      //cout<<"Call was to a directory, we do nothing"<<endl;
        //return;
	//}
    if((*modifiedNode).inode == nullptr){
      cout<<"WHY IS THIS A NULLPTR???"<<endl;
      return;
    }
    if((*modifiedNode).inode->unsaved_changes){
        copyFile(modFilePath, sourceRoot, backupRoot);
        (*modifiedNode).inode->unsaved_changes = false;
        cout<<"Unsaved changes were detected and saved"<<endl;
    }
    else{
        cout<<"No unsaved changes"<<endl;
    }
}

void handleIN_DELETE(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot){
    cout<<"Handling IN_DELETE...";
    char modFilePath[512];
    strcpy(modFilePath, modifiedFileName);
    tree<Node>::pre_order_iterator modifiedNode = findNodeByName(modFilePath, sourceTree);
    tree<Node>::pre_order_iterator backupNode = findNodeByName(modFilePath, backupTree);
    if(modifiedNode == nullptr || backupNode == nullptr){
        cout<<"Node is not at root level, checking deep"<<endl;
        strcpy(modFilePath, getRelativePath(modifiedFileName, (*it).name.c_str()));
        modifiedNode = findNodeByName(modFilePath, sourceTree);
        backupNode = findNodeByName(modFilePath, backupTree);
        if(modifiedNode == nullptr || backupNode == nullptr){
            cout<<"We exit disgracefully";
            exit(1);
        } else{
            cout<<"Found it. Nodes: "<<(*modifiedNode).name<<" and "<<(*backupNode).name<<endl;
        }

    }
    if(isDirectory((*modifiedNode))){
        cout<<"Call was to a directory, we do rmdir to "<<getRelativePath((*modifiedNode).name.c_str(), backupRoot)<<endl;
        (*sourceTree).erase(modifiedNode);
        (*backupTree).erase(backupNode);
	rmdir(getRelativePath((*modifiedNode).name.c_str(), backupRoot));
        return;
    }
    else{
      cout<<"Call was to a file, we unlink the file "<<getRelativePath((*modifiedNode).name.c_str(), backupRoot)<<endl;
        (*sourceTree).erase(modifiedNode);
        (*backupTree).erase(backupNode);
	unlink(getRelativePath((*modifiedNode).name.c_str(), backupRoot));
        return;
    }
}

// This implementation assumes it is ONLY BEING CALLED in the case where the last event was NOT IN_MOVED_TO.
// i.e. we're doing the "act as in IN_CREATE, moreover copy the data" instruction from the project
void handleIN_MOVED_TO(tree<Node>::pre_order_iterator it, tree<Node> *backupTree, tree<Node> *sourceTree, char* modifiedFileName, char* sourceRoot, char* backupRoot){

}