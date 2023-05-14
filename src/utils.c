#include "utils.h"

static pthread_mutex_t fs_mutex = PTHREAD_MUTEX_INITIALIZER;

bool str_ends_with(const char* str, const char* suffix) {
    const size_t str_len = strlen(str);
    const size_t suffix_len = strlen(suffix);
    return suffix_len <= str_len && strcmp(str + str_len - suffix_len, suffix) == 0;
}

void send_response(int client_socket, const char* response) {
    int response_len = strlen(response);
    int bytes_sent = send(client_socket, response, response_len, 0);
    if (bytes_sent != response_len) {
        perror("Error sending response");
        exit(EXIT_FAILURE);
    }
}

int establish_data_connection(struct user* current_user) {
    if (current_user->data_connection_type == ACTIVE) {
        current_user->data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(current_user->data_socket, (struct sockaddr*)&(current_user->data_address), sizeof(current_user->data_address)) < 0) {
            send_response(current_user->control_socket, "425 Can't open data connection.\r\n");
            return -1;
        }
    } else {
        socklen_t data_address_length = sizeof(current_user->data_address);
        current_user->client_data_socket = accept(current_user->data_socket, (struct sockaddr*) &current_user->data_address, &data_address_length);
        if (current_user->client_data_socket < 0) {
            send_response(current_user->control_socket, "425 Can't open data connection.\r\n");
            return -1;
        }
    }
    return 0;
}

bool is_dir(const char* const path) {
    bool file_is_dir = false;
    pthread_mutex_lock(&fs_mutex);
    struct stat statbuf = { 0 };
    if (stat(path, &statbuf) != 0) {
        return false;
    }
    file_is_dir = S_ISDIR(statbuf.st_mode);
    pthread_mutex_unlock(&fs_mutex);
    return file_is_dir;
}

bool file_exists(const char* const filepath) {
    bool exists = false;
    pthread_mutex_lock(&fs_mutex);
    exists = access(filepath, F_OK) == 0;
    pthread_mutex_unlock(&fs_mutex);
    return exists;
}

void make_archive_operation(char* dirpath, char* archive_filepath, const enum CompressorType compressor_type, enum ArchiveOperation archive_operation) {
    const size_t command_length = strlen("tar -czf") + 1 + strlen(archive_filepath) + 1 + strlen("-C") + 1 + strlen(dirpath) + 1 + strlen(".");
    char* command = calloc(command_length + 1, sizeof(char));
    const char compressor_flag = (compressor_type == GZIP) ? 'z' : 'j';
    const char operation_flag = (archive_operation == CREATE) ? 'c' : 'x';
    const char extra_char = (archive_operation == CREATE) ? '.' : ' ';
    snprintf(command, command_length + 1, "tar -%c%cf %s -C %s %c", operation_flag, compressor_flag, archive_filepath, dirpath, extra_char);
    puts(command);
    pthread_mutex_lock(&fs_mutex);
    const int status = system(command);
    pthread_mutex_unlock(&fs_mutex);
    if (status != 0) {
        fprintf(stderr, "Archive error: %s\n", strerror(errno));
    }
    free(command);
}

void save_file(const struct user* current_user, const int data_socket, const char* const filepath) {
    pthread_mutex_lock(&fs_mutex);
    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        send_response(current_user->control_socket, "550 Failed to open file.\r\n");
        return;
    }
    size_t n = 0;
    char buf[BUFFER_SIZE] = { 0 };
    while ((n = recv(data_socket, buf, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buf, sizeof(char), n, file) != n) {
            send_response(current_user->control_socket, "550 Failed to write file.\r\n");
            fclose(file);
            return;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}

void transfer_file(const struct user* current_user, const int data_socket, const char* const filepath) {
    pthread_mutex_lock(&fs_mutex);
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file at %s\n", filepath);
        send_response(current_user->control_socket, "550 File unavailable.\r\n");
        return;
    }
    char send_buffer[256] = {0};
    size_t bytes_read = 0;
    while((bytes_read = fread(send_buffer, sizeof(char), sizeof(send_buffer), file)) > 0){
        send(data_socket, send_buffer, bytes_read, 0);
    }
    fclose(file);
    pthread_mutex_unlock(&fs_mutex);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}

