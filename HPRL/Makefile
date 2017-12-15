CC=g++
LD=g++
INCLUDES=-I/usr/local/include -I/usr/local/include/FL -I./art
CFLAGS=-g $(INCLUDES)
LDFLAGS=-L/usr/local/lib -lGL -lfltk -lfltk_gl -lm -lXext -lXfixes -lX11 -ldl -lXinerama -lXcursor -lXrender

all: cthonicexpedition

cthonicexpedition : main.o Game.o MapCell.o Overworld.o Movement.o Energy.o DrawWindow.o lodepng.o glf.o
	$(LD) $^ $(LDFLAGS) -o cthonicexpedition

clean:
	rm -f cthonicexpedition *.o

