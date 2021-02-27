PRGNAME := routeptr

CXX := g++
CXXFLAGS := -O2 -c -Wno-unused-result

CPP := $(wildcard src/*)
OBJS := $(addprefix build/, $(patsubst src/%.cpp, %.o, $(CPP)))

build/%.o: src/%.cpp
	@mkdir -p build
	@echo CXX "$<"
	@$(CXX) -I./include -I./include/ev $(CXXFLAGS) -o "$@" "$<"

all: $(OBJS)
	@echo LD $(PRGNAME)
	@$(CXX) -o $(PRGNAME) $(OBJS) -pthread

clean:
	rm -rf $(PRGNAME) build