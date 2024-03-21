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
        if (tfs_readByte(fd, &file_contents[i]) == -1) {
            printf("Cannot read file contents.\n");
            return;
        }
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
    int oneBlockFD = createFile("1Block", 540);

    int testFD = findFileDescriptorByName("1Block");
    printf("actual FD : %d, guessed FD: %d\n", oneBlockFD, testFD);
    //debug_print_filesystem();

    tfs_makeRO("1Block");

    //debug_write_fileblocks();
    //print_file_contents("1Block", oneBlockFD, 20);

    //printf("\n\nRenaming file...\n");
    int twoBlockFD = createFile("2Block", 400);

    /* testing renaming file from "1Block" to "Block" */
    tfs_rename(oneBlockFD, "Block");

    /* checking file metadata */
    tfs_readFileInfo(twoBlockFD);
    tfs_readFileInfo(oneBlockFD);

    print_file_contents("Block", oneBlockFD, 540);

    /* testing overwriting a file with multiple allocated blocks */
    char overwrite[75] = "this is overwriting!";
    tfs_writeFile(oneBlockFD, overwrite, sizeof(overwrite));

    print_file_contents("Block", oneBlockFD, 75);
    print_file_contents("2Block", twoBlockFD, 400);
    //debug_print_filesystem();

    /* testing reading the directory from the root */
    tfs_readdir();

    tfs_closeFile(oneBlockFD);

    /* testing for writing to closed file -- should error */
    char overwrite2[75] = "overwriting the file again!";
    tfs_writeFile(oneBlockFD, overwrite2, sizeof(overwrite2));

    /* testing for reading file info of a closed file -- should erorr */
    tfs_readFileInfo(oneBlockFD);
    print_file_contents("Block", oneBlockFD, 75);

    /* testing readdir after closing a file -- should still show the file in directory */
    tfs_readdir();
    
    /* attempting to create a file with size greater than possible -- should error */
    int threeBlockFD = createFile("NewFile", 10000);
    printf("threeBlockFD: %d\n", threeBlockFD);

    tfs_readdir();

    tfs_closeFile(threeBlockFD);
    tfs_closeFile(oneBlockFD);

    /* testing that readdir works despite all files being closed */
    tfs_readdir();

    /* re-opening an existing file -- should allow us to read its previous contents */
    twoBlockFD = tfs_openFile("2Block");
    print_file_contents("2Block", twoBlockFD, 400);

    /* overwriting the data that was in 2Block before closing/opening */
    char writingStuff[1000] = "This should overwrite what 2Block had.";
    tfs_writeFile(twoBlockFD, writingStuff, sizeof(writingStuff));

    print_file_contents("2Block", twoBlockFD, 1000);

    testFD = findFileDescriptorByName("2Block");
    printf("actual FD : %d, guessed FD: %d\n", twoBlockFD, testFD);
    testFD = findFileDescriptorByName("Block");
    printf("actual FD : %d, guessed FD: %d\n", oneBlockFD, testFD);

    /* testing unmounting the system */
    tfs_closeFile(twoBlockFD);
    tfs_unmount();

    tfs_readdir();
    twoBlockFD = tfs_openFile("2Block");


    /* testing re-mounting the same disk */
    tfs_mount(filename);

    tfs_readdir();

    /* writing to file after unmounting -- should error */
    char moreWritingStuff[1000] = "I had a tasty burrito today!";
    tfs_writeFile(1, moreWritingStuff, sizeof(moreWritingStuff));

    //sleep(4);
    /* opening + writing to file in file system after unmount-mount + testing creation time */
    oneBlockFD = tfs_openFile("Block");
    printf("\toneBlockFD: %d\n", oneBlockFD);
    tfs_readFileInfo(oneBlockFD);
    print_file_contents("Block", oneBlockFD, 75);

    /* testing creation + modification time after re-opening */
    //sleep(10);
    oneBlockFD = tfs_openFile("Block");
    printf("\toneBlockFD: %d\n", oneBlockFD);
    tfs_writeFile(oneBlockFD, moreWritingStuff, sizeof(moreWritingStuff));
    print_file_contents("Block", oneBlockFD, sizeof(moreWritingStuff));
    tfs_readFileInfo(oneBlockFD);

    /* testing access time from reading */
    //sleep(10);
    print_file_contents("Block", oneBlockFD, sizeof(moreWritingStuff));
    tfs_readFileInfo(oneBlockFD);
    //tfs_readFileInfo(twoBlockFD);
    //tfs_readFileInfo(threeBlockFD);

    /* testing access time from renaming */
    //sleep(5);
    tfs_rename(oneBlockFD, "NewName");
    tfs_readFileInfo(oneBlockFD);

    twoBlockFD = tfs_openFile("2Block");
    printf("\ttwoBlockFD: %d\n", twoBlockFD);
    tfs_closeFile(twoBlockFD);
    tfs_rename(twoBlockFD, "NewName2");
    tfs_readFileInfo(twoBlockFD);

    // tfs_defrag();

    // tfs_displayFragments();

    /* Testing find file descriptor by name */
    testFD = findFileDescriptorByName("NewName");
    printf("actual FD : %d, guessed FD: %d\n", oneBlockFD, testFD);

    /* and finishing the tests */
    tfs_closeFile(oneBlockFD);
    tfs_unmount();

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
