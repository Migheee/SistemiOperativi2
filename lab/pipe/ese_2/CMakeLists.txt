cmake_minimum_required(VERSION 3.20)

project(pipe_ese_2 VERSION 1.0)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/inc)

add_executable(ese_2 src/consumer.c src/producer.c src/errExit.c src/main.c)