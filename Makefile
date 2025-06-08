CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -MMD -I$(SRC_DIR)
SRC_DIR = src

CFILES = $(shell find $(SRC_DIR) -name "*.c")
OBJS = $(CFILES:.c=.o)
CPPFILES = $(shell find $(SRC_DIR) -name "*.cpp")
OBJS += $(CPPFILES:.cpp=.o)
CCFILES = $(shell find $(SRC_DIR) -name "*.cc")
OBJS += $(CCFILES:.cc=.o)

DEPENDS = ${OBJS:.o=.d}

compiler: $(OBJS)
	$(CXX) -o $@ $^

-include $(DEPENDS)

.PHONY: clean test format
clean:
	 rm -f $(OBJS)
	 rm -f $(DEPENDS)
	 rm -f compiler

test:
	python3 sp25-tests/test.py $(shell git branch --show-current) .

format:
	find $(SRC_DIR) \( -name "*.cpp" -o -name "*.hpp" -o -name "*.def" \) | xargs clang-format -i
