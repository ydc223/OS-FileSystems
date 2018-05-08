File Mirror system written by Shantanu Bhatia and Yana Chala

Tu run:
make clean && make
./mymirror [ROOT DIRECTORY NAME] [NAME YOU WANT BACKUP DIRECTORY TO TAKE]

We use an external library tree.hh for the actual creation of the tree. For namefile and directory data, we store nodes in this tree which point to Inode structs. These Inode structs contain the statbuf data, as well as the number of hard links to that inode and all the names which hardlink to it. 

Initially, we create a directory tree by traversing the directory we have to watch, creating a node from every directory or file name. If the inode pointed to by a filename in the source has already been stored and is linked to by another Node, we increment the number of hard links on that Inode, store the name of this new hardlinking node, and make the Node point to that Inode.

Once the tree for the source directory has been created, we sync it with the backup folder. We do this by traversing the source tree and creating corresponding nodes. If it is a directory, we create a copy directory. If we are backing up a file that links to other files in source, we check if the files it links to exist yet in backup, and if they do we link to it and update the linkage metadata in the Inode struct. 

For handling INOTIFY events, we run an infinite loop reading event data, and  create event handlers for each of the events we have to watch. For each event, we find the relevant nodes in the Source and Backup trees, and act on the Inodes they point to accordingly. Since IN_DELETE_SELF is never being generated on the target SSH machine (as confirmed by Yana with Nabil), we take care of the deletion of a directory, hypothetically, in IN_DELETE.