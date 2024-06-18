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
CXXFLAGS := -std=gnu++17 -g3 -Wall -Wextra -I$(INCLUDE) -pthread


all: dirs
	@make -s server
	@make -s commander
	@gcc ./tests/progDelay.c -o ./bin/progDelay

dirs:
	@mkdir -p bin
	@mkdir -p build
	@mkdir -p out
	
./build/%.o: $(COMMON)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@ -pthread

./build/%.o: $(SERVER)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@ -pthread

./build/%.o: $(COMMANDER)/%.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@ -pthread

clean:
	@rm -rf ./bin/ ./build/

server: dirs $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o ./bin/jobExecutorServer -pthread

commander: dirs $(COMMANDER_OBJS)
	$(CC) $(COMMANDER_OBJS) -o ./bin/jobCommander


IP ?= linux01.di.uoa.gr
PORT_PREFIX ?= 123
BUFFER_SIZE ?= 8
THREAD_POOL_SIZE ?= 4
USERNAME ?= sdi2100203

ssh_setup:
	-ssh-keygen || true
	ssh-copy-id $(USERNAME)@$(IP)

test_cases:
	@chmod u+x ./tests/scripts/custom/give_perms.sh
	@./tests/scripts/custom/give_perms.sh
	@./tests/scripts/custom/run_given.sh $(IP) $(PORT_PREFIX) $(BUFFER_SIZE) $(THREAD_POOL_SIZE)
	@echo Running: test.sh
	@-./tests/scripts/custom/test.sh $(IP) $(PORT_PREFIX) $(BUFFER_SIZE) $(THREAD_POOL_SIZE) > ./tests/results/results_test.txt 2>&1 || true