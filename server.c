#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "awale.h"

static Game games[MAX_GAMES];

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   int actual = 0;
   int max = sock;

   Client clients[MAX_CLIENTS];
   for (int g = 0; g < MAX_GAMES; ++g) {
      games[g].used = 0;
   }

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock, "", STATE_FREE, -1, -1 };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0) {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               } else {
                  handle_client_message(clients, i, actual, buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

/* Helpers */
static void trim_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[--len] = '\0';
    }
}

static int find_client_by_name(Client *clients, int actual, const char *name)
{
    for (int i = 0; i < actual; i++) {
        if (strcmp(clients[i].name, name) == 0) return i;
    }
    return -1;
}

static void handle_client_message(Client *clients, int idx, int actual, char *buffer)
{
    Client *c = &clients[idx];

    if (buffer[0] != '/') {
        // Not a command, treat as chat
        send_message_to_all_clients(clients, *c, actual, buffer, 0);
        return;
    }

    if (strncmp(buffer, "/list", 5) == 0) {
        send_user_list(clients, idx, actual);
    } else if (strncmp(buffer, "/challenge ", 11) == 0) {
        handle_challenge(clients, idx, actual, buffer + 11);
    } else if (strncmp(buffer, "/accept ", 8) == 0) {
        handle_accept(clients, idx, actual, buffer + 8);
    } else if (strncmp(buffer, "/refuse ", 8) == 0) {
        handle_refuse(clients, idx, actual, buffer + 8);
    } else if (strncmp(buffer, "/move ", 6) == 0) {
        handle_move(clients, idx, buffer + 6);
    } else if (strncmp(buffer, "/help", 5) == 0) {
        char msg[BUF_SIZE];
        snprintf(msg, sizeof msg,
            "Available commands:" CRLF
            "/list              - Show connected users" CRLF
            "/challenge <user>  - Challenge another player" CRLF
            "/accept <user>     - Accept a challenge" CRLF
            "/refuse <user>     - Refuse a challenge" CRLF
            "/move <n>          - Play a move (depending on the game)" CRLF
            "/help              - Show this help message" CRLF);
        write_client(c->sock, msg);
    } else {
        char msg[BUF_SIZE];
        snprintf(msg, sizeof msg, "Unknown command: '%s'" CRLF "Type /help for available commands." CRLF, buffer);
        write_client(c->sock, msg);
    }
}


static void send_user_list(Client *clients, int idx, int actual)
{
    char msg[BUF_SIZE];
    msg[0] = '\0';
    strcat(msg, "Users online:\n");
    for (int i = 0; i < actual; i++) {
        strcat(msg, "- ");
        strcat(msg, clients[i].name);
        strcat(msg, "\n");
    }
    write_client(clients[idx].sock, msg);
}

static void handle_challenge(Client *clients, int idx, int actual, const char *target_name)
{
    int j = find_client_by_name(clients, actual, target_name);
    if (j == -1) {
        write_client(clients[idx].sock, "Unknown user.\n");
        return;
    }
    if (idx == j) {
        write_client(clients[idx].sock, "You cannot challenge yourself.\n");
        return;
    }

    clients[idx].state = STATE_WAITING;
    clients[idx].pending_with = j;
    clients[j].pending_with = idx;

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s challenged you. Type /accept %s or /refuse %s\n",
             clients[idx].name, clients[idx].name, clients[idx].name);
    write_client(clients[j].sock, msg);

    write_client(clients[idx].sock, "Challenge sent.\n");
}

static int create_game(Game *games, int max_games, int idxA, int idxB)
{
    for (int g = 0; g < max_games; g++) {
        if (!games[g].used) {
            games[g].used = 1;
            initialiser_jeu(&games[g].awale);

            // random qui est South/North
            int r = rand() % 2;
            if (r == 0) {
                games[g].player_south = idxA;
                games[g].player_north = idxB;
            } else {
                games[g].player_south = idxB;
                games[g].player_north = idxA;
            }
            return g;
        }
    }
    return -1; // plus de place
}

void handle_accept(Client *clients, int idx, int actual, const char *challenger_name)
{
    char name[BUF_SIZE];

    /* On copie le nom reçu et on enlève les \r\n éventuels */
    strncpy(name, challenger_name, BUF_SIZE - 1);
    name[BUF_SIZE - 1] = '\0';
    trim_newline(name);

    int j = find_client_by_name(clients, actual, name);
    if (j == -1) {
        write_client(clients[idx].sock, "Unknown user." CRLF);
        return;
    }

    /* Vérifier qu'il y a bien un défi en cours entre idx et j */
    if (clients[idx].pending_with != j || clients[j].pending_with != idx) {
        write_client(clients[idx].sock, "No pending challenge with this user." CRLF);
        return;
    }

    /* Créer la partie */
    int game_id = create_game(games, MAX_GAMES, idx, j);
    if (game_id < 0) {
        write_client(clients[idx].sock, "Cannot create game (server full)." CRLF);
        write_client(clients[j].sock,   "Cannot create game (server full)." CRLF);
        clients[idx].pending_with = -1;
        clients[j].pending_with = -1;
        clients[idx].state = STATE_FREE;
        clients[j].state = STATE_FREE;
        return;
    }

    clients[idx].state     = STATE_PLAYING;
    clients[idx].game_id   = game_id;
    clients[j].state       = STATE_PLAYING;
    clients[j].game_id     = game_id;
    clients[idx].pending_with = -1;
    clients[j].pending_with = -1;

    char msg[BUF_SIZE];

    /* Annonce de la partie */
    snprintf(msg, sizeof msg,
             "Game %d created between %s and %s" CRLF,
             game_id, clients[idx].name, clients[j].name);
    write_client(clients[idx].sock, msg);
    write_client(clients[j].sock, msg);

    /* Dire à chacun s’il est South ou North et qui commence */
    if (games[game_id].player_south == idx) {
        write_client(clients[idx].sock, "You are South, you start." CRLF);
        write_client(clients[j].sock,   "You are North, you play second." CRLF);
    } else {
        write_client(clients[j].sock,   "You are South, you start." CRLF);
        write_client(clients[idx].sock, "You are North, you play second." CRLF);
    }

   awale_format_board(&games[game_id].awale, msg, sizeof msg);
   write_client(clients[idx].sock, msg);
   write_client(clients[j].sock, msg);
}

/* Minimal stub implementations for game-related handlers.
 * These currently just notify the requesting client that the command
 * is not yet implemented. They are defined with external linkage to
 * match the prototypes in server.h so the program links cleanly.
 */
void handle_refuse(Client *clients, int idx, int actual, const char *from_name)
{
   char msg[BUF_SIZE];
   snprintf(msg, sizeof msg, "Refuse from '%s' not implemented yet" CRLF, from_name);
   write_client(clients[idx].sock, msg);
}

void handle_move(Client *clients, int idx, const char *move_args)
{
   char msg[BUF_SIZE];
   snprintf(msg, sizeof msg, "Move '%s' not implemented yet" CRLF, move_args);
   write_client(clients[idx].sock, msg);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
