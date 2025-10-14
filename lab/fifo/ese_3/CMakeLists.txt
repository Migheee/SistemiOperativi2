cmake_minimum_required(VERSION 3.20)

project(fifo_ese_3 VERSION 1.0)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/inc)

add_executable(client src/client.c src/errExit.c)
add_executable(server src/server.c src/errExit.c)