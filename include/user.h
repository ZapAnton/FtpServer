#pragma once
#include <netinet/in.h>
#include "stdbool.h"
#include "consts.h"

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
