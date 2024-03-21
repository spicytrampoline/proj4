#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>   // for timestamps
#include "libDisk.h" // Include the disk emulator library
#include "tinyFS.h"

FileTableEntry fileTable[FILE_TABLE_SIZE]; // file table to track open files

//I think the table needs to be dynamic, not a fixed size. I'm not sure how to implement this at the moment.
int nextFileDescriptor = 0; // next available file descriptor

static int mounted = 0; // flag to indicate whether a file system is mounted
static int mounted_disk = -1; // variable to store the disk number of the mounted file system

char superblock[BLOCKSIZE] = {0};

/* Free blocks - LIFO ish */
/* root inode is the head of a list of inodes */

/* Linked list, always adds to the front. Superblock points to the head. */
void add_free_block(int disk, int block_num) {
    char freeBlock[BLOCKSIZE] = {0}; // empty block filled with 0s
    freeBlock[_BLOCK_TYPE] = 4;      // free block
    freeBlock[_MAGIC_NUMBER] = 0x44;
    freeBlock[_BLOCK_POINTER] = superblock[_FREE_BLOCK_HEAD];
    writeBlock(disk, block_num, freeBlock);

    superblock[_FREE_BLOCK_HEAD] = block_num;
    superblock[_NUM_FREE_BLOCKS]++;

    //printf("free block added: free block next is %d superblock head is %d\n", freeBlock[_BLOCK_POINTER], superblock[_FREE_BLOCK_HEAD]);
}

/* Linked list, always takes from front. Superblock points to the head. */
int get_free_block() {
    int free_block = superblock[_FREE_BLOCK_HEAD];
    if (free_block == -1) {
        printf("No free blocks available.\n");
        return -1; // failure (no free blocks available) so return neg
    }
    char freeBlock[BLOCKSIZE] ;
    readBlock(mounted_disk, free_block, &freeBlock);

    superblock[_FREE_BLOCK_HEAD] = freeBlock[_BLOCK_POINTER];
    superblock[_NUM_FREE_BLOCKS]--;

    
    return free_block;
}


/* This func used to delete a file's associated data blocks*/
void release_file_chain(int head)
{
    char freeBlock[BLOCKSIZE] = {0}; // empty block filled with 0s
    int walk = head;
    while (walk != -1)
    {
        readBlock(mounted_disk, walk, freeBlock);
        int prev = walk;
        walk = freeBlock[_BLOCK_POINTER];
        add_free_block( mounted_disk, prev);
    }
}

/* Walk file table array to find a free spot */
int getFileDescriptor() {
    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        if (fileTable[i].inodeBlock == -1) {
            return i;
        }
    }

    printf("File table is full.\n");
    return -1;
}


/* Releases index in use. */
void releaseFileDescriptor(int FD)
{
    fileTable[FD].inodeBlock = -1;
    fileTable[FD].filePointer = -1;

}


/* Inits the file system. opens file, makes superblock, */
int tfs_mkfs(char *filename, int nBytes) {
    // check if nBytes is valid
    if (nBytes < BLOCKSIZE) {
        return -1; // failure (nBytes should be at least BLOCKSIZE) so return neg
    }
    
    // open the disk file using the disk emulator library
    int disk = openDisk(filename, nBytes);
    if (disk == -1) {
        return -1; // failure (unable to open disk file) so return neg
    }
    
    
    superblock[_BLOCK_TYPE] = 1;        // byte 0
    superblock[_MAGIC_NUMBER] = 0x44;   // byte 1
    superblock[_ROOT_INODE_BLOCK] = -1; // byte 4 
    superblock[_FREE_BLOCK_HEAD] = -1;  // byte 8 head ptr to list of free blocks
    superblock[_NUM_FREE_BLOCKS] = 0;   // byte 12 - num of free blocks available
    superblock[_TOTAL_BLOCKS] = nBytes / BLOCKSIZE; // byte 16 - 40 blocks
    
    // adding free blocks (in the beginning, 39)
    for (size_t i = superblock[_TOTAL_BLOCKS] - 1; i >= 1; i--)     // do not want to overwrite superblock
    {
        add_free_block(disk, i);
    }
    
    // write the superblock to the disk
    if (writeBlock(disk, SUPERBLOCK_BLOCK_NUM, &superblock) != 0) {     // TODO &superblock
        closeDisk(disk);
        return -1; // failure (unable to write superblock) so return neg
    }

    printf("superblock info: type: %d magid: %d inode: %d free_head:%d num_free:%d total:%d\n", superblock[_BLOCK_TYPE], 
        superblock[_MAGIC_NUMBER], superblock[_ROOT_INODE_BLOCK], superblock[_FREE_BLOCK_HEAD], superblock[_NUM_FREE_BLOCKS], 
        superblock[_TOTAL_BLOCKS]);
    
    // close disk file (should this be here?)
    closeDisk(disk);
    
    return 0; // success
}



