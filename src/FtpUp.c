#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include "utils.h"
#include "user.h"
#include "consts.h"
#include "config.h"
#include "command.h"

Config config = {0, "", "", 0, 0, "", ""}; // Инициализируем структуру

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
int shared_data = 0;
int running = 1;

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

		process_command(buffer, &current_user, &config);
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
