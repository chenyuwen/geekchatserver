all: server client

server: server.o libserver.a mysql_connector.o libcrypto.a libjansson.a libcrc32.a libhashmap.a libmethods.a
	gcc $(filter %.o, $^) -lmysqlclient -L./ $(patsubst lib%.a,-l%, $(filter %.a,$^)) -o server

client: libjansson.a libcrc32.a libhashmap.a client.o
	gcc $(filter %.o, $^) -L./ $(patsubst lib%.a,-l%, $(filter %.a,$^)) -o client

libserver.a: packet.o users.o mysql_connector.o random_pool.o
	ar -r $@ $^

libmethods.a: $(patsubst %.c, %.o, $(wildcard ./methods/*.c))
	ar -r $@ $^

libcrc32.a: ./crc32/crc32.o
	ar -r $@ $^

libcrypto.a: ./crypto/sha256.o
	ar -r $@ $^

libjansson.a:: $(patsubst %.c, %.o, $(wildcard ./jansson/*.c))
	ar -r $@ $^

libhashmap.a: ./c_hashmap/hashmap.o
	ar -r $@ $^

mysql_connector.o: mysql_connector.c
	gcc -c $^ -o $@ -lmysqlclient

.PHONY : clean
clean:
	@rm server client *.o *.a ./crc32/*.o ./c_hashmap/*.o ./jansson/*.o ./methods/*.o

