build:
	cc -Wall -Wextra -std=c99 -pedantic main.c -o main
asan:
	cc -Wall -Wextra -std=c99 -pedantic -g -fsanitize=address,undefined main.c -o main
test:
	cc -Wall -Wextra -std=c99 -pedantic test.c -o test && ./test && rm ./test
server:
	cc -Wall -Wextra -std=c99 -pedantic -g -fsanitize=address,undefined server.c -o server
