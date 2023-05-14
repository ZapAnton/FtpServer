#include "command.h"

enum Command command_str_to_enum(const char* const command_str) {
    enum Command command = UNKNOWN;
	if (strcmp(command_str, "NOOP") == 0) {
        command = NOOP;
	} else if (strcmp(command_str, "HELP") == 0) {
        command = HELP;
	} else if (strcmp(command_str, "SYST") == 0) {
        command = SYST;
    } else if (strcmp(command_str, "QUIT") == 0) {
        command = QUIT;	
    } else if (strcmp(command_str, "USER") == 0) {
        command = USER;
    } else if (strcmp(command_str, "PASS") == 0) {
        command = PASS;    
    } else if (strcmp(command_str, "PORT") == 0) {
        command = PORT;
	} else if (strcmp(command_str, "PASV") == 0) {
        command = PASV;
    } else if (strcmp(command_str, "NLST") == 0) {
        command = NLST;
	} else if (strcmp(command_str, "LIST") == 0) {
        command = LIST;
	} else if (strcmp(command_str, "TYPE") == 0) {
        command = TYPE;
	} else if (strcmp(command_str, "STOR") == 0) {
        command = STOR;
    } else if (strcmp(command_str, "RETR") == 0) {
        command = RETR;
	} else if (strcmp(command_str, "RNFR") == 0) {
        command = RNFR;
	} else if (strcmp(command_str, "RNTO") == 0) {
        command = RNTO;
	} else if (strcmp(command_str, "DELE") == 0) {
        command = DELE;
	} else if (strcmp(command_str, "PWD") == 0) {
        command = PWD;
    } else if (strcmp(command_str, "CWD") == 0) {
        command = CWD;    
	} else if (strcmp(command_str, "MKD") == 0) {
        command = MKD;
	} else if (strcmp(command_str, "RMD") == 0) {
        command = RMD;	
	} else if (strcmp(command_str, "ABOR") == 0) {
        command = ABOR;	
	}
    return command;
}

int process_command(char* buffer, struct user* current_user, const struct Config* config) {
    puts(buffer);
    const char* command_str = strtok(buffer, " \r\n");
    char* argument = strtok(NULL, " \r\n");
    const enum Command command = command_str_to_enum(command_str);
    switch (command) {
        case NOOP:
            send_response(current_user->control_socket, "200 NOOP command successful.\r\n");
            break;
		case HELP:
            run_help(current_user, argument);
            break;
		case SYST:
            send_response(current_user->control_socket, "315 UNIX.\r\n");
            break;
		case QUIT:
            run_quit(current_user);
            break;
		case USER:
            run_user(current_user, argument, config);
            break;
        case PASS: 
            run_pass(current_user, argument, config);
            break;		
		case PORT:
            run_port(current_user, argument);
            break;
		case PASV:
            run_pasv(current_user);
            break;
		case NLST:
            run_nlst(current_user, argument, config);
            break;
		case LIST:
            run_list(current_user, argument, config);
            break;
		case TYPE:
            run_type(current_user, argument);
            break;	
		case STOR:
            run_stor(current_user, argument);
            break;
        case RETR:
            run_retr(current_user, argument, config);
            break;
		case RNFR:
            run_rnfr(current_user, argument);
            break;
		case RNTO:
            run_rnto(current_user, argument);
            break;
		case DELE:
            run_dele(current_user, argument);
            break;
		case PWD:
            run_pwd(current_user);
            break;
		case CWD:
            run_cwd(current_user, argument, config);
            break;
		case MKD:
            run_mkd(current_user, argument);
            break;
		case RMD:
            run_rmd(current_user, argument);
            break;
		case ABOR:
            run_abor(current_user);
            break;		
        case UNKNOWN:
            send_response(current_user->control_socket, "502 Command not implemented.\r\n");
            break;
        default:
            send_response(current_user->control_socket, "500 Syntax error, command unrecognized.\r\n");
            break;
    }
    return 0;
}
