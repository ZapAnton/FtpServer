#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

typedef struct Config {
	int command_port;
    char username[256];
    char password[256];
    int timeout;
	int thread_count;
	char server_directory[256];
	char tar_command_path[256];	
} Config;

Config config = {0, "", "", 0, 0, "", ""}; // Инициализируем структуру

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
int shared_data = 0;
int running = 1;

struct user {
    char name[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    bool authenticated;
    int control_socket;
	int data_socket;
	bool is_pasv;
    struct sockaddr_in data_address;
	char* current_directory;
	bool is_aborted;
	char rnfr_name[BUFFER_SIZE];
};

enum Command {
    NOOP,
	QUIT,
	HELP,
    USER,
    PASS,
    SYST,
    PORT,
	PASV,
    NLST,
	LIST,
	STOR,
	UPLOAD,
    RETR,
	GET,
	DELE,
	CWD,
	PWD,
	MKD,
	RMD,
	RNFR,
	RNTO,
	ABOR,
    UNKNOWN,
};

void send_response(int client_socket, const char* response) {
    int response_len = strlen(response);
    int bytes_sent = send(client_socket, response, response_len, 0);
    if (bytes_sent != response_len) {
        perror("Error sending response");
        exit(EXIT_FAILURE);
    }
}

/*void process_pasv_command(int client_socket, struct user* current_user) {
    // Получение адреса сервера и порта, на котором будет прослушиваться соединение для передачи данных
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = 0;

    // Создание сокета для передачи данных и привязка его к заданному адресу сервера
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    bind(data_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Перевод сокета в режим прослушивания
    listen(data_socket, 1);

    // Получение порта, на котором был создан сокет
    socklen_t server_address_length = sizeof(server_address);
    getsockname(data_socket, (struct sockaddr*)&server_address, &server_address_length);
    int data_port = ntohs(server_address.sin_port);

    // Сохранение адреса и порта для передачи данных в структуре current_user
    memcpy(&(current_user->data_address), &server_address, sizeof(server_address));

    // Отправка клиенту сообщения с IP-адресом и портом для соединения
    char response[BUFFER_SIZE];
    unsigned char* p = (unsigned char*)&server_address.sin_addr;
    snprintf(response, BUFFER_SIZE, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",
             p[0], p[1], p[2], p[3], data_port / 256, data_port % 256);
    send_response(client_socket, response);
}*/

enum Command command_str_to_enum(const char* const command_str) {
    enum Command command = UNKNOWN;
	if (strcmp(command_str, "NOOP") == 0) {
        command = NOOP;
    } else if (strcmp(command_str, "QUIT") == 0) {
        command = QUIT;
	} else if (strcmp(command_str, "HELP") == 0) {
        command = HELP;
    } else if (strcmp(command_str, "USER") == 0) {
        command = USER;
    } else if (strcmp(command_str, "PASS") == 0) {
        command = PASS;
    } else if (strcmp(command_str, "SYST") == 0) {
        command = SYST;
    } else if (strcmp(command_str, "PORT") == 0) {
        command = PORT;
	} else if (strcmp(command_str, "PASV") == 0) {
        command = PASV;
    } else if (strcmp(command_str, "NLST") == 0) {
        command = NLST;
	} else if (strcmp(command_str, "LIST") == 0) {
        command = LIST;
	} else if (strcmp(command_str, "STOR") == 0) {
        command = STOR;
	} else if (strcmp(command_str, "UPLOAD") == 0) {
        command = UPLOAD;
    } else if (strcmp(command_str, "RETR") == 0) {
        command = RETR;
	} else if (strcmp(command_str, "GET") == 0) {
        command = GET;
	} else if (strcmp(command_str, "DELE") == 0) {
        command = DELE;
    } else if (strcmp(command_str, "CWD") == 0) {
        command = CWD;
    } else if (strcmp(command_str, "PWD") == 0) {
        command = PWD;
	} else if (strcmp(command_str, "MKD") == 0) {
        command = MKD;
	} else if (strcmp(command_str, "RMD") == 0) {
        command = RMD;
	} else if (strcmp(command_str, "RNFR") == 0) {
        command = RNFR;
	} else if (strcmp(command_str, "RNTO") == 0) {
        command = RNTO;
	} else if (strcmp(command_str, "ABOR") == 0) {
        command = ABOR;
	}
    return command;
}

void run_help(struct user* current_user, const char* const argument) {
    if (argument == NULL) {
        send_response(current_user->control_socket, "214-The following commands are recognized.\r\n");
        send_response(current_user->control_socket, " ABOR CWD DELE GET HELP LIST MKD NLST NOOP PASS PASV PORT PWD QUIT RETR RMD RNFR RNTO STOR SYST UPLOAD USER\r\n");
        send_response(current_user->control_socket, "214 Help OK.\r\n");
    } else {
        send_response(current_user->control_socket, "214 Help not available for specified command.\r\n");
    }
}

void run_quit(struct user* const current_user) {
    send_response(current_user->control_socket, "221 Goodbye.\r\n");
    close(current_user->control_socket);
    if (current_user->data_socket >= 0) {
        close(current_user->data_socket);
        current_user->data_socket = -1;
    }
}

void run_user(struct user* const current_user, const char* const argument) {
    if (strcmp(argument, config.username) == 0) {
        send_response(current_user->control_socket, "331 Please specify the password.\r\n");
        strncpy(current_user->name, argument, MAX_USERNAME_LENGTH - 1);
    } else {
        send_response(current_user->control_socket, "332 Need account for login.\r\n");
    }
}

void run_pass(struct user* const current_user, const char* const argument) {
    if (strcmp(current_user->name, config.username) == 0 && strcmp(argument, config.password) == 0) {
        current_user->authenticated = true;
        send_response(current_user->control_socket, "230 Logged in.\r\n");
    } else {
        send_response(current_user->control_socket, "530 Login incorrect.\r\n");
    }
}

void run_port(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    // Разбиение аргумента на IP-адрес и порт
	char* ip1 = strtok(argument, ",");
	char* ip2 = strtok(NULL, ",");
	char* ip3 = strtok(NULL, ",");
	char* ip4 = strtok(NULL, ",");
	int port1 = atoi(strtok(NULL, ","));
	int port2 = atoi(strtok(NULL, ","));
    char ip_str[16] = {'\0'};
    snprintf(ip_str, 16, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);
	int data_port = port1 * 256 + port2;
	// Преобразование IP-адреса в структуру sockaddr_in
	struct sockaddr_in data_address = { 0 };
	memset(&data_address, 0, sizeof(data_address));
	inet_pton(AF_INET, ip_str, &data_address.sin_addr);
	data_address.sin_family = AF_INET;
	data_address.sin_port = htons(data_port);
	// Сохранение IP-адреса и порта для передачи данных в структуре current_user
	memcpy(&(current_user->data_address), &data_address, sizeof(data_address));
	current_user->is_pasv = false;
	current_user->is_aborted = false;
	// Отправка клиенту сообщения об успешной установке порта для передачи данных
	char response[BUFFER_SIZE];
	snprintf(response, BUFFER_SIZE, "200 PORT command successful (%s:%d).\r\n", ip_str, data_port);
	send_response(current_user->control_socket, response);
}

void run_pasv(struct user* const current_user, const char* const argument) {
	if (!current_user || !argument) {}
}

int establish_data_connection(struct user* current_user, int* data_socket) {
    *data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(*data_socket, (struct sockaddr*)&(current_user->data_address), sizeof(current_user->data_address)) < 0) {
        send_response(current_user->control_socket, "425 Can't open data connection.\r\n");
        return -1;
    }
	
