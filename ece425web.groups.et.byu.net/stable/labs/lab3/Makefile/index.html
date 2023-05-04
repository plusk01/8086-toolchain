#####################################################################
# ECEn 425 Lab 3 Makefile

lab3.bin:	lab3final.s
		nasm lab3final.s -o lab3.bin -l lab3.lst

lab3final.s:	clib.s myisr.s myinth.s primes.s
		cat clib.s myisr.s myinth.s primes.s > lab3final.s

myinth.s:	myinth.c
		cpp myinth.c myinth.i
		c86 -g myinth.i myinth.s

primes.s:	primes.c
		cpp primes.c primes.i
		c86 -g primes.i primes.s

clean:
		rm lab3.bin lab3.lst lab3final.s myinth.s myinth.i \
		primes.s primes.i
