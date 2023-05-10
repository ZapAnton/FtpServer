CFLAGS := -std=c17 -Wall -Wextra -pedantic -Wfatal-errors -Werror
CC := gcc
BUILD_DIR := ./build
FINAL_BIN := ftp_server

$(BUILD_DIR)/$(FINAL_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ FtpUp.c

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: run
run: $(BUILD_DIR)/$(FINAL_BIN)
	$(BUILD_DIR)/$(FINAL_BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
