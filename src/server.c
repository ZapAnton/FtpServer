#include "command.h"
#include "config.h"
#include "consts.h"
#include "user.h"
#include "utils.h"
#include <signal.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct Config config = {.command_port = 0,
                        .username = "",
                        .password = "",
                        .timeout = 0,
                        .server_directory = "",
                        .tar_command_path = "",
                        .bz2_command_path = ""}; // Инициализируем структуру

volatile bool running = true;
int thread_count = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig)
{
	printf("\nSIGINT = %d - Stopping FTP server.\n", sig);
    pthread_mutex_lock(&mutex);
    running = false;
    pthread_mutex_unlock(&mutex);
}

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

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    printf("Thread No %lu accepted the connection. Current thread count is %d\n", pthread_self(),
           get_thread_count());
    // Отправка сообщения приветствия клиенту
    char* welcome_message = "220 FTP server is ready\r\n";
    send_response(client_socket, welcome_message);
    // Чтение и обработка команд от клиента
    char buffer[BUFFER_SIZE] = {'\0'};
    struct user current_user = {0};
    current_user.data_connection_type = ACTIVE;
    current_user.control_socket = client_socket;
    strcpy(current_user.current_directory, config.server_directory);
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
    printf("Thread No %lu closed the connection. Current thread count is %d\n", pthread_self(),
           get_thread_count());
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
        free(client_socket);
        return;
    }
    pthread_detach(thread);
}

int main() {
	signal(SIGINT, handle_sigint);
    puts("Starting FTP server.");
    if (parse_config_file(&config) != 0) {
        return -1;
    }
    thread_count = get_cpu_count() - 1;
    if (thread_count < 1) {
        thread_count = 1;
    }
    printf("Using %d threads for handling incoming client connections.\n", thread_count);
    // Создание сокета для прослушивания входящих соединений
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Установка сокета на переиспользование порта
    const int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // Настройка адреса и порта для прослушивания входящих соединений
    struct sockaddr_in server_address = {0};
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(config.command_port);

    // Связывание сокета с адресом и портом
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == 0) {
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

	//Подготовка структуры fd_set для использования с функцией select()
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
		
    // Ожидание входящих соединений
    while (running) {
        if (get_thread_count() < 1) {
            continue;
        }
		// Вызов select() для проверки состояния сокета
        int select_result = select(server_socket + 1, &read_fds, NULL, NULL, NULL);
        if (select_result == -1)
        {
            if (errno == EINTR)
            {
                // select() был прерван сигналом, продолжить следующую итерацию цикла
                continue;
            }
            else
            {
                // Обработка других ошибок select()
                perror("Error in select");
                break;  // Выход из цикла и завершение программы
            }
        }
        // Ожидание входящего соединения
        struct sockaddr_in client_address = {0};
        socklen_t client_address_length = sizeof(client_address);        
        int client_socket = -1;
        if (FD_ISSET(server_socket, &read_fds))
        {
            // Вызов accept() для принятия входящего подключения
            client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
        }
        if (client_socket < 0) {
            fprintf(stderr, "Thread %lu: Error accepting incoming connection: %s\n", pthread_self(),
                    strerror(errno));
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
	printf("\nFTP server is stopped.\n");
    return 0;
}