int tfs_mount(char *diskname) {
    if (mounted) {
        printf("A file system is already mounted.\n");
        return -1; // failure (file system already mounted) so return neg
    }

    // open the disk file using libDisk
    int disk = openDisk(diskname, 0);
    if (disk == -1) {
        printf("Failed to open disk.\n");
        return -1; // failure (unable to open disk file) so return neg
    }
    //printf("in mount, opened disk %d\n", disk);

    // read superblock from disk via libDisk    
    if (readBlock(disk, SUPERBLOCK_BLOCK_NUM, &superblock) != 0) {
        closeDisk(disk);
        printf("Failed to read superblock.\n");
        return -1; // failure (unable to read superblock)
    }

    //printf("superblock info: %d %d %d %d %d %s\n", superblock.blockType, superblock.magicNumber, superblock.rootInodeBlock, superblock.freeBlockIndex, superblock.numFreeBlocks, superblock.freeSpace);

    // check if magic number is correct
    if (superblock[_MAGIC_NUMBER] != 0x44) {
        closeDisk(disk);
        printf("Incorrect magic number. Not a TinyFS filesystem.\n");
        return -1; // failure (not a TinyFS filesystem) so return neg
    }

    // update mounted flag and disk number
    mounted = 1;
    mounted_disk = disk;
    //printf("tfs_mount: mounted_disk = %d\n", mounted_disk);

    printf("File system mounted successfully.\n");

    for (int i = 0; i < FILE_TABLE_SIZE; i++) {
        releaseFileDescriptor(i);        
    }

    return 0; // success
}

int tfs_unmount(void) {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted) so return neg
    }

    // Close the disk file
    if (closeDisk(mounted_disk) != 0) {
        printf("Failed to close disk.\n");
        return -1; // failure (unable to close disk)
    }

    // reset mounted flag and disk number
    mounted = 0;
    mounted_disk = -1;

    printf("File system unmounted successfully.\n");

    return 0; // success
}



