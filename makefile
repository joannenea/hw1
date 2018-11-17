all:
	gcc -o fork eric_fork.c
	gcc -o select eric_select.c

clean:
	rm fork select
