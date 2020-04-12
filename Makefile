#http://nuclear.mutantstargoat.com/articles/make/
#https://interrupt.memfault.com/blog/best-and-worst-gcc-clang-compiler-flags

src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -Wall -Wextra -Wpedantic \
		 -Wshadow -Wdouble-promotion \
		 -Wformat=2 -Wformat-truncation \
		 -Wundef \
		 -fno-common \
		 -Wconversion \
		 -ggdb -g3

LDFLAGS = 

jtshell: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) jtshell