fileDescriptor tfs_openFile(char *name) {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (name == NULL) {
        printf("Empty name.\n");
        return -1;
    }

    // check if file already exists- need to walk inode list 
    char inode[BLOCKSIZE] = {0};
    int inodeIndex = -1;
    int inode_walk = superblock[_ROOT_INODE_BLOCK];
    while (inode_walk != -1)
    {    // should run through all inodes
        // to find if file already exists, we want to run through each name and compare to our input name
        if (readBlock(mounted_disk, inode_walk, inode) != 0) { //inode and file extent blocks start from block 1
            printf("Failed to read inode block.\n");
            return -1; // failure (unable to read root inode block)
        }
        char temp_name[9] = {0};
        memcpy(&temp_name, &inode[_NAME], 9);
        if (strcmp(temp_name, name) == 0) {     // compare names to see if file alreay exists 
            inodeIndex = inode_walk;
            break;
        }
        inode_walk = inode[_BLOCK_POINTER];     // walk to next inode in list 
    }


    
    if (inodeIndex == -1) {
        // create a new inode for file since it doesn't exist

        // init new inode
        char new_inode[BLOCKSIZE] = {0};
        new_inode[_BLOCK_TYPE] = 2; // inode block type
        new_inode[_MAGIC_NUMBER] = 0x44; // magic number for inode block
        new_inode[_BLOCK_POINTER] = superblock[_ROOT_INODE_BLOCK]; // point to next inode block
        
        memcpy(&(new_inode[_NAME]), name, 8);
        new_inode[_NAME + 8] = '\0';  // end file name in null   
        new_inode[_SIZE]= 0;    // size of data extent
        new_inode[_CONTENT_BLOCK_HEAD] = -1; // no data yet- will be head for data linked list (content blocks)

        time_t current_time = time(NULL);
        char timeBytes[8];
        memcpy(timeBytes, &current_time, sizeof(current_time));

        memcpy(&(new_inode[_CREATION_TIME]), timeBytes, sizeof(timeBytes)); //update creation time
        memcpy(&(new_inode[_LAST_ACCESS_TIME]), timeBytes, sizeof(timeBytes)); // update last access time
        memcpy(&(new_inode[_LAST_MODIFICATION_TIME]), timeBytes, sizeof(timeBytes)); // update last modification time
        
        inodeIndex = get_free_block();        
        //printf("new file: %s, inode: %d\n", name, inodeIndex);
        writeBlock(mounted_disk, inodeIndex, new_inode);

        superblock[_ROOT_INODE_BLOCK] = inodeIndex;        // pop to front of inode list
        writeBlock(mounted_disk, SUPERBLOCK_BLOCK_NUM, superblock); //add updated write block
    }

    // now, inode index pts to whichever inode we found (or created)

    // add entry to file table
    int fileDescriptor = getFileDescriptor();

    fileTable[fileDescriptor].inodeBlock = inodeIndex; // 1 = root inode block
    fileTable[fileDescriptor].filePointer = 0;

    // printf("fileTable inodeBlock: %d\n", fileTable[fileDescriptor].inodeBlock);
    // printf("fileTable filePointer: %d\n", fileTable[fileDescriptor].filePointer);

    // return file descriptor
    return fileDescriptor;
}


