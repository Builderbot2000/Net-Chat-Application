s-talk: s-talk.o send.o receive.o list.o
	gcc -pthread s-talk.o send.o receive.o list.o -o s-talk

s-talk.o: s-talk.c send.h receive.h list.h
	gcc -c s-talk.c

send.o:
	gcc -c send.c

receive.o:
	gcc -c receive.c

clean:
	rm s-talk.o send.o receive.o s-talk