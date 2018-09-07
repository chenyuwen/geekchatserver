
all:
	gcc server.c ./crc32/crc32.c ./c_hashmap/hashmap.c -o server
	gcc client.c ./crc32/crc32.c ./c_hashmap/hashmap.c -o client

.PHONY : clean
clean:
	rm server client

