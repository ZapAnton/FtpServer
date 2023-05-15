#pragma once
#include "consts.h"

struct Config {
    int command_port;
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    int timeout;
    char server_directory[PARAM_SIZE];
    char tar_command_path[PARAM_SIZE];
    char bz2_command_path[PARAM_SIZE];
};
