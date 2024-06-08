# Compiler
CC = g++

# Paths
INCLUDE := ./include

COMMON       := ./src/common
COMMON_SRCS  := $(wildcard $(COMMON)/*.cpp)
COMMON_OBJS  := $(patsubst $(COMMON)/%.cpp,./build/%.o,$(COMMON_SRCS))


SERVER         := ./src/server
SERVER_SRCS  := $(wildcard $(SERVER)/*.cpp)
SERVER_OBJS  := $(patsubst $(SERVER)/%.cpp,./build/%.o,$(SERVER_SRCS)) $(COMMON_OBJS)


COMMANDER        := ./src/commander
COMMANDER_SRCS  := $(wildcard $(COMMANDER)/*.cpp)
COMMANDER_OBJS  := $(patsubst $(COMMANDER)/%.cpp,./build/%.o,$(COMMANDER_SRCS)) $(COMMON_OBJS)


TARGET   := $(word 1, $(MAKECMDGOALS))
CXXFLAGS := -std=gnu++17 -g3 -Wall -Wextra -I$(INCLUDE)


./build/%.o: $(COMMON)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@

./build/%.o: $(SERVER)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@

./build/%.o: $(COMMANDER)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@

clean:
	@rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(COMMANDER_OBJS)
	@rm -f ./bin/jobExecutorServer ./bin/jobCommander ./bin/progDelay

server: $(SERVER_OBJS)
	$(CC) $^ -o ./bin/jobExecutorServer

commander: $(COMMANDER_OBJS)
	$(CC) $^ -o ./bin/jobCommander

all: 
	@make -s server
	@make -s commander
#@gcc ./scripting/test_cases/progDelay.c -o progDelay

test_cases:
	@chmod u+x ./scripting/test_cases/scripts/custom/give_perms.sh
	@./scripting/test_cases/scripts/custom/give_perms.sh
	@make -s all > /dev/null 2>&1
	./scripting/test_cases/scripts/custom/run_given.sh
	./scripting/test_cases/scripts/custom/test.sh
	@make clean > /dev/null 2>&1