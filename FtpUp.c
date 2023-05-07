#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
    struct sockaddr_in data_address;
};

enum Command {
    QUIT,
	HELP,
    USER,
    PASS,
    SYST,
    PORT,
    NLST,
    RETR,
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
    if (strcmp(command_str, "QUIT") == 0) {
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
    } else if (strcmp(command_str, "NLST") == 0) {
        command = NLST;
    } else if (strcmp(command_str, "RETR") == 0) {
        command = RETR;
    }
    return command;
}

void run_help(struct user* current_user, const char* argument) {
    if (argument == NULL) {
        send_response(current_user->control_socket, "214-The following commands are recognized.\r\n");
        send_response(current_user->control_socket, " HELP NLST PASS PORT QUIT RETR SYST USER\r\n");
        send_response(current_user->control_socket, "214 Help OK.\r\n");
    } else {
        send_response(current_user->control_socket, "214 Help not available for specified command.\r\n");
    }
}

void run_quit(const struct user* const current_user) {
    send_response(current_user->control_socket, "221 Goodbye.\r\n");
    close(current_user->control_socket);
    if (current_user->data_socket >= 0) {
        close(current_user->data_socket);
        current_user->data_socket = -1;
    }
	
	restart = true;
}

void run_user(struct user* const current_user, const char* const argument) {
    if (strcmp(argument, config->username) == 0) {
        send_response(current_user->control_socket, "331 Please specify the password.\r\n");
        strncpy(current_user->name, argument, MAX_USERNAME_LENGTH - 1);
    } else {
        send_response(current_user->control_socket, "332 Need account for login.\r\n");
    }
}

void run_pass(struct user* const current_user, const char* const argument) {
    if (strcmp(current_user->name, config->username) == 0 && strcmp(argument, config->password) == 0) {
        current_user->authenticated = true;
        send_response(current_user->control_socket, "230 Logged in.\r\n");
    } else {
        send_response(current_user->control_socket, "530 Login incorrect.\r\n");
    }
}

void run_port(struct user* const current_user, char* argument) {
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

	// Отправка клиенту сообщения об успешной установке порта для передачи данных
	char response[BUFFER_SIZE];
	snprintf(response, BUFFER_SIZE, "200 PORT command successful (%s:%d).\r\n", ip_str, data_port);
	send_response(current_user->control_socket, response);
}

int establish_data_connection(const struct user* current_user, int* data_socket) {
    *data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(*data_socket, (struct sockaddr*)&(current_user->data_address), sizeof(current_user->data_address)) < 0) {
        send_response(current_user->control_socket, "425 Can't open data connection.\r\n");
        return -1;
    }
    return 0;
}

void run_nlst(const struct user* current_user, const char* const argument) {    
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
    size_t directory_path_length = strlen(coonfig->server_directory);
    if (argument) {
        directory_path_length += strlen(argument) + 1;
    }
    char* directory_path = calloc(directory_path_length + 1, sizeof(char));
    if (argument) {
        snprintf(directory_path, directory_path_length + 1, "%s/%s", coonfig->server_directory, argument);
    } else {
        snprintf(directory_path, directory_path_length + 1, "%s", coonfig->server_directory);
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
    if (!file_exists(config->tar_command_path)) {
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

void run_retr(const struct user* current_user, const char* const filepath) {
    if (!current_user->authenticated) {
        send_response(current_user->control_socket, "530 Not logged in.\r\n");
        return;
    }
    send_response(current_user->control_socket, "150 Opening ASCII mode data connection\r\n");
    int data_socket = 0;
    if (establish_data_connection(current_user, &data_socket) == -1) {
        return;
    }
    size_t filepath_length = strlen(coonfig->server_directory) + 1 + strlen(filepath);
    char* path = calloc(filepath_length + 1, sizeof(char));
    snprintf(path, filepath_length + 1, "%s/%s", coonfig->server_directory, filepath);
    if (is_dir(path)) {
        transfer_dir(current_user, data_socket, path);
    } else {
        transfer_file(current_user, data_socket, path);
    }
    free(path);
    // Закрытие соединения для передачи данных
    close(data_socket);
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
        case NLST:
            run_nlst(current_user, argument);
            break;
        case RETR:
            run_retr(current_user, argument);
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
    server_address.sin_port = htons(config->command_port);

    // Связывание сокета с адресом и портом
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == 0) {
    printf("FTP server is ready\r\n");
	} else {
		perror("Socket binding error with address and port");
		exit(EXIT_FAILURE);
	}

    // Установка сервера в режим прослушивания
    if (listen(server_socket, config->thread_count) < 0) {
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
		timeout.tv_sec = config->timeout;
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
