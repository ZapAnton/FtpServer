#include "commands/help.h"

void run_help(struct user* current_user, const char* const argument) {
    if (argument == NULL) {
        send_response(current_user->control_socket,
                      "214-The following commands are recognized.\r\n");
        send_response(current_user->control_socket,
                      " ABOR CWD DELE HELP LIST MKD NLST NOOP PASS PASV PORT PWD QUIT RETR RMD "
                      "RNFR RNTO STOR SYST TYPE USER\r\n");
        send_response(current_user->control_socket, "214 Help OK.\r\n");
    } else {
        send_response(current_user->control_socket,
                      "214 Help not available for specified command.\r\n");
    }
}
