CC=i686-w64-mingw32-gcc
CFLAGS=-static-libgcc -static-libstdc++ -I..
LIBS=-lopengl32 -lgdi32 -lkernel32

layered.exe: layered.c
	$(CC) $(CFLAGS) -o layered.exe layered.c ../wglarb.c $(LIBS)
