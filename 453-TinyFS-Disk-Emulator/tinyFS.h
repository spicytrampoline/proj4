#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libDisk.h" // Include the disk emulator library

#define BLOCKSIZE 256
#define DATA_BLOCK_DATA_SIZE 252
#define DEFAULT_DISK_SIZE 10240
#define BLOCK_COUNT (DEFAULT_DISK_SIZE / BLOCKSIZE)
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define SUPERBLOCK_BLOCK_NUM 0

//macros for all blocks
#define _BLOCK_TYPE 0
#define _MAGIC_NUMBER 1 
#define _BLOCK_POINTER 2
//bytes 3 empty

// superblock position macros
#define _ROOT_INODE_BLOCK 4 //int, where the inode blocks start (inodes could be mixed in w data blocks)
#define _FREE_BLOCK_HEAD 8 //int, where the free blocks start
#define _NUM_FREE_BLOCKS  12 //int, total free blocks
#define _TOTAL_BLOCKS 16 //int, total blocks on the disk

//macros for inode
#define _NAME 4 //char[9], file name
#define _SIZE 13 //int, file size
#define _CONTENT_BLOCK_HEAD 17 //int, block number of the first data block
#define _CREATION_TIME 21   // time_t is 8 bytes
#define _LAST_MODIFICATION_TIME 29   // time_t is 8 bytes
#define _LAST_ACCESS_TIME 37   // time_t is 8 bytes



typedef struct {
    int inodeBlock; // block number of the inode block containing this file's inode
    int filePointer; // current position of the file pointer
} FileTableEntry;

#define FILE_TABLE_SIZE 20 // max # of open files

typedef int fileDescriptor;

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *diskname);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);
int tfs_rename(fileDescriptor FD, char* newName);   // extra credit b
void tfs_readdir(); // extra credit b
int tfs_readFileInfo(fileDescriptor FD);    // extra credit e

// TODO Remove these
int tfs_get_mounted_disk( );
void debug_print_freechain();
void debug_print_filesystem();
void debug_write_fileblocks();