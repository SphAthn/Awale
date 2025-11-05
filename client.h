#ifndef CLIENT_H
#define CLIENT_H

#ifdef WIN32

#include <winsock2.h>

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <sys/select.h> /* select, fd_set, STDIN_FILENO */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF     "\r\n"
#define PORT     1977

#define BUF_SIZE 1024

/* The client implementation has internal (static) helpers; they should not be
 * declared in this header. This header exposes the Client type used by both
 * client and server code. */

typedef enum {
    STATE_FREE,        // pas en partie
    STATE_WAITING,     // a lancé un défi, attend réponse
    STATE_PLAYING      // en train de jouer une partie
} ClientState;

typedef struct {
    SOCKET sock;
    char name[BUF_SIZE];

    ClientState state;
    int game_id;       // index dans le tableau de parties, -1 si aucune
    int pending_with;  // index du client avec qui il a une demande en cours, -1 sinon
} Client;

#endif /* guard */
