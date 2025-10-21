# ---- Makefile (simple, auto-détection des .c du dossier) ----

# Config
CC      ?= gcc
CSTD    ?= -std=c11
CFLAGS  ?= -O2 -Wall -Wextra -pedantic $(CSTD)
LDFLAGS ?=
TARGET  ?= awale

# Sources/objets (tous les .c du dossier)
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

# Règle par défaut
all: $(TARGET)

# Edition de liens
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilation de chaque .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Lancement rapide
run: $(TARGET)
	./$(TARGET)

# Nettoyage
clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all run debug release clean
# ---- fin Makefile ----