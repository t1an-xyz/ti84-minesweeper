NAME = MINSWPR
DESCRIPTION = "Minesweeper"
COMPRESSED = NO
ARCHIVED = NO
CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz -Wstring-plus-int

include $(shell cedev-config --makefile)