void transfer_dir(const struct user* current_user, const int data_socket, char* dirpath, const enum CompressorType compressor_type, const struct Config* config) {
    if (compressor_type == GZIP && !file_exists(config->tar_command_path)) {
        fprintf(stderr, "%s command not found", config->tar_command_path);
        send_response(current_user->control_socket, "550 tar command unavailable.\r\n");
        return;
    } else if (compressor_type == BZIP2 && !file_exists(config->bz2_command_path)) {
        fprintf(stderr, "%s command not found", config->bz2_command_path);
        send_response(current_user->control_socket, "550 bzip2 command unavailable.\r\n");
        return;
    }
    const char* archive_extension = (compressor_type == GZIP) ? ".tar.gz" : ".bz2";
    const size_t archive_filepath_length = strlen(dirpath) + strlen(archive_extension);
    char* archive_filepath = calloc(archive_filepath_length + 1, sizeof(char));
    snprintf(archive_filepath, archive_filepath_length + 1, "%s%s", dirpath, archive_extension);
    if (!file_exists(archive_filepath)) {
        make_archive_operation(dirpath, archive_filepath, compressor_type, CREATE);
    }
    transfer_file(current_user, data_socket, archive_filepath);
    free(archive_filepath);
}

char* format_perms(mode_t mode) {
    char *perms = calloc(11, sizeof(char));
    if (perms == NULL) {
        return NULL;
    }
    strcpy(perms, "---------");
    if (S_ISDIR(mode)) {
        perms[0] = 'd';
    }
    if (mode & S_IRUSR) {
        perms[1] = 'r';
    }
    if (mode & S_IWUSR) {
        perms[2] = 'w';
    }
    if (mode & S_IXUSR) {
        perms[3] = 'x';
    }
    if (mode & S_IRGRP) {
        perms[4] = 'r';
    }
    if (mode & S_IWGRP) {
        perms[5] = 'w';
    }
    if (mode & S_IXGRP) {
        perms[6] = 'x';
    }
    if (mode & S_IROTH) {
        perms[7] = 'r';
    }
    if (mode & S_IWOTH) {
        perms[8] = 'w';
    }
    if (mode & S_IXOTH) {
        perms[9] = 'x';
    }
    return perms;
}

char* format_time(time_t mtime) {
    const size_t time_str_length = 21;
    char* time_str = calloc(time_str_length, sizeof(char));
    if (time_str == NULL) {
        return NULL;
    } 
    struct tm *ltm = localtime(&mtime);
    strftime(time_str, time_str_length, "%b %d %H:%M", ltm);
    return time_str;
}

void get_absolute_path(char* relative_path, char* absolute_path, char* current_directory) {
    if (relative_path[0] == '/') {
        // Абсолютный путь уже указан
        strncpy(absolute_path, relative_path, BUFFER_SIZE);
    } else {
        // Указан относительный путь, поэтому нужно добавить его в текущий каталог
        if (current_directory[strlen(current_directory) - 1] == '/') {
            // Текущий каталог уже заканчивается косой чертой, поэтому не нужно его добавлять
            snprintf(absolute_path, BUFFER_SIZE, "%s%s", current_directory, relative_path);
        } else {
            // Текущий каталог не заканчивается косой чертой, поэтому нужно добавить его
            snprintf(absolute_path, BUFFER_SIZE, "%s/%s", current_directory, relative_path);
        }
    }
}

size_t get_cpu_count(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int parse_config_file(struct Config* config) {
	FILE *fp = fopen("config.conf", "r");
    if (fp == NULL) {
        perror("Error opening config file");
        return 1;
    }
	char line[PARAM_SIZE];
    char key[PARAM_SIZE];
    char value[PARAM_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%[^=]=%s", key, value) != 2) {
            fprintf(stderr, "Error parsing line: %s", line);
            fclose(fp);
            return 1;
        }
		if (strcmp(key, "command_port") == 0) {
            config->command_port = atoi(value);
        } else if (strcmp(key, "username") == 0) {
            strcpy(config->username, value);
        } else if (strcmp(key, "password") == 0) {
            strcpy(config->password, value);
        } else if (strcmp(key, "timeout") == 0) {
            config->timeout = atoi(value);
        } else if (strcmp(key, "server_directory") == 0) {
			if (!file_exists(value)) {
				fprintf(stderr, "Error parsing line: %sDirectory '%s' does not exist.\n", line, value);
				fclose(fp);
				return 1;
			}
            strcpy(config->server_directory, value);
        } else if (strcmp(key, "tar_command_path") == 0) {
            strcpy(config->tar_command_path, value);
        } else if (strcmp(key, "bz2_command_path") == 0) {
            strcpy(config->bz2_command_path, value);
        }
    }
	if (ferror(fp)) {
        perror("Error reading config file");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}
