all:
	gcc -o eric_fork eric_fork.c
	gcc -o eric_select eric_select.c

clean:
	rm eric_fork eric_select
