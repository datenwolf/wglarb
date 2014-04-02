TARGET=x86_64-w64-mingw32
CC=-$(TARGET)-gcc
CFLAGS=-static-libgcc -static-libstdc++ -I..
LIBS=-lopengl32 -lgdi32 -lkernel32

WINDRES=$(TARGET)-windres

layered.exe: layered.c layered_rc.o
	$(CC) $(CFLAGS) -o layered.exe layered.c layered_rc.o ../wglarb.c $(LIBS) -Wl,--subsystem,windows


layered_rc.o: layered.rc
	$(WINDRES) --input layered.rc --output layered_rc.o