	current_user->data_socket = *data_socket;
	
    return 0;
}

void run_nlst(struct user* current_user, const char* const argument) {    
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection for entry list\r\n");

    // Получение списка файлов
    size_t directory_path_length = strlen(config.server_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s", config.server_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", config.server_directory);
    }
    DIR* directory = opendir(directory_path);
    struct dirent* entry;
    char buffer[BUFFER_SIZE];
    while ((entry = readdir(directory)) != NULL) {
        snprintf(buffer, BUFFER_SIZE, "%s\r\n", entry->d_name);
        send(data_socket, buffer, strlen(buffer), 0);
    }
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(data_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}

bool is_dir(const char* const path) {
    struct stat statbuf = { 0 };
    if (stat(path, &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

bool file_exists(const char* const filepath) {
    return access(filepath, F_OK) == 0;
}

void make_archive(char* dirpath, char* archive_filepath) {
    const size_t command_length = strlen("tar -czf") + 1 + strlen(archive_filepath) + 1 + strlen("-C") + 1 + strlen(dirpath) + 1 + strlen(".");
    char* command = calloc(command_length + 1, sizeof(char));
    snprintf(command, command_length + 1, "tar -czf %s -C %s .", archive_filepath, dirpath);
    const int status = system(command);
    free(command);
    if (status != 0) {
        fprintf(stderr, "Archive error: %s\n", strerror(errno));
    }
}

void transfer_file(const struct user* current_user, const int data_socket, const char* const filepath) {
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
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
    fclose(file);
}

void transfer_dir(const struct user* current_user, const int data_socket, char* dirpath) {
    if (!file_exists(config.tar_command_path)) {
        fprintf(stderr, "'tar' command not found");
        send_response(current_user->control_socket, "550 tar command unavailable.\r\n");
        return;
    }
    const char* archive_extension = ".tar.gz";
    const size_t archive_filepath_length = strlen(dirpath) + strlen(archive_extension);
    char* archive_filepath = calloc(archive_filepath_length + 1, sizeof(char));
    snprintf(archive_filepath, archive_filepath_length + 1, "%s%s", dirpath, archive_extension);
    if (!file_exists(archive_filepath)) {
        make_archive(dirpath, archive_filepath);
    }
    transfer_file(current_user, data_socket, archive_filepath);
    free(archive_filepath);
}

void run_retr(struct user* current_user, const char* const filepath) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection\r\n");
	//TODO: С этим надо что-то сделать....
	/*if (current_user->data_socket < 0) {
        send_response(current_user->control_socket, "425 No data connection");
        return;
    }
    if (current_user->is_aborted) {
        send_response(current_user->control_socket, "426 Connection aborted");
        return;
    }*/
	
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    size_t filepath_length = strlen(config.server_directory) + 1 + strlen(filepath);
    char* path = calloc(filepath_length + 1, sizeof(char));
    snprintf(path, filepath_length + 1, "%s/%s", config.server_directory, filepath);
    if (is_dir(path)) {
        transfer_dir(current_user, data_socket, path);
    } else {
        transfer_file(current_user, data_socket, path);
    }
    free(path);
    // Закрытие соединения для передачи данных
    close(data_socket);
}

void run_get(const struct user* current_user, const char* const filepath) {
	if (!current_user || !filepath) {}
}

char* format_perms(mode_t mode) {
    char *perms = (char*)malloc(10 * sizeof(char));
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
    char *time_str = (char*)malloc(20 * sizeof(char));
    if (time_str == NULL) {
        return NULL;
    }
    struct tm *ltm = localtime(&mtime);
    strftime(time_str, 20, "%b %d %H:%M", ltm);
    return time_str;
}

void run_list(struct user* const current_user, const char* const argument) {    
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    // Отправка клиенту сообщения о начале передачи списка файлов
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection for entry list\r\n");

    // Получение списка файлов
    size_t directory_path_length = strlen(config.server_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s", config.server_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", config.server_directory);
    }
    DIR* directory = opendir(directory_path);
	if (directory == NULL) {
		send_response(current_user->control_socket, "550 Failed to open directory.");
		return;
	}
    struct dirent* entry;
    char buffer[BUFFER_SIZE];
    while ((entry = readdir(directory)) != NULL) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		sprintf(buffer, "%s/%s", directory_path, entry->d_name);
		struct stat info;
		if (stat(buffer, &info) == -1) {
			send_response(current_user->control_socket, "550 Failed to get file information.");
			closedir(directory);
			return;
		}
        /* TODO: Rewrite with snprintf
		char *type = (S_ISDIR(info.st_mode)) ? "d" : "-";
		char *perms = format_perms(info.st_mode);
		char *mtime = format_time(info.st_mtime);
		char filename[BUFFER_SIZE];
		strncpy(filename, entry->d_name, BUFFER_SIZE);
		send_response(data_socket, "%s%s %3lu %-8d %-8d %8lu %s %s\r\n", type, perms,
					  (unsigned long)info.st_nlink, info.st_uid, info.st_gid,
					  (unsigned long)info.st_size, mtime, filename);
		free(perms);
		free(mtime);
        */
	}
    closedir(directory);
    free(directory_path);

    // Закрытие соединения для передачи данных
    close(data_socket);
    send_response(current_user->control_socket, "226 Transfer complete.\r\n");
}

