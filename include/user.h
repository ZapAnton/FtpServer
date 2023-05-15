#pragma once
#include "consts.h"
#include "data_connection.h"
#include "stdbool.h"
#include <netinet/in.h>

struct user {
    char name[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    bool authenticated;
    int control_socket;
    int data_socket;
    int client_data_socket;
    struct sockaddr_in data_address;
    enum DataConnectionType data_connection_type;
    char current_directory[PARAM_SIZE];
    bool is_aborted;
    char rnfr_name[PARAM_SIZE];
};
