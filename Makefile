all: laba6

laba6: server.c client.c 
	gcc -Wall -Werror -Wextra -o server server.c 
	gcc -Wall -Werror -Wextra -o client client.c 

clean:
	rm -f client
	rm -f server