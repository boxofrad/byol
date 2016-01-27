parsing: *c *.h
	cc -std=c99 parsing.c mpc.c -o bin/parsing -ledit -lm
