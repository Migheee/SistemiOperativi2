cmake_minimum_required(VERSION 3.20)

project(msgq_ese_2 VERSION 1.0)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/inc)

add_executable(client src/client.c src/errExit.c src/order.c)
add_executable(server src/server.c src/errExit.c src/order.c)