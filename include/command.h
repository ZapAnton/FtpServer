#pragma once
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "utils.h"
#include "config.h"
#include "commands/abor.h"
#include "commands/cwd.h"
#include "commands/dele.h"
#include "commands/help.h"
#include "commands/list.h"
#include "commands/mkd.h"
#include "commands/nlst.h"
#include "commands/pass.h"
#include "commands/pasv.h"
#include "commands/port.h"
#include "commands/pwd.h"
#include "commands/quit.h"
#include "commands/retr.h"
#include "commands/rmd.h"
#include "commands/rnfr.h"
#include "commands/rnto.h"
#include "commands/stor.h"
#include "commands/user.h"
#include "commands/type.h"

enum Command {
    ABOR, //1-Отменить передачу
	CWD,  //2-Перейти в директорию
	DELE, //3-Удалить файл
	HELP, //4-Справка по командам
	LIST, //5-Получить содержимое каталога
	MKD,  //6-Создать директорию
	NLST, //7-Получить список каталогов
	NOOP, //8-Контроль соединения
	PASS, //9-Пароль
	PASV, //10-Перейти в пассивный режим
	PORT, //11-Перейти в активный режим
	PWD,  //12-Узнать текущую директорию
	QUIT, //13-Закончить работу
	RETR, //14-Получить файл с сервера
	RMD,  //15-Удалить директорию
	RNFR, //16-Выбрать файл для переименования
	RNTO, //17-Переименовать файл
	STOR, //18-Загрузить файл на сервер
	SYST, //19-Получить информацию о типа ОС на сервере
    USER, //20-Логин
    TYPE,
    UNKNOWN,
};

enum Command command_str_to_enum(const char* const command_str);

int process_command(char* buffer, struct user* current_user, const struct Config* config);
