CFLAGS := -std=c17 -lpthread -Wall -Wextra -pedantic -Wfatal-errors -Werror
CC := gcc
FINAL_BIN := ftp_server
BUILD_DIR := ./build
INCLUDE_DIR := ./include
SRC_DIR := ./src
COMMANDS_SRC_DIR := $(SRC_DIR)/commands
SRC := $(SRC_DIR)/server.c $(SRC_DIR)/command.c $(SRC_DIR)/utils.c $(COMMANDS_SRC_DIR)/abor.c $(COMMANDS_SRC_DIR)/cwd.c $(COMMANDS_SRC_DIR)/dele.c $(COMMANDS_SRC_DIR)/help.c $(COMMANDS_SRC_DIR)/list.c $(COMMANDS_SRC_DIR)/mkd.c $(COMMANDS_SRC_DIR)/nlst.c $(COMMANDS_SRC_DIR)/pass.c $(COMMANDS_SRC_DIR)/pasv.c $(COMMANDS_SRC_DIR)/port.c $(COMMANDS_SRC_DIR)/pwd.c $(COMMANDS_SRC_DIR)/quit.c $(COMMANDS_SRC_DIR)/retr.c $(COMMANDS_SRC_DIR)/rmd.c $(COMMANDS_SRC_DIR)/rnfr.c $(COMMANDS_SRC_DIR)/rnto.c $(COMMANDS_SRC_DIR)/stor.c $(COMMANDS_SRC_DIR)/user.c  

$(BUILD_DIR)/$(FINAL_BIN): $(BUILD_DIR) $(SRC)
	$(CC) -I$(INCLUDE_DIR) $(CFLAGS) -o $@ $(SRC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: run
run: $(BUILD_DIR)/$(FINAL_BIN)
	$(BUILD_DIR)/$(FINAL_BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
