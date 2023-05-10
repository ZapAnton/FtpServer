#pragma once
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "user.h"
#include "utils.h"
#include "config.h"

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

enum Command command_str_to_enum(const char* const command_str);

void process_command(char* buffer, struct user* current_user, const Config* config);
