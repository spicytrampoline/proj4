#include <stdio.h>
#include "tinyFS.h"

int createFile(char* filename, int fileSize) {
    int fd = tfs_openFile(filename);
    if (fd == -1) {
        printf("Failed to open file.\n");
        return 1;
    }

    char data[fileSize];
    memset(data, '_', sizeof(data));
    // Copy "Starting data...<filename>" into data
    memcpy(data, "Starting data...", 15);
    memcpy(data + 9, filename, strlen(filename));

    // Copy "Data End" to end of data, including /0 end of string
    strcpy(data + sizeof(data) - 9, "Data End");


    int result = tfs_writeFile(fd, data, sizeof(data));
    if (result != 0) {
        printf("Failed to write data to the file.\n");
        return 1;
    }

    return fd;
}

void deleteFile(int fd) {
    int result = tfs_deleteFile(fd);
    if (result != 0) {

        printf("Failed to delete file.\n");
    }
}

void print_file_contents(char* filename, int fd, int size)
{
    char file_contents[size];

    for (size_t i = 0; i < size; i++)
    {
        tfs_readByte(fd, &file_contents[i]);
    }
    
    printf("   %s file contents: %s\n", filename, file_contents);
}


//********************************************************************************
//  Testing Helpers
//  - createFile - ask the tfs to create a file of a certain size
//      - format of file will be "Starting <filename>____Data End" with 
//        underscores filling the rest of the space
//  - deleteFile - ask the tfs to delete a file
//  - print_file_contents - ask the tfs to read the file and print the contents
//  - debug_print_filesystem - ask tfs to printout free block chain, inode block 
//      chain, and data block chain
//  - debug_write_fileblocks - ask tfs to write all the file data blocks to
//      files "block0x" for inspection.
//
//********************************************************************************
int main() {
    char* filename = "tinyFSDisk"; // file name for the disk
    int diskSize = DEFAULT_DISK_SIZE; 
    //fileDescriptor fd;
    int result;

    // create a TinyFS file system
    printf("Creating TinyFS file system...\n");
    result = tfs_mkfs(filename, diskSize);
    if (result == 0) {
        printf("TinyFS file system created successfully.\n\n");
    } else {
        printf("Failed to create TinyFS file system.\n");
        return 1;
    }

    // mount the TinyFS file system
    printf("\n\nMounting TinyFS file system...\n");
    result = tfs_mount(filename);
    if (result == 0) {
        printf("TinyFS file system mounted successfully.\n");
    } else {
        printf("Failed to mount TinyFS file system.\n");
        return 1;
    }

    //debug_print_filesystem();
    int oneBlockFD = createFile("1Block", 20);
    debug_print_filesystem();

    //debug_write_fileblocks();
    print_file_contents("1Block", oneBlockFD, 20);

    printf("\n\nRenaming file...\n");
    tfs_rename(oneBlockFD, "Block");
    debug_print_filesystem();


    //int twoBlockFD = createFile("2Block", DATA_BLOCK_DATA_SIZE + 1);
    //debug_print_filesystem();

    // printf("\n\nAbout to overwrite....\n");
    // tfs_writeFile(oneBlockFD, "SecondMessage!", sizeof("SecondMessage!"));
    //debug_write_fileblocks();
    // print_file_contents("1Block", oneBlockFD, sizeof("SecondMessage!"));

    // int threeBlockSize = DATA_BLOCK_DATA_SIZE * 2 + 2;
    // int threeBlockFD = createFile("3Block", threeBlockSize);

    // printf("\nAfter 1 1-block and 1 3-block file created\n");
    // debug_print_filesystem();

    //print_file_contents("3Block", threeBlockFD, threeBlockSize);

    //deleteFile(oneBlockFD);
    
    //  printf("\n\nAfter 1-block deleted\n");
    //  debug_print_filesystem();

    // int twoBlockSize = DATA_BLOCK_DATA_SIZE  + 1;
    // int twoBlockFD = createFile("2Block", twoBlockSize);

    // printf("\n\nAfter 2-block created-fragmented system\n");
    // debug_print_filesystem();

    //print_file_contents("2Block", twoBlockFD, twoBlockSize);


    // cleanup
    // deleteFile(twoBlockFD);
    // deleteFile(threeBlockFD);

    //debug_print_filesystem();


//     //open a file in the TinyFS file system
//     printf("\n\nOpening and creating 1-block file test.txt...\n");
//     int fd = tfs_openFile("test.txt");
//     if (fd != -1) {
//         printf("File opened successfully with file descriptor: %d\n", fd);
//     } else {
//         printf("Failed to open file.\n");
//         return 1;
//     }

//     // // write data to the file
//     printf("\n\nWriting data to the file...\n");
//     char data[] = "Hello, TinyFS!";
//     result = tfs_writeFile(fd, data, sizeof(data));
//     if (result == 0) {
//         printf("Data written to the file successfully.\n");
//     } else {
//         printf("Failed to write data to the file.\n");
//         return 1;
//     }

//     //debug_print_filesystem();

//     //open another file in the TinyFS file system
//     printf("\n\nOpening another file...\n");
//     fd = tfs_openFile("t2.txt");
//     //printf("AFTER RETURNING\n");
//     if (fd != -1) {
//         printf("File opened successfully with file descriptor: %d\n", fd);
//     } else {
//         printf("Failed to open file.\n");
//         return 1;
//     }

//     // // write data to the file
//     printf("\n\nWriting another file...\n");
// //    char newdata[] = "I love Pepper! I love coding! I love sleeping! I love eating! I love playing! I love reading! I love writing! I love learning! I love teaching! I love everything! I love life! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love Pepper! I love coding! I love sleeping! I love eating! I love playing! I love reading! I love writing! I love learning! I love teaching! I love everything! I love life! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love Pepper! I love coding! I love sleeping! I love eating! I love playing! I love reading! I love writing! I love learning! I love teaching! I love everything! I love life! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching! I love learning! I love writing! I love reading! I love playing! I love eating! I love sleeping! I love coding! I love Pepper! I love you! I love me! I love us! I love them! I love him! I love her! I love it! I love them! I love us! I love me! I love you! I love life! I love everything! I love teaching!";
//     //char newdata[] = "chmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, and --- for group and other.chmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, anjew";
//     char newdata[] = "chmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, and --- for group and other.chmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, anjewchmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, and --- for group and other.chmod - change permissions can use letters, or short hand 3 numbers for ugo. 700 for example sets rwx for user, anjew";
    
//     result = tfs_writeFile(fd, newdata, sizeof(newdata));
//     if (result == 0) {
//         printf("Data written to the file successfully.\n");
//     } else {
//         printf("Failed to write data to the file.\n");
//         return 1;
//     }

    //debug_print_filesystem();


    // //open a third file in the TinyFS file system
    // printf("\n\nOpening a third file...\n");
    // fd = tfs_openFile("t3.txt");
    // printf("AFTER RETURNING\n");
    // if (fd != -1) {
    //     printf("File opened successfully with file descriptor: %d\n", fd);
    // } else {
    //     printf("Failed to open file.\n");
    //     return 1;
    // }

    // // // write data to the file
    // printf("\n\nWriting another file...\n");
    // char newnewdata[] = "I love coding!";
    // result = tfs_writeFile(fd, newnewdata, sizeof(newnewdata));
    // if (result == 0) {
    //     printf("Data written to the file successfully.\n");
    // } else {
    //     printf("Failed to write data to the file.\n");
    //     return 1;
    // }

    // // write more data to first file
    // // printf("\n\nWriting more data to first file...\n");
    // // char moredata[] = "I love sleeping!";
    // // result = tfs_writeFile(1, moredata, sizeof(moredata));
    // // if (result == 0) {
    // //     printf("Data written to the file successfully.\n");
    // // } else {
    // //     printf("Failed to write data to the file.\n");
    // //     return 1;
    // // }

    // // // close the file
    // // printf("Closing the file...\n");
    // // result = tfs_closeFile(fd);
    // // if (result == 0) {
    // //     printf("File closed successfully.\n");
    // // } else {
    // //     printf("Failed to close the file.\n");
    // //     return 1;
    // // }

    // // // unmount the TinyFS file system
    // // printf("Unmounting TinyFS file system...\n");
    // // result = tfs_unmount();
    // // if (result == 0) {
    // //     printf("TinyFS file system unmounted successfully.\n");
    // // } else {
    // //     printf("Failed to unmount TinyFS file system.\n");
    // //     return 1;
    // // }

    // return 0;
}
