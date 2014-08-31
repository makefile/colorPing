CC	= gcc 
#CFLAGS  = -O2 -Wall -D_GNU_SOURCE
CFLAGS = -Wall 
SOURCES	= myPing.c util.h util.c
OBJECTS	= ${SOURCES:.c=.o}

OUT	= myPing
LIBS = -lpthread
#LIBS	= -lungif -L/usr/X11R6/lib -ljpeg -lpng

all: $(OUT)
	@echo Build DONE.

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)  $(OUT)

distclean: clean
