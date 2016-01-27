parsing: *.c *.h
	cc -std=c99 -Wall main.c lval.c mpc.c -o bin/parsing -ledit -lm
