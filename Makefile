#  Msys/MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall -DUSEGLEW -I/mingw64/include/opencv4 -std=c++23
LIBS=-lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2_mixer -lglfw3 -lglew32 -lglu32 -lopengl32 -lm
CLEAN=rm -f *.exe *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall -Wno-deprecated-declarations  -DUSEGLEW -I/usr/include/opencv4 -std=c++23
LIBS=-lSDL2main -lSDL2 -lSDL2_mixer -lglfw -lglew -framework Cocoa -framework OpenGL -framework IOKit
#  Linux/Unix/Solaris
else
CFLG=-O3 -Wall -I/usr/include/opencv4
LIBS=-lglfw -lGLU -lGL -lm -std=c++23 -pthread
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a stars
endif

SUBDIRS := $(wildcard */.)

all: $(SUBDIRS) stars
clean: $(SUBDIRS)
	$(CLEAN)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.cpp.o:
	g++ -c $(CFLG) -I /usr/include/opencv4 $<

.PHONY: $(SUBDIRS)

SRC = $(wildcard */*.cpp)
OBJ = $(SRC:.cpp=.o)

stars: $(OBJ)
	g++ $(CFLG) -o $@ $^ $(LIBS)