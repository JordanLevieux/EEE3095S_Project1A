.RECIPEPREFIX +=
CC = gcc
CFLAGS = -Wall -lm -lrt -lwiringPi

PROG = bin/*
OBJS = obj/*

default:
    mkdir -p bin obj
    $(CC) $(CFLAGS) -c src/GreenHouse.c -o obj/GreenHouse

run:
    sudo ./bin/Clock

clean:
    rm $(PROG) $(OBJS)
