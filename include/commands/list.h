#pragma once
#include <dirent.h>

#include "user.h"
#include "utils.h"
#include "data_connection.h"

void run_list(struct user* const current_user, const char* const argument, const Config* config);
