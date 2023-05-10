CFLAGS := -std=c17 -Wall -Wextra -pedantic -Wfatal-errors -Werror
CC := gcc
BUILD_DIR := ./build
INCLUDE_DIR := ./include
SRC_DIR := ./src
SRC := $(SRC_DIR)/FtpUp.c $(SRC_DIR)/utils.c
FINAL_BIN := ftp_server

$(BUILD_DIR)/$(FINAL_BIN): $(BUILD_DIR)
	$(CC) -I$(INCLUDE_DIR) $(CFLAGS) -o $@ $(SRC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: run
run: $(BUILD_DIR)/$(FINAL_BIN)
	$(BUILD_DIR)/$(FINAL_BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