void run_cwd(struct user* const current_user, char* const argument) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
	char* new_directory = argument;
    // Обработка относительного пути
    if (argument[0] != '/') {
        char* current_directory = current_user->current_directory;
        size_t len = strlen(current_directory) + strlen(argument) + 2;
        new_directory = (char*) malloc(len);
        snprintf(new_directory, len, "%s/%s", current_directory, argument);
    }
    // Проверка, что новый путь существует
    if (access(new_directory, F_OK) == -1) {
        send_response(current_user->control_socket, "550 Invalid path\r\n");
        return;
    }
    // Обновление текущего рабочего каталога
    free(current_user->current_directory);
    current_user->current_directory = new_directory;
    send_response(current_user->control_socket, "250 Directory changed\r\n");
}

void run_pwd(const struct user* const current_user) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char* current_directory = current_user->current_directory;
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "257 \"%s\"\r\n", current_directory);
    send_response(current_user->control_socket, response);
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

void run_mkd(const struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char response[BUFFER_SIZE];
    char directory[BUFFER_SIZE];
    get_absolute_path(argument, directory, current_user->current_directory);
    if (mkdir(directory, 0777) == 0) {
        snprintf(response, BUFFER_SIZE, "257 \"%s\" created\r\n", argument);
    } else {
        snprintf(response, BUFFER_SIZE, "550 %s\r\n", strerror(errno));
    }
    send_response(current_user->control_socket, response);
}

