CC = gcc

BIN_DIR = bin/

UTIL_SRCS = util/socket_util.c util/string_util.c util/constant.c
UTIL_OBJS = $(UTIL_SRCS:.c=.o)

SERVER = $(BIN_DIR)ftp_server
SERVER_SRCS = server/ftp_server.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT = $(BIN_DIR)ftp_client
CLIENT_SRCS = client/ftp_client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJS) $(UTIL_OBJS)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(CC) -o $@ $^

$(CLIENT): $(CLIENT_OBJS) $(UTIL_OBJS)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(CC) -o $@ $^

clean:
	rm $(UTIL_OBJS)
	rm $(SERVER)
	rm $(SERVER_OBJS)
	rm $(CLIENT)
	rm $(CLIENT_OBJS)
