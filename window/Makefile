#  Msys/MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall -DUSEGLEW -I/mingw64/include/opencv4 -std=c++20
LIBS=-lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2_mixer -lglfw3 -lglew32 -lglu32 -lopengl32 -lm
CLEAN=rm -f *.exe *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall -Wno-deprecated-declarations  -DUSEGLEW -I/usr/include/opencv4 -std=c++20
LIBS=-lSDL2main -lSDL2 -lSDL2_mixer -lglfw -lglew -framework Cocoa -framework OpenGL -framework IOKit
#  Linux/Unix/Solaris
else
CFLG=-O3 -Wall -I/usr/include/opencv4 -std=c++20
LIBS=-lSDL2 -lSDL2_mixer -lglfw -lGLU -lGL -lm
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a
endif

all: window.o

window.o: window.cpp window.hpp

.cpp.o:
	g++ -c $(CFLG) -I /usr/include/opencv4 $<

# Clean
clean:
	$(CLEAN)