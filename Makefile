# Chess Engine Make Commands:
#   make            build the optimized engine
#   make debug      build with sanitizers + symbols for bug hunting
#   make run        build (if needed) and run the engine
#   make clean      remove build artifacts

CXX      := clang++
TARGET   := chess_engine

SRCS     := board.cpp eval.cpp main.cpp moveGen.cpp nnue.cpp search.cpp
OBJS     := $(SRCS:.cpp=.o)
DEPS     := $(OBJS:.o=.d)

CXXFLAGS := -std=c++17 -Wall -Wextra -MMD -MP

RELFLAGS := -O3 -march=native -DNDEBUG

DBGFLAGS := -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer

CXXFLAGS += $(RELFLAGS)

.PHONY: all debug run clean

all: $(TARGET)

debug: CXXFLAGS := -std=c++17 -Wall -Wextra -MMD -MP $(DBGFLAGS)
debug: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
