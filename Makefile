# Compiler
CC = g++

# Paths
INCLUDE := ./include

COMMON       := ./src/common
COMMON_SRCS  := $(wildcard $(COMMON)/*.cpp)
COMMON_OBJS  := $(subst .cpp,.o,$(COMMON_SRCS))


SERVER		 := ./src/server
SERVER_SRCS  := $(wildcard $(SERVER)/*.cpp) $(COMMON_SRCS)
SERVER_OBJS  := $(subst .cpp,.o,$(SERVER_SRCS))


COMMANDER		:= ./src/commander
COMMANDER_SRCS  := $(wildcard $(COMMANDER)/*.cpp) $(COMMON_SRCS)
COMMANDER_OBJS  := $(subst .cpp,.o,$(COMMANDER_SRCS))


TARGET   := $(word 1, $(MAKECMDGOALS))
CXXFLAGS := -std=gnu++17 -g3 -Wall -Wextra 

# Compile options
CXXFLAGS += -I$(INCLUDE)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $^ -o $@

clean:
	@rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(COMMANDER_OBJS)
	@rm -f ./jobExecutorServer ./jobCommander ./progDelay

server: $(SERVER_OBJS)
	@rm -f ./jobExecutorServer.txt
	$(CC) $^ -o ./jobExecutorServer

commander: $(COMMANDER_OBJS)
	@rm -f ./jobExecutorServer.txt
	$(CC) $^ -o ./jobCommander

all: 
	make -s server
	make -s commander
#@gcc ./scripting/test_cases/progDelay.c -o progDelay

test_cases:
	@chmod u+x ./scripting/test_cases/scripts/custom/give_perms.sh
	@./scripting/test_cases/scripts/custom/give_perms.sh
	@make -s all > /dev/null 2>&1
	./scripting/test_cases/scripts/custom/run_given.sh
	./scripting/test_cases/scripts/custom/test.sh
	@make clean > /dev/null 2>&1