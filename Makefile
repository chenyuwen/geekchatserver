all: server client

server: libjansson.a libcrc32.a libhashmap.a
	gcc server.c packet.c -L./ $(patsubst lib%.a,-l%, $^) -o server

client: libjansson.a libcrc32.a libhashmap.a
	gcc client.c -L./ $(patsubst lib%.a,-l%, $^) -o client

libcrc32.a: ./crc32/crc32.o
	ar -r $@ $^

libjansson.a:: $(patsubst %.c, %.o, $(wildcard ./jansson/*.c))
	ar -r $@ $^

libhashmap.a: ./c_hashmap/hashmap.o
	ar -r $@ $^

%.o : %.c%
	gcc -c $^ -o $@ -I

.PHONY : clean
clean:
	@rm server client *.a ./crc32/*.o ./c_hashmap/*.o ./jansson/*.o 