void run_rmd(const struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char absolute_path[BUFFER_SIZE];
    get_absolute_path(argument, absolute_path, current_user->current_directory);

    // Проверка, существует ли каталог и есть ли у пользователя разрешение на его удаление
    if (access(absolute_path, F_OK) == -1 || access(absolute_path, W_OK) == -1) {
        send_response(current_user->data_socket, "550 Requested action not taken. File unavailable.");
        return;
    }

    // Попытка удалить каталог
    if (rmdir(absolute_path) == -1) {
        send_response(current_user->data_socket, "550 Requested action not taken. File unavailable.");
        return;
    }
	
    send_response(current_user->data_socket, "250 Requested file action okay, completed.");
}

void run_abor(struct user* const current_user) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    if (current_user->data_socket > 0) {
        close(current_user->data_socket);
        current_user->data_socket = -1;
    }
    current_user->is_aborted = 1;
    send_response(current_user->control_socket, "226 Abort successful");
}

void run_stor(struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    if (current_user->data_socket == -1) {
        send_response(current_user->control_socket, "425 Use PORT or PASV first.");
        return;
    }
    char path[BUFFER_SIZE];
	get_absolute_path(argument, path, current_user->current_directory);
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        send_response(current_user->control_socket, "550 Failed to open file.");
        return;
    }
    int n = 0;
    char buf[BUFFER_SIZE] = { 0 };
    while ((n = read(current_user->data_socket, buf, BUFFER_SIZE)) > 0) {
        if (fwrite(buf, 1, n, file) != (size_t) n) {
            send_response(current_user->control_socket, "550 Failed to write file.");
            fclose(file);
            return;
        }
    }
    fclose(file);
    send_response(current_user->control_socket, "226 Transfer complete.");
}

void run_upload(struct user* const current_user, const char* const argument) {
	if (!current_user || !argument) {}
}

void run_dele(struct user* const current_user, char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char filepath[BUFFER_SIZE] = { 0 };
	get_absolute_path(argument, filepath, current_user->current_directory);
    if (remove(filepath) == 0) {
        send_response(current_user->control_socket, "250 File deleted.");
    } else {
        send_response(current_user->control_socket, "550 Failed to delete file.");
    }
}

void run_rnfr(struct user* const current_user, const char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char filename[BUFFER_SIZE];
    snprintf(filename, BUFFER_SIZE, "%s/%s", current_user->current_directory, argument);
    if (access(filename, F_OK) == -1) {
        send_response(current_user->control_socket, "550 File or directory not found.");
        return;
    }
    strncpy(current_user->rnfr_name, argument, BUFFER_SIZE);
    send_response(current_user->control_socket, "350 RNFR accepted - file exists, ready for destination.");
}

void run_rnto(struct user* const current_user, const char* const argument) {
	if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
	
    char old_filename[BUFFER_SIZE] = { 0 };
    char new_filename[BUFFER_SIZE] = { 0 };
    snprintf(old_filename, BUFFER_SIZE + 1, "%s/%s", current_user->current_directory, current_user->rnfr_name);
    snprintf(new_filename, BUFFER_SIZE + 1, "%s/%s", current_user->current_directory, argument);
    if (rename(old_filename, new_filename) == -1) {
        send_response(current_user->control_socket, "550 Failed to rename file or directory.");
        return;
    }
    send_response(current_user->control_socket, "250 RNTO successful.");
	//TODO: current_user->rnfr_name нужно ли очищать?
}

