#pragma once
#include <dirent.h>

#include "user.h"
#include "utils.h"

void run_nlst(struct user* current_user, const char* const argument, const Config* config);
