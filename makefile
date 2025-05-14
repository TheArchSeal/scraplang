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
CFLAGS=-g -Wall -Wextra -I$(INC_DIR) -MMD -MP

TARGET=$(BIN_DIR)/smlc
SOURCES=$(wildcard $(SRC_DIR)/*.c)
OBJECTS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
MAIN_OBJ=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MAIN_SRC))
DEPS=$(OBJECTS:.o=.d)

TEST_CATEGORIES=$(patsubst $(TEST_DIR)/%/,%,$(wildcard $(TEST_DIR)/*/))
TEST_TARGETS=$(addprefix $(TEST_BIN_DIR)/,$(TEST_CATEGORIES))
TEST_SOURCES=$(foreach category,$(TEST_CATEGORIES),$(wildcard $(TEST_DIR)/$(category)/*.c))
TEST_OBJECTS=$(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SOURCES))
TEST_DEPS=$(TEST_OBJECTS:.o=.d)

.PHONY: all test clean

all: $(TARGET)

test: $(TEST_TARGETS)
	@-$(TEST_COMMAND)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(TEST_BIN_DIR):
	mkdir -p $@

define TEST_RULES
$(1)_OBJECTS=$(filter $(TEST_OBJ_DIR)/$(1)/%.o,$(TEST_OBJECTS)) $(filter-out $(MAIN_OBJ),$(OBJECTS))
$(TEST_BIN_DIR)/$(1): $$($(1)_OBJECTS) | $(TEST_BIN_DIR)
	$(CC) $$^ -o $$@

$(TEST_OBJ_DIR)/$(1)/%.o: $(TEST_DIR)/$(1)/%.c | $(TEST_OBJ_DIR)/$(1)
	$(CC) $(CFLAGS) -c $$< -o $$@

$(TEST_OBJ_DIR)/$(1):
	mkdir -p $$@
endef

$(foreach category,$(TEST_CATEGORIES),$(eval $(call TEST_RULES,$(category))))

-include $(DEPS) $(TEST_DEPS)