void process_command(char* buffer, struct user* current_user) {
    puts(buffer);
    const char* command_str = strtok(buffer, " \r\n");
    char* argument = strtok(NULL, " \r\n");
    const enum Command command = command_str_to_enum(command_str);
    switch (command) {
        case QUIT:
            run_quit(current_user);
            break;
		case PWD:
            run_pwd(current_user);
            break;
		case ABOR:
            run_abor(current_user);
            break;
		case HELP:
            run_help(current_user, argument);
            break;
        case USER:
            run_user(current_user, argument);
            break;
        case PASS: 
            run_pass(current_user, argument);
            break;
        case PORT:
            run_port(current_user, argument);
            break;
		case PASV:
            run_pasv(current_user, argument);
            break;
        case NLST:
            run_nlst(current_user, argument);
            break;
		case LIST:
            run_list(current_user, argument);
            break;
		case STOR:
            run_stor(current_user, argument);
            break;
		case UPLOAD:
            run_upload(current_user, argument);
            break;
        case RETR:
            run_retr(current_user, argument);
            break;
		case GET:
            run_get(current_user, argument);
            break;
		case DELE:
            run_dele(current_user, argument);
            break;
		case RNFR:
            run_rnfr(current_user, argument);
            break;
		case RNTO:
            run_rnto(current_user, argument);
            break;
		case CWD:
            run_cwd(current_user, argument);
            break;
		case MKD:
            run_mkd(current_user, argument);
            break;
		case RMD:
            run_rmd(current_user, argument);
            break;
		case NOOP:
            send_response(current_user->control_socket, "200 NOOP command successful.\r\n");
            break;
        case SYST:
            send_response(current_user->control_socket, "315 UNIX.\r\n");
            break;
        case UNKNOWN:
            send_response(current_user->control_socket, "502 Command not implemented.\r\n");
            break;
        default:
            send_response(current_user->control_socket, "500 Syntax error, command unrecognized.\r\n");
            break;
    }
}

void *handle_client(void *arg) {
    int client_socket = *(int *) arg;
	
	// Отправка сообщения приветствия клиенту
    char* welcome_message = "220 FTP server is ready\r\n";
    send_response(client_socket, welcome_message);
    
	// Чтение и обработка команд от клиента
    char buffer[BUFFER_SIZE] = { '\0' };
    struct user current_user = { 0 };
    current_user.control_socket = client_socket;
	while (true) {
		int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
		if (bytes_received <= 0) {
			break;
		}
		buffer[bytes_received] = '\0';

		process_command(buffer, &current_user);
	}
	
	// Окончание обработки запросов клиента
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
	FILE *fp = fopen("config.conf", "r");
    if (fp == NULL) {
        perror("Error opening config file");
        return 1;
    }
	
	char line[256];
    char key[256];
    char value[256];
	
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%[^=]=%s", key, value) != 2) {
            fprintf(stderr, "Error parsing line: %s", line);
            fclose(fp);
            return 1;
        }
		
		if (strcmp(key, "command_port") == 0) {
            config.command_port = atoi(value);
        } else if (strcmp(key, "username") == 0) {
            strcpy(config.username, value);
        } else if (strcmp(key, "password") == 0) {
            strcpy(config.password, value);
        } else if (strcmp(key, "timeout") == 0) {
            config.timeout = atoi(value);
        } else if (strcmp(key, "thread_count") == 0) {
            config.thread_count = atoi(value);
        } else if (strcmp(key, "server_directory") == 0) {
            strcpy(config.server_directory, value);
        } else if (strcmp(key, "tar_command_path") == 0) {
            strcpy(config.tar_command_path, value);
        }
    }
	
	if (ferror(fp)) {
        perror("Error reading config file");
        fclose(fp);
        return 1;
    }
	
    fclose(fp);
	
    // Создание сокета для прослушивания входящих соединений
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Установка сокета на переиспользование порта
    const int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // Настройка адреса и порта для прослушивания входящих соединений
    struct sockaddr_in server_address = { 0 };
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(config.command_port);

    // Связывание сокета с адресом и портом
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == 0) {
    printf("FTP server is ready\r\n");
	} else {
		perror("Socket binding error with address and port");
		exit(EXIT_FAILURE);
	}

    // Установка сервера в режим прослушивания
    if (listen(server_socket, config.thread_count) < 0) {
        fprintf(stderr, "Error listening for connections\n");
        exit(EXIT_FAILURE);
    }

	// Ожидание входящих соединений
    while (running) {
		// Ожидание входящего соединения
		struct sockaddr_in client_address = { 0 };
		socklen_t client_address_length = sizeof(client_address);
		int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_length);
		if (client_socket < 0) {
            fprintf(stderr, "Error accepting incoming connection\n");
            continue;
        }
		
		// Установка таймаута на прием данных
		struct timeval timeout;
		timeout.tv_sec = config.timeout;
		timeout.tv_usec = 0;
		if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
			perror("setsockopt failed");
			continue;
		}

        // Создание потока для обработки клиента
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, &client_socket) != 0) {
            fprintf(stderr, "Error creating thread for client\n");
            close(client_socket);
            continue;
        }

        pthread_detach(thread);
    }

    // Завершение работы сервера
    close(server_socket);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_var);

    return 0;
}
