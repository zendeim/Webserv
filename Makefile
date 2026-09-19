# Configuration ------------------------------- #
NAME = webserv
VPATH := $(shell find src -type d)
SRC = main.cpp
SRC_TEST = test.cpp
LDLIBS =
ARG = assets/configs/default.conf

# Defaults ------------------------------------ #
.DEFAULT_GOAL := re # This is intentional. Project is small, compilation is fast
RM := rm -f
BUILD_PATH = build
OBJ_PATH = $(BUILD_PATH)/obj
BIN = build/$(NAME)
OBJ = $(addprefix $(OBJ_PATH)/, $(SRC:.cpp=.o))

# Flags --------------------------------------- #
CXX = clang++
CPPFLAGS = $(addprefix -I,$(VPATH))
CXXFLAGS = -Wall -Wextra -O2 -fno-exceptions
LDFLAGS = -nostdlib++ # Insane that just linking with stdlib++ accrues a 70kb allocation for exception pools (WITH EXCEPTIONS DISABLED!)
DEBUG = -g -DDEBUG_MODE -O0 -Wpedantic -Wcast-qual -Wfloat-equal -Wswitch-default -Wsign-conversion
ASAN = -fsanitize=address,undefined,leak -fno-omit-frame-pointer
TSAN = -fsanitize=thread -fno-omit-frame-pointer
FAST = -march=native -O3 -ffast-math -fstrict-aliasing

TEST = -O3 -march=native
# Pattern Rules: Compilation ------------------ #
$(OBJ_PATH)/%.o: %.cpp | $(OBJ_PATH)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Linking
$(BIN): $(OBJ) | $(BUILD_PATH)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

# Directory
$(OBJ_PATH):
	@mkdir -p $@
$(BUILD_PATH):
	@mkdir -p $@

# Phonies ------------------------------------- #
all: $(BIN)

run:
	clear
	./$(BIN) $(ARG)

test: CXXFLAGS += $(TEST)
test:
	$(MAKE) clean
	$(MAKE) SRC="$(SRC_TEST)" CXXFLAGS="$(CXXFLAGS)" all

vrun:
	clear
	valgrind ./$(BIN) $(ARG)

compdb: | $(BUILD_PATH)
	$(RM) $(BUILD_PATH)/compile_commands.json
	bear --output $(BUILD_PATH)/compile_commands.json -- $(MAKE) clean asan

clean:
	$(RM) -r $(OBJ_PATH)

fclean: clean
	$(RM) $(BIN)

re: fclean all

debug: CXXFLAGS += $(DEBUG)
debug: clean $(BIN)

asan: CXXFLAGS += $(DEBUG) $(ASAN)
asan: LDFLAGS += $(ASAN)
asan: clean $(BIN)

tsan: CXXFLAGS += $(DEBUG) $(TSAN)
tsan: LDFLAGS += $(TSAN)
tsan: clean $(BIN)

fast: CXXFLAGS += $(FAST)
#fast: LDFLAGS += -flto
fast: clean $(BIN)

.PHONY: all run vrun compdb clean fclean re debug asan tsan fast test
