#pragma once

#include "data_connection.h"
#include "user.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>

void run_pasv(struct user* const current_user);
