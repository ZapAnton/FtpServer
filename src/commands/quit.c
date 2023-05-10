#include "commands/quit.h"

void run_quit(struct user* const current_user) {
    send_response(current_user->control_socket, "221 Goodbye.\r\n");
    close(current_user->control_socket);
    if (current_user->data_socket >= 0) {
        close(current_user->data_socket);
        current_user->data_socket = -1;
    }
}
