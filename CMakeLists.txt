cmake_minimum_required(VERSION 3.22)
project(vk_db_internship_test_task C)

set(CMAKE_C_STANDARD 11)

add_executable(server
        server.c
        common_structs.h)

add_executable(client
        client.c
        common_structs.h)
