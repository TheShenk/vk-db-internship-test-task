//
// Created by shenk on 04.04.2022.
//

#ifndef VK_DB_INTERNSHIP_TEST_TASK_COMMON_STRUCTS_H
#define VK_DB_INTERNSHIP_TEST_TASK_COMMON_STRUCTS_H

#define MAX_FILENAME_LEN 256

struct init_msg {
    char save_filename[MAX_FILENAME_LEN];
    unsigned int file_size;
};

#endif //VK_DB_INTERNSHIP_TEST_TASK_COMMON_STRUCTS_H
