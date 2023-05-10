#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "user.h"
#include "config.h"

void send_response(int client_socket, const char* response);

int establish_data_connection(struct user* current_user, int* data_socket);

bool is_dir(const char* const path);

bool file_exists(const char* const filepath);

void make_archive(char* dirpath, char* archive_filepath);


void transfer_file(const struct user* current_user, const int data_socket, const char* const filepath);

void transfer_dir(const struct user* current_user, const int data_socket, char* dirpath, const Config* config);

char* format_perms(mode_t mode);

char* format_time(time_t mtime);

void get_absolute_path(char* relative_path, char* absolute_path, char* current_directory);
