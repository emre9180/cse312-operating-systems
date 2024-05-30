# Compiler
CC := gcc

# Directories
SRC_DIR := src
OBJ_DIR := object_files

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Compiler flags
CFLAGS := -Wall -Wextra

# Target
TARGET := myprogram

# Default target
all: $(TARGET)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)