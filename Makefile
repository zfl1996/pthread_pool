all:
	gcc pthread_pool_server.c -I../include -lpthread -o server
	gcc pthread_pool_client.c -o client

clean:
	-rm client -rf
	-rm server -rf
