shell:
	cc -Wall -Wextra -std=c99 -pedantic -g -fsanitize=address,undefined shell.c -o shell
server:
	cc -Wall -Wextra -std=c99 -pedantic -g -fsanitize=address,undefined server.c -o server
test:
	cc -Wall -Wextra -std=c99 -pedantic test.c -o test && ./test && rm ./test
