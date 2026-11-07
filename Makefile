# JajaCC Makefile
# Copyright (C) 2026 student25-ch
# Licensed under the GNU General Public License v3.

.RECIPEPREFIX = >

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Isrc
TARGET   ?= jajacc

SRC = src/lexer.cpp src/parser.cpp src/codegen.cpp src/pe.cpp src/main.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
> $(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
> $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
> rm -f $(OBJ) $(TARGET)

.PHONY: all clean
