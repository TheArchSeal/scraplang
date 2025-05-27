SRC_DIR=src
MAIN_SRC=$(SRC_DIR)/main.c
INC_DIR=include
BIN_DIR=bin
BUILD_DIR=build
OBJ_DIR=$(BUILD_DIR)/obj

TEST_DIR=tests
TEST_BIN_DIR=$(BIN_DIR)/tests
TEST_OBJ_DIR=$(BUILD_DIR)/tests
TEST_COMMAND=./test.sh

CC=gcc
CFLAGS=-g -Wall -Wextra -Wpedantic -Werror -std=c2x -I$(INC_DIR) -D__USE_MINGW_ANSI_STDIO=1 -MMD -MP

ifeq ($(OS),Windows_NT)
MKDIR=mkdir
RMDIR=rmdir /s /q
BASH="C:/Program Files/Git/bin/bash.exe"
EXE=.exe
else
MKDIR=mkdir -p
RMDIR=rm -rf
BASH=/bin/bash
EXE=
endif

TARGET=$(BIN_DIR)/smlc$(EXE)
SOURCES=$(wildcard $(SRC_DIR)/*.c)
OBJECTS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
MAIN_OBJ=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MAIN_SRC))
DEPS=$(OBJECTS:.o=.d)

TEST_CATEGORIES=$(patsubst $(TEST_DIR)/%/,%,$(wildcard $(TEST_DIR)/*/))
TEST_TARGETS=$(foreach category,$(TEST_CATEGORIES),$(TEST_BIN_DIR)/$(category)$(EXE))
TEST_SOURCES=$(foreach category,$(TEST_CATEGORIES),$(wildcard $(TEST_DIR)/$(category)/*.c))
TEST_OBJECTS=$(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SOURCES))
TEST_DEPS=$(TEST_OBJECTS:.o=.d)

.PHONY: all test clean

all: $(TARGET)

test: $(TEST_TARGETS)
	@-$(BASH) $(TEST_COMMAND)

clean:
	-$(RMDIR) $(BIN_DIR) $(BUILD_DIR)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(TEST_BIN_DIR):
	$(MKDIR) "$@"

define TEST_RULES
$(1)_OBJECTS=$(filter $(TEST_OBJ_DIR)/$(1)/%.o,$(TEST_OBJECTS)) $(filter-out $(MAIN_OBJ),$(OBJECTS))
$(TEST_BIN_DIR)/$(1)$(EXE): $$($(1)_OBJECTS) | $(TEST_BIN_DIR)
	$(CC) $$^ -o $$@

$(TEST_OBJ_DIR)/$(1)/%.o: $(TEST_DIR)/$(1)/%.c | $(TEST_OBJ_DIR)/$(1)
	$(CC) $(CFLAGS) -c $$< -o $$@

$(TEST_OBJ_DIR)/$(1):
	$(MKDIR) "$$@"
endef

$(foreach category,$(TEST_CATEGORIES),$(eval $(call TEST_RULES,$(category))))

-include $(DEPS) $(TEST_DEPS)
