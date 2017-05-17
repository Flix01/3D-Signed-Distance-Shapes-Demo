#
# Cross Platform Makefile
# Compatible with Ubuntu 14.04.1 and Mac OS X
#

#CXX = g++
#CC = gcc

EXE = 3D_Signed_Distance_Shapes_Demo
OBJS = main.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lglut -lGL -lX11 -lm

	#CXXFLAGS = -I../../ `pkg-config --cflags glut`
	#CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -lglut

	CXXFLAGS = -I../../ -I/usr/local/include
	#CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), MINGW64_NT-6.3)
   ECHO_MESSAGE = "Windows"
   LIBS = -lglut32 -lgdi32 -lopengl32 -limm32

   #CXXFLAGS = -I../../ -I../libs/glut `pkg-config --cflags glut`
   #CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(CFLAGS) $(LIBS)

clean:
	rm $(EXE) $(OBJS)


