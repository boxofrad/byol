parsing: *c *.h
	cc -std=c99 -Wall parsing.c mpc.c -o bin/parsing -ledit -lm