int tfs_closeFile(fileDescriptor FD) {
    // Implement closing a file in the TinyFS filesystem
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("The FD is invalid.\n");
        return -1; // failure (Invalid file descriptor)
    }

    //remove entry from file table
    fileTable[FD].inodeBlock = -1;
    fileTable[FD].filePointer = -1;

    return 0; // success
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {    
    // Implement writing to a file in the TinyFS filesystem
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("Invalid file descriptor.\n");
        return -1; // failure (Invalid file descriptor)
    }

    //read in inode from inode block on disk (to get name of the file of FD)
    char inode[BLOCKSIZE];  // make space to read inode inode
    if (readBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1){ //read in inode block using fd table
        printf("Failed to read inode block.\n");
        return -1; // failure (unable to read inode block)
    }

    
    if (inode[_CONTENT_BLOCK_HEAD] != -1) {     // something already here
        release_file_chain(inode[_CONTENT_BLOCK_HEAD]);
    }
    
    int blocks_avail = superblock[_NUM_FREE_BLOCKS];
    int blocks_needed = size / DATA_BLOCK_DATA_SIZE;    // DATA_BLOCK_DATA_SIZE = 252 (subtract the 4bytes needded for metadata)
    if (size % DATA_BLOCK_DATA_SIZE != 0) {
        blocks_needed++;
    }

    if (blocks_needed > blocks_avail) {
        printf("Not enough free blocks to write file.\n");
        return -1; // failure (Not enough free blocks to write file)
    }

    char* current_buffer;// = buffer;


    // Work backwards so we can write each block out and then chain it to prev
    int next_block = -1;
    for (int i = blocks_needed-1; i>=0; i--) {
        char block_data[BLOCKSIZE] = {0};
        block_data[_BLOCK_TYPE] = 3;
        block_data[_MAGIC_NUMBER] = 0x44;
        block_data[_BLOCK_POINTER] = next_block; // need to be able to pt to next block, so need to work bkwrds

        // int bytes_to_copy = (size - i * (BLOCKSIZE - 4)) > (BLOCKSIZE - 4) ? (BLOCKSIZE - 4) : (size - i * (BLOCKSIZE - 4));
        // memcpy(&block_data[4], current_buffer, bytes_to_copy);
        // current_buffer += bytes_to_copy;
        int bytes_to_copy = DATA_BLOCK_DATA_SIZE;
        if (i == blocks_needed - 1) {   // if on last block, we need to use mod. else, assume we write the whole block (252 bytes)
            bytes_to_copy = size % DATA_BLOCK_DATA_SIZE;
        }

        current_buffer = buffer + (i * DATA_BLOCK_DATA_SIZE);   // start writing from correct spot.. need to move block(s) forward. ex. second block will pt at 253rd spot
        memcpy(&block_data[4], current_buffer, bytes_to_copy);  // skip 4 byte offset for metadata
        
        int data_block_idx = get_free_block();  // link new block to file chain
        next_block = data_block_idx;    // save for when we move backwards

        writeBlock(mounted_disk, data_block_idx, block_data);

        // TODO MODIFICATION TIME
        // time_t current_time = time(NULL);
        // char timeBytes[8];
        // memcpy(timeBytes, &current_time, sizeof(current_time));
        // memcpy(&(inode[_LAST_MODIFICATION_TIME]), timeBytes, 8);
        // //memcpy(&(inode[_LAST_MODIFICATION_TIME]), current_time, 8);

    }

    time_t current_time = time(NULL);
    char timeBytes[8];
    memcpy(timeBytes, &current_time, sizeof(current_time));

    memcpy(&(inode[_LAST_MODIFICATION_TIME]), timeBytes, sizeof(timeBytes)); // update last modification time
    memcpy(&(inode[_LAST_ACCESS_TIME]), timeBytes, sizeof(timeBytes)); // update last access time

    // write the updated inode back to disk
    // if (writeBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1) {
    //     printf("Failed to write inode block.\n");
    //     return -1; // failure (unable to write inode block)
    // }

    writeBlock(mounted_disk, SUPERBLOCK_BLOCK_NUM, superblock); //add updated write block
    
    // int inode_offset = fileTable[FD].inodeIndex; not using anymore
    int* inodeSize = (int*)&inode[_SIZE];   // int ptr fakes 4 bytes
    *inodeSize = size;

    inode[_CONTENT_BLOCK_HEAD] = next_block;    // set ptr to last block we wrote (first content block)
    writeBlock(mounted_disk, fileTable[FD].inodeBlock, inode);

    fileTable[FD].filePointer = 0;  
    return 0;   // added to compile TODO change
}

