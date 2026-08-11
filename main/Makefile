BUILD_TYPE ?= release

#  Msys/MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall -DUSEGLEW -I/mingw64/include/opencv4
LIBS=-lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2_mixer -lglfw3 -lglew32 -lglu32 -lopengl32 -lm -std=c++23 -pthread
CLEAN=rm -f *.exe *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall -Wno-deprecated-declarations  -DUSEGLEW -I/usr/include/opencv4
LIBS=-lSDL2main -lSDL2 -lSDL2_mixer -lglfw -lglew -framework Cocoa -framework OpenGL -framework IOKit -std=c++23 -pthread
#  Linux/Unix/Solaris
else
ifeq ($(BUILD_TYPE),release)
	CFLG=-O3 -Wall -I/usr/include/opencv4 -Wextra -std=c++23 -pthread
else
	CFLG=-O0 -g -DDEBUG -Wall -I/usr/include/opencv4 -Wextra -std=c++23 -pthread
endif
LIBS=-lSDL2 -lSDL2_mixer -lglfw -lGLU -lGL -lm
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a
endif

all: main.o

main.o: main.cpp point.hpp tree.hpp

.cpp.o:
	g++ -c $(CFLG) -I /usr/include/opencv4 $<

# Clean
clean:
	$(CLEAN)