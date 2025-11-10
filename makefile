# ---- Makefile pour serveur / client Awale ----

CC      ?= gcc
CSTD    ?= -std=c11
CFLAGS  ?= -O2 -Wall -Wextra -pedantic $(CSTD)
LDFLAGS ?=

SERVER  := server
CLIENT  := client

# Sources explicites
SERVER_SRCS := server.c awale.c
CLIENT_SRCS := client.c

SERVER_OBJS := $(SERVER_SRCS:.c=.o)
CLIENT_OBJS := $(CLIENT_SRCS:.c=.o)

# Règle par défaut : tout construire
all: $(SERVER) $(CLIENT)

# Link serveur
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Link client
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation de chaque .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	$(RM) $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER) $(CLIENT)

.PHONY: all run-server run-client clean
# ---- fin Makefile ----