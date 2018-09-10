all: server client

server: server.o packet.o libjansson.a libcrc32.a libhashmap.a libhello.a
	gcc $(filter %.o, $^) -L./ $(patsubst lib%.a,-l%, $(filter %.a,$^)) -o server

client: libjansson.a libcrc32.a libhashmap.a client.o
	gcc $(filter %.o, $^) -L./ $(patsubst lib%.a,-l%, $(filter %.a,$^)) -o client

libhello.a: com.hello.request.o
	ar -r $@ $^

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
	@rm server client *.o *.a ./crc32/*.o ./c_hashmap/*.o ./jansson/*.o

