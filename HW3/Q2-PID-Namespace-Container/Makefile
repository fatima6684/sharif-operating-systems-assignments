CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = zocker
SOURCES = main.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	@echo "Setting CAP_SYS_ADMIN capability..."
	sudo setcap cap_sys_admin+ep $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean

