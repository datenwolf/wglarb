TARGET64=x86_64-w64-mingw32
TARGET32=mingw32
CC64=-$(TARGET64)-gcc
CC32=-$(TARGET32)-gcc
CFLAGS=-static-libgcc -static-libstdc++ -I..
LIBS=-lopengl32 -lgdi32 -lkernel32

WINDRES64=$(TARGET64)-windres
WINDRES32=$(TARGET32)-windres

.PHONY: all

all: layered64.exe layered32.exe

layered64.exe: layered.c layered64_rc.o
	$(CC64) $(CFLAGS) -o layered64.exe layered.c layered64_rc.o ../wglarb.c $(LIBS) -Wl,--subsystem,windows

layered32.exe: layered.c layered32_rc.o
	$(CC32) -march=i686 $(CFLAGS) -o layered32.exe layered.c layered32_rc.o ../wglarb.c $(LIBS) -Wl,--subsystem,windows


layered64_rc.o: layered.rc manifest.xml
	$(WINDRES64) --input layered.rc --output layered64_rc.o

layered32_rc.o: layered.rc manifest.xml
	$(WINDRES32) --input layered.rc --output layered32_rc.o

