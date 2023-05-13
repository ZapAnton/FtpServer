#pragma once

struct Config {
	int command_port;
    char username[256];
    char password[256];
    int timeout;
	char server_directory[256];
	char tar_command_path[256];	
};
