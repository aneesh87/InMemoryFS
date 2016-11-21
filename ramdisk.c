/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


#define MAX_NAME 512

//Globals

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

typedef struct __data {
	char name[MAX_NAME];
	int  isdir;
	struct stat st;
	int data_size;
} Ndata;

typedef struct element {
	Ndata data;
	char * filedata;
	struct element * parent;
	struct element * firstchild;
	struct element * next;
} Node;

long freememory;
Node * Root;


int allocate_node(Node ** node) {

	if (freememory < sizeof(Node)) {
		return -ENOSPC;
	}

	*node = calloc(1, sizeof(Node));
	if (*node == NULL) {
		return -ENOSPC;
	} else {
		freememory = freememory - sizeof(Node);
		return 0;
	}
}
int check_path(const char * path, Node ** n) {

	char temp[MAX_NAME];
	strncpy(temp, path, MAX_NAME);

	if(strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
	   *n = Root;
	   return 1;		
	}

	char * ele = strtok(temp, "/");
	Node * parent = Root;
	Node * childptr = NULL;
	while (ele != NULL) {
		int found = 0;
		childptr = parent->firstchild;
		while (childptr != NULL) {
			if(strcmp(childptr->data.name, ele) == 0) {
				found = 1;
				break;
			}
			childptr = childptr->next;
		}
		if (!found) {*n = NULL; return 0;}

		ele = strtok(NULL, "/");
		parent = childptr;
	}
	*n = childptr;
	return 1;
}

static int ram_getattr(const char *path, struct stat *stbuf)
{

	Node *t = NULL;
	int valid = check_path(path, &t);
	if (!valid) {
		return -ENOENT;
	} else {
		*stbuf = t->data.st;
		return 0;
	}
}


static int ram_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
    
    time_t T;
    time(&T);

    Node * parent = NULL;
	
	int valid = check_path(path, &parent);
	if (!valid) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	Node * temp = NULL;

	for(temp = parent->firstchild; temp; temp = temp->next) {
		filler(buf, temp->data.name, NULL, 0);
	}
	parent->data.st.st_atime = T;


	return 0;
}

static int ram_open(const char *path, struct fuse_file_info *fi)
{
	FILE *fp;
	fp = fopen("/home/agupta27/log.txt","a+");
	fprintf(fp, "%s\n", "open");
	fprintf(fp, "%s\n", path);
	fclose(fp);
	
	Node *p= NULL;
	int valid = check_path(path, &p);
	if (!valid) {
		return -ENOENT;
	}
	return 0;
}

static int ram_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	FILE *fp;
	fp = fopen("/home/agupta27/log.txt","a+");
	
	fprintf(fp, "%s\n", "read");
	fprintf(fp, "%s\n", path);
	fclose(fp);

	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

void init_for_dir(Node * newchild, char * dname) {
	
	newchild->data.isdir = 1;
	strcpy(newchild->data.name, dname);

	newchild->data.st.st_nlink = 2;   // . and ..
	newchild->data.st.st_uid = getuid();
	newchild->data.st.st_gid = getgid();
	newchild->data.st.st_mode = S_IFDIR |  0755; //755 is default directory permissions

	newchild->data.st.st_size = 4096;

	time_t T;
	time(&T);

	newchild->data.st.st_atime = T;
	newchild->data.st.st_mtime = T;
	newchild->data.st.st_ctime = T;
}

static int ram_mkdir(const char *path, mode_t mode) {

	Node *parent = NULL;
	int valid = check_path(path, &parent);

	if(valid) {
		return -EEXIST;
	}

	char * ptr = strrchr(path, '/');
	char tmp[MAX_NAME];
	strncpy(tmp, path, ptr - path);
    tmp[ptr - path] = '\0';

	valid = check_path(tmp, &parent);
	if (!valid) {
		return -ENOENT;
	}
	Node * newchild = NULL;
	int ret = allocate_node(&newchild);

	if(ret != 0) {
		return ret;
	}

    ptr++;
    init_for_dir(newchild, ptr);
	
	parent->data.st.st_nlink = parent->data.st.st_nlink + 1;

	newchild->parent = parent;
	newchild->next = parent->firstchild;
	parent->firstchild = newchild;

	return 0;
}

static struct fuse_operations hello_oper = {
	.getattr	= ram_getattr,
	.readdir	= ram_readdir,
	.open		= ram_open,
	.read		= ram_read,
	.mkdir		= ram_mkdir,
};

int main(int argc, char *argv[])
{
	FILE *fp;
	fp = fopen("/home/agupta27/log.txt","w+");
	fclose(fp);

	//for extra credit
	char filedump[MAX_NAME];

	if (argc < 3) {
		fprintf(stderr, "ramdisk:Too few arguments\n");
		fprintf(stderr, "ramdisk <mount_point> <size>");
		return -1;
	}

	if (argc > 4) {
		fprintf(stderr, "ramdisk:Too many arguments\n");
		fprintf(stderr, "ramdisk <mount_point> <size> [<filename>]");
		return -1;
	}

	if (argc == 4) {
		strncpy(filedump, argv[3], MAX_NAME);
		argc--;
	}

	freememory = atol(argv[2]) * 1024 * 1024;
	if (freememory <= 0) {
		fprintf(stderr, "Invalid Memory Size\n");
		return -1;
	}

	//initialize the root


	Root = (Node *)calloc(1, sizeof(Node));
	strcpy(Root->data.name, "/");
	Root->data.isdir = 1;
	Root->data.st.st_nlink = 2;   // . and ..
	Root->data.st.st_uid = 0;
	Root->data.st.st_gid = 0;
	Root->data.st.st_mode = S_IFDIR |  0755; //755 is default directory permissions

	time_t T;
	time(&T);

	Root->data.st.st_size = 4096;
	Root->data.st.st_atime = T;
	Root->data.st.st_mtime = T;
	Root->data.st.st_ctime = T;
	freememory = freememory - sizeof(Node); 

	return fuse_main(argc-1, argv, &hello_oper, NULL);
}
