#pragma once
#include "config.h"
#include "user.h"
#include "utils.h"

void run_user(struct user* const current_user, const char* const argument,
              const struct Config* config);
