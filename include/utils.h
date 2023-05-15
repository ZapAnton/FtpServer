#pragma once
#include "config.h"
#include "user.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

enum CompressorType {
    NONE,
    GZIP,
    BZIP2,
};

enum ArchiveOperation {
    CREATE,
    EXTRACT,
};

void send_response(int client_socket, const char* response);

int establish_data_connection(struct user* current_user);

bool is_dir(const char* const path);

bool file_exists(const char* const filepath);

void make_archive_operation(char* dirpath, char* archive_filepath,
                            const enum CompressorType compressor_type,
                            enum ArchiveOperation archive_operation);

void unpack_archive(char* archive_filepath, enum CompressorType compressor_type);

void transfer_file(const struct user* current_user, const int data_socket,
                   const char* const filepath);

void save_file(const struct user* current_user, const int data_socket, const char* const filepath);

void transfer_dir(const struct user* current_user, const int data_socket, char* dirpath,
                  enum CompressorType compressor_type, const struct Config* config);

char* format_perms(mode_t mode);

char* format_time(time_t mtime);

void get_absolute_path(char* relative_path, char* absolute_path, char* current_directory);

int parse_config_file(struct Config* config);

size_t get_cpu_count(void);

bool str_ends_with(const char* str, const char* suffix);

int rename_mutex(const char* old_name, const char* new_name);

int mkdir_mutex(const char* pathname, mode_t mode);

int rmdir_mutex(const char* pathname);
