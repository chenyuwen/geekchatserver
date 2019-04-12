cmake_minimum_required(VERSION 2.8)

project(geekchat)

set(GK_SRCS timer.c random_pool.c packet.c mlog.c messages.c users.c tokens.c friends.c server.c mysql_connector.c)

add_executable(gkserver ${GK_SRCS})

add_subdirectory(methods)
set(EXTLIB ${EXTLIB} methods)

add_subdirectory(crc32)
set(EXTLIB ${EXTLIB} crc32)

add_subdirectory(c_hashmap)
set(EXTLIB ${EXTLIB} hashmap)

add_subdirectory(jansson)
set(EXTLIB ${EXTLIB} jansson)

add_subdirectory(crypto)
set(EXTLIB ${EXTLIB} crypto)

set(EXTLIB ${EXTLIB} mysqlclient)
target_link_libraries(gkserver ${EXTLIB})

################################################################

set(CLIENT_SRC client.c)

add_executable(gkclient ${CLIENT_SRC})

target_link_libraries(gkclient ${EXTLIB})

