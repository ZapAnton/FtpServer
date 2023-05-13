#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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

int thread_count = 1;
bool running = true;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int get_thread_count() {
    int current_thread_count = 0;
    pthread_mutex_lock(&mutex);
    current_thread_count = thread_count;
    pthread_mutex_unlock(&mutex);
    return current_thread_count;
}

void thread_count_dec() {
    pthread_mutex_lock(&mutex);
    thread_count -= 1;
    pthread_mutex_unlock(&mutex);
}

void thread_count_inc() {
    pthread_mutex_lock(&mutex);
    thread_count += 1;
    pthread_mutex_unlock(&mutex);
}

void *handle_client(void *arg) {
    int client_socket = *(int*) arg;
    free(arg);
    printf("Thread No %lu accepted the connection. Current thread count is %d\n", pthread_self(), get_thread_count());
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
		const int status = process_command(buffer, &current_user, &config);
        if (status == -1) {
            break;
        }
	}
	// Окончание обработки запросов клиента
    close(client_socket);
    thread_count_inc();
    printf("Thread No %lu closed the connection. Current thread count is %d\n", pthread_self(), get_thread_count());
    pthread_exit(NULL);
}

void create_worker_thread(int* client_socket) {
    thread_count_dec();
    // Создание потока для обработки клиента
    pthread_t thread = 0;
    printf("Creating thread for socket %d\n", *client_socket);
    if (pthread_create(&thread, NULL, handle_client, client_socket) != 0) {
        fprintf(stderr, "Error creating thread for client\n");
        thread_count_inc();
        return;
    }
    pthread_detach(thread);
}

int main() {
    puts("Starting FTP server.");
    if (parse_config_file(&config) != 0) {
        return -1;
    }
    thread_count = get_cpu_count() - 1;
    printf("Using %d threads for handling incoming client connections.\n", thread_count);
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
    if (listen(server_socket, get_thread_count()) < 0) {
        fprintf(stderr, "Error listening for connections\n");
        exit(EXIT_FAILURE);
    }

	// Ожидание входящих соединений
    while (running) {
        if (get_thread_count() < 1) {
            continue;
        }
    // Ожидание входящего соединения
        struct sockaddr_in client_address = { 0 };
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_length);
        if (client_socket < 0) {
            fprintf(stderr, "Thread %lu: Error accepting incoming connection: %s\n", pthread_self(), strerror(errno));
            pthread_exit(NULL);
        } 
        // Установка таймаута на прием данных
        struct timeval timeout;
        timeout.tv_sec = config.timeout;
        timeout.tv_usec = 0;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt failed");
            pthread_exit(NULL);
        }
        int* client_socket_copy = calloc(1, sizeof(client_socket));
        *client_socket_copy = client_socket;
        create_worker_thread(client_socket_copy);
    }

    // Завершение работы сервера
    close(server_socket);
    pthread_mutex_destroy(&mutex);
    return 0;
}
