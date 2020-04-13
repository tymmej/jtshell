#http://nuclear.mutantstargoat.com/articles/make/
#https://interrupt.memfault.com/blog/best-and-worst-gcc-clang-compiler-flags
#https://stackoverflow.com/questions/7004702/how-can-i-create-a-makefile-for-c-projects-with-src-obj-and-bin-subdirectories

TARGET = jtshell

CC = gcc
LINKER = gcc

CFLAGS = -Wall -Wextra -Wpedantic \
		 -Wshadow -Wdouble-promotion \
		 -Wformat=2 -Wformat-truncation \
		 -Wundef \
		 -fno-common \
		 -Wconversion \
		 -ggdb -g3

LDFLAGS = 

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