int tfs_deleteFile(fileDescriptor FD) {
    // Implement deleting a file in the TinyFS filesystem
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("Invalid file descriptor.\n");
        return -1; // failure (Invalid file descriptor)
    }

    //read in inode from inode block on disk 
    char inode[BLOCKSIZE];
    if (readBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1){ //read in inode block
        printf("Failed to read inode block.\n");
        return -1; // failure (unable to read inode block)
    }

    // Free all the blocks that the file takes up
    release_file_chain(inode[_CONTENT_BLOCK_HEAD]);

    // We need to free the INODE block for this file
    int next_inode = inode[_BLOCK_POINTER];
    if (superblock[_ROOT_INODE_BLOCK] == fileTable[FD].inodeBlock) {
        superblock[_ROOT_INODE_BLOCK] = next_inode;
    } else {
        int inode_walk = superblock[_ROOT_INODE_BLOCK];
        while (inode_walk != -1)
        {
            if (readBlock(mounted_disk, inode_walk, inode) == -1){ //read in inode block
                printf("Failed to read inode block.\n");
                return -1; // failure (unable to read inode block)
            }

            if (inode[_BLOCK_POINTER] == fileTable[FD].inodeBlock) {
                inode[_BLOCK_POINTER] = next_inode;
                writeBlock(mounted_disk, inode_walk, inode);
                break;
            }
            inode_walk = inode[_BLOCK_POINTER];
        }
    }
    add_free_block(mounted_disk, fileTable[FD].inodeBlock);

    
    //update superblock
    writeBlock(mounted_disk, SUPERBLOCK_BLOCK_NUM, superblock); //add updated write block


    // Release file table entry
    releaseFileDescriptor(FD);
    
    return 0; // TODO added to compile
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    // Implement reading a byte from a file in the TinyFS filesystem
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("Invalid file descriptor.\n");
        return -1; // failure (Invalid file descriptor)
    }
    
    char inode[BLOCKSIZE];  // ptr to inode block
    
    if (readBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1){ //read in inode block
        printf("Failed to read inode block.\n");
        return -1; // failure (unable to read inode block)
    }

    int *size_ptr = (int*)&inode[_SIZE];
    int size = *size_ptr;

    int offset = fileTable[FD].filePointer;

    if (offset > size) {
        printf("End of file reached.\n");
        return -1;
    }

    int block_num = offset / DATA_BLOCK_DATA_SIZE ;

    char block[BLOCKSIZE];
    int nextBlock = inode[_CONTENT_BLOCK_HEAD];
    for (int i = 0; i <= block_num; i++) {
        if (readBlock(mounted_disk, nextBlock, block) != 0) {
            printf("Failed to read block.\n");
            return -1; // failure (unable to read block)
        }
        nextBlock = block[_BLOCK_POINTER];
    }

    time_t current_time = time(NULL);
    char timeBytes[8];
    memcpy(timeBytes, &current_time, sizeof(current_time));
    memcpy(&(inode[_LAST_ACCESS_TIME]), timeBytes, sizeof(timeBytes)); // update last access time

    if (writeBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1) {
        printf("Error updating inode block with access time in function tfs_readByte.\n");
        return -1;
    }

    int blockOffset = offset - (block_num * DATA_BLOCK_DATA_SIZE);
    *buffer = block[4+blockOffset];
    fileTable[FD].filePointer++;
    return 0;
    
    //I'm not sure if this is the correct way to read the file

}

int tfs_seek(fileDescriptor FD, int offset) {
    // Implement seeking within a file in the TinyFS filesystem
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }

    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("Invalid file descriptor.\n");
        return -1; // failure (Invalid file descriptor)
    }
    fileTable[FD].filePointer = offset;
    return 0; // success
}

// DEBUGGING
int tfs_get_mounted_disk( ) {
    return mounted_disk;
}

void debug_print_file_chain(int startingBlock)
{
    char block[BLOCKSIZE];
    int block_idx = startingBlock;
    int max_walk = 1;
    while (block_idx != -1 && max_walk < 100) {
        
        readBlock(mounted_disk, block_idx, block);
        printf("->%d", block_idx);
        block_idx = block[_BLOCK_POINTER];
        max_walk++;
    }
    printf("\n");
}

void debug_print_freechain()
{
    char superblock[BLOCKSIZE];
    int rb = readBlock(mounted_disk, 0, superblock);
    if (rb == -1) {
        printf("Failed to read superblock.\n");
        return; // failure (unable to read superblock)
    }
    printf("--------------------    Superblock & File Info    --------------------\n");
    printf("num free blocks: %d\n", superblock[_NUM_FREE_BLOCKS]);
    printf("free block chain: ");
    debug_print_file_chain(superblock[_FREE_BLOCK_HEAD]);

}



void debug_print_files()
{
    char superblock[BLOCKSIZE];
    int rb = readBlock(mounted_disk, 0, superblock);
    if (rb == -1) {
        printf("Failed to read superblock.\n");
        return; // failure (unable to read superblock)
    }
    printf("iNode chain: ");
    debug_print_file_chain(superblock[_ROOT_INODE_BLOCK]);

    char inodeBlock[BLOCKSIZE];
    int inode_block_idx = superblock[_ROOT_INODE_BLOCK];
    int max_walk = 1;
    while (inode_block_idx != -1 && max_walk < 100) {
        
        readBlock(mounted_disk, inode_block_idx, inodeBlock);

        char name[9] = {0};
        memcpy(name, &(inodeBlock[_NAME]), 9);
        int* inodeSize = (int*)&inodeBlock[_SIZE];

        printf("   filename: %s, size: %d\n", name, *inodeSize);
        printf("        Data block chain: ");
        debug_print_file_chain(inodeBlock[_CONTENT_BLOCK_HEAD]);
        // print_file_contents(inodeBlock[_CONTENT_BLOCK_HEAD], *inodeSize);



        inode_block_idx = inodeBlock[_BLOCK_POINTER];
        max_walk++;
    }
    printf("\n");

}

