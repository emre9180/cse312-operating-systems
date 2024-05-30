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

# Targets
TARGET1 := makeFileSystem
TARGET2 := fileSystemOper

# Default target
all: $(TARGET1) $(TARGET2)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create executables
$(TARGET1): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET2): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET1) $(TARGET2)
