#pragma once

typedef struct Config {
	int command_port;
    char username[256];
    char password[256];
    int timeout;
	int thread_count;
	char server_directory[256];
	char tar_command_path[256];	
} Config;
