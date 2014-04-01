CC=i686-w64-mingw32-gcc
CFLAGS=-static-libgcc -static-libstdc++ -I..
LIBS=-lopengl32 -lgdi32 -lkernel32

WINDRES=i686-w64-mingw32-windres

layered.exe: layered.c layered.rc.o
	$(CC) $(CFLAGS) -o layered.exe layered.c layered.rc.o ../wglarb.c $(LIBS) -Wl,--subsystem,windows


layered.rc.o: layered.rc
	$(WINDRES) --input layered.rc --output layered.rc.o