void debug_write_fileblocks()
{
    char superblock[BLOCKSIZE];
    int rb = readBlock(mounted_disk, 0, superblock);
    if (rb == -1) {
        printf("Failed to read superblock.\n");
        return; // failure (unable to read superblock)
    }
    // printf("------ Printing Files ------\n");
    // printf("iNode head: %d\n", superblock[_ROOT_INODE_BLOCK]);
    //debug_print_file_chain(superblock[_ROOT_INODE_BLOCK]);

    char inodeBlock[BLOCKSIZE];
    int inode_block_idx = superblock[_ROOT_INODE_BLOCK];
    int max_walk = 1;
    while (inode_block_idx != -1 && max_walk < 100) {
        
        readBlock(mounted_disk, inode_block_idx, inodeBlock);

        char name[9] = {0};
        memcpy(name, &(inodeBlock[_NAME]), 9);
        int* inodeSize = (int*)&inodeBlock[_SIZE];

        printf("Writing blocks for file %s, size: %d ", name, *inodeSize);


        char block[BLOCKSIZE];
        int block_idx = inodeBlock[_CONTENT_BLOCK_HEAD];
        int max_walk = 1;
        while (block_idx != -1 && max_walk < 100) {
            
            readBlock(mounted_disk, block_idx, block);
            printf("->%d", block_idx);

            char filename[11] = {0};
            memcpy(filename, "block ", 6);
            char block_num[4];
            sprintf(block_num, "%d", block_idx);
            memcpy(&filename[5], block_num, 3);

            int flags = O_RDWR | O_CREAT;
            int fd = open(filename, flags, S_IRUSR | S_IWUSR);
            write(fd, block, BLOCKSIZE);
            close(fd);


            block_idx = block[_BLOCK_POINTER];
            max_walk++;
        }
        printf("\n");



        inode_block_idx = inodeBlock[_BLOCK_POINTER];
        max_walk++;
    }
    printf("\n");
}

void debug_print_filesystem()
{
    debug_print_freechain();
    debug_print_files();

}



/* EXTRA CREDIT OPTION B*/

/* This func changes new name*/
int tfs_rename(fileDescriptor FD, char* newName) {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }
    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("The FD is invalid.\n");
        return -1; // failure (Invalid file descriptor)
    }
    if (sizeof(newName) > 8) {
        printf("The newname is too long.\n");
        return -1;
    }

    char inode[BLOCKSIZE];
    if (readBlock(mounted_disk, fileTable[FD].inodeBlock, &inode) == -1) {
        printf("Unable to access file inode. Make sure file is open.\n");
        return -1;
    }

    memset(&(inode[_NAME]), '\0', 9); // set MEMORY to null for 9 spots . to blank out old name
    strcpy(&(inode[_NAME]), newName);   // copy new line in with null ending

    time_t current_time = time(NULL);
    char timeBytes[8];
    memcpy(timeBytes, &current_time, sizeof(current_time));
    memcpy(&(inode[_LAST_ACCESS_TIME]), timeBytes, sizeof(timeBytes)); // update last access time
    memcpy(&(inode[_LAST_MODIFICATION_TIME]), timeBytes, sizeof(timeBytes)); // update ast modification time

    writeBlock(mounted_disk, fileTable[FD].inodeBlock, &inode);

    return 0;
}

