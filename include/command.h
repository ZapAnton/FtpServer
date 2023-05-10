#pragma once
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "utils.h"
#include "config.h"
#include "commands/help.h"
#include "commands/quit.h"
#include "commands/user.h"
#include "commands/pass.h"
#include "commands/port.h"
#include "commands/pasv.h"
#include "commands/nlst.h"
#include "commands/retr.h"
#include "commands/get.h"
#include "commands/list.h"
#include "commands/cwd.h"
#include "commands/pwd.h"
#include "commands/mkd.h"
#include "commands/rmd.h"
#include "commands/abor.h"
#include "commands/stor.h"
#include "commands/upload.h"
#include "commands/dele.h"
#include "commands/rnfr.h"
#include "commands/rnto.h"

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
