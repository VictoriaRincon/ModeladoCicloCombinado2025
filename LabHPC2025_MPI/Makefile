# MPI C++ Compiler
CXX = mpic++

# Compiler flags
# -std=c++17: Use the C++17 standard
# -O3: High level of optimization
# -Wall: Enable all warnings
# -Iinclude: Tell compiler to look for headers in the 'include' directory
CXXFLAGS = -std=c++17 -O3 -Wall -Iinclude

# All source files for the optimizer executable
SRCS = src/main.cpp src/ga.cpp src/solucion.cpp src/unidad_termica.cpp

# Object files are created from source files and placed in the 'obj' directory
OBJS = $(patsubst src/%.cpp, obj/%.o, $(SRCS))

# The final executable will be placed in the 'bin' directory
TARGET = bin/optimizer

# The 'all' target is the default one, executed when you just run 'make'
# It depends on the main executable.
.PHONY: all
all: $(TARGET)

# Rule for linking the final executable from object files
# It depends on all object files and the existence of the 'bin' directory
$(TARGET): $(OBJS) | bin
	@echo "Linking executable..."
	$(CXX) $(OBJS) -o $@

# Pattern rule for compiling .cpp files into .o files
# It depends on the source file and the existence of the 'obj' directory
obj/%.o: src/%.cpp | obj
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to create directories. This runs only if the directory doesn't exist.
bin obj:
	@echo "Creating directory $@..."
	mkdir -p $@

# The 'clean' target removes all compiled files
.PHONY: clean
clean:
	@echo "Cleaning up project..."
	rm -rf bin obj logs scores resultados/*

