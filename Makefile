build:
	cc -Wall -Wextra -std=c99 -pedantic main.c -o main
test:
	cc -Wall -Wextra -std=c99 -pedantic test.c -o test && ./test && rm ./test
