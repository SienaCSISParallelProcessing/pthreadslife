# Makefile for serial life game
#
# Jim Teresco, CS 338, Williams College
# CS 341, Mount Holyoke College
# CS 400/CS 335, Siena College
#
CFILES=life.c
OFILES=$(CFILES:.c=.o)
CC=gcc -Wall

life:	$(OFILES)
	$(CC) -o life $(OFILES) -pthread

clean::
	/bin/rm -f life $(OFILES)