/* We did not implement hierarchy, so we just need to walk the inodes and print the filenames. */
void tfs_readdir() {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }
    int walk = superblock[_ROOT_INODE_BLOCK];    // ptr to inodes
    char inode[BLOCKSIZE];
    printf("\nDisplaying files from root directory: \n");
    while (walk != -1) {
        readBlock(mounted_disk, walk, &inode);
        printf("%s\n", &(inode[_NAME]));

        walk = inode[_BLOCK_POINTER];
    }
    printf("\n");
}


/* EXTRA CREDIT OPTION E */
int tfs_readFileInfo(fileDescriptor FD) {
    // Check if mounted
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return -1; // failure (no file system mounted)
    }
    // Check fd valid
    if (FD < 0 || FD >= FILE_TABLE_SIZE) {
        printf("The FD is invalid.\n");
        return -1; // failure (Invalid file descriptor)
    }
    
    // get inode
    char inode[BLOCKSIZE];
    if (readBlock(mounted_disk, fileTable[FD].inodeBlock, inode) == -1) {
        printf("Unable to access file contents.\n");
        return -1;
    }
    printf("File: %s\n", &(inode[_NAME]));

    // convert and print creation time
    time_t creation_time;
    memcpy(&creation_time, &(inode[_CREATION_TIME]), sizeof(time_t));
    printf("\tCreation: %s", asctime(localtime(&creation_time)));

    // convert and print last access time
    time_t access_time;
    memcpy(&access_time, &(inode[_LAST_ACCESS_TIME]), sizeof(time_t));
    printf("\tAccess: %s", asctime(localtime(&access_time)));

    // convert and print last modification time
    time_t modification_time;
    memcpy(&modification_time, &(inode[_LAST_MODIFICATION_TIME]), sizeof(time_t));
    printf("\tModify: %s", asctime(localtime(&modification_time)));

    return 0;
}

void tfs_displayFragments() {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return;
    }

    printf("Fragmentation Map:\n");

    int block_num = superblock[_FREE_BLOCK_HEAD];
    while (block_num != -1) {
        printf("Block %d: Allocated\n", block_num);
        char block[BLOCKSIZE];
        if (readBlock(mounted_disk, block_num, block) == -1) {
            printf("Failed to read block %d\n", block_num);
            return;
        }
        block_num = block[_BLOCK_POINTER];
    }

    printf("End of Fragmentation Map\n");
}

void tfs_defrag() {
    if (!mounted) {
        printf("No file system is currently mounted.\n");
        return;
    }

    printf("Defragmenting filesystem...\n");

    // Initialize variables
    int current_block = superblock[_FREE_BLOCK_HEAD];
    int last_free_block = -1;
    int last_allocated_block = -1;

    // Traverse the free block list
    while (current_block != -1) {
        char block[BLOCKSIZE];
        if (readBlock(mounted_disk, current_block, block) == -1) {
            printf("Failed to read block %d\n", current_block);
            return;
        }

        // Move the block to the end of the disk
        if (last_free_block != -1) {
            if (writeBlock(mounted_disk, last_free_block, block) == -1) {
                printf("Failed to write block %d\n", last_free_block);
                return;
            }
        }

        // Update block pointers
        if (last_allocated_block != -1) {
            char last_allocated_block_data[BLOCKSIZE];
            if (readBlock(mounted_disk, last_allocated_block, last_allocated_block_data) == -1) {
                printf("Failed to read block %d\n", last_allocated_block);
                return;
            }
            last_allocated_block_data[_BLOCK_POINTER] = last_free_block;
            if (writeBlock(mounted_disk, last_allocated_block, last_allocated_block_data) == -1) {
                printf("Failed to write block %d\n", last_allocated_block);
                return;
            }
        }

        // Update the free block list
        last_free_block = current_block;
        current_block = block[_BLOCK_POINTER];

        // Update last allocated block
        last_allocated_block = last_free_block;
    }

    // Update superblock with new head of free block list
    superblock[_FREE_BLOCK_HEAD] = last_free_block;
    superblock[_NUM_FREE_BLOCKS] = 1; // Only one free block at the end
    writeBlock(mounted_disk, SUPERBLOCK_BLOCK_NUM, superblock);

    printf("Defragmentation completed.\n");
}
