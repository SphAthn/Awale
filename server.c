#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "awale.h"

static Game games[MAX_GAMES];
PlayerStats stats[MAX_STATS];
int stats_count = 0;

static void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

static void end(void)
{
    save_stats("stats.txt");
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
    load_stats("stats.txt");

    Client clients[MAX_CLIENTS];
    for (int g = 0; g < MAX_GAMES; ++g)
    {
        games[g].used = 0;
    }

    fd_set rdfs;

    while (1)
    {
        int i = 0;
        FD_ZERO(&rdfs);

        /* add STDIN_FILENO */
        FD_SET(STDIN_FILENO, &rdfs);

        /* add the connection socket */
        FD_SET(sock, &rdfs);

        /* add socket of each client */
        for (i = 0; i < actual; i++)
        {
            FD_SET(clients[i].sock, &rdfs);
        }

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        /* something from standard input : i.e keyboard */
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            /* stop process when type on keyboard */
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            /* new client */
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            /* after connecting the client sends its name */
            if (read_client(csock, buffer) == -1)
            {
                /* disconnected */
                continue;
            }

            /* what is the new maximum fd ? */
            max = csock > max ? csock : max;

            FD_SET(csock, &rdfs);

            Client c = {csock, "", STATE_FREE, -1, -1};
            strncpy(c.name, buffer, BUF_SIZE - 1);
            clients[actual] = c;
            actual++;

            /* Welcome message */
            char welcome[BUF_SIZE];
            snprintf(welcome, sizeof(welcome),
                     "Hi %s!\n"
                     "Welcome to the Awale server!\n"
                     "Type /help to see available commands.\n"
                     "Start by typing /list to see who’s online.\n\n",
                     c.name);
            write_client(c.sock, welcome);
        }
        else
        {
            int i = 0;
            for (i = 0; i < actual; i++)
            {
                /* a client is talking */
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);
                    /* client disconnected */
                    if (c == 0)
                    {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);
                    }
                    else
                    {
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
void load_stats(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
        return; // fichier inexistant → stats vides

    stats_count = 0;
    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '#')
            continue; // ignorer les commentaires
        if (sscanf(line, "%s %d %d %d %d",
                   stats[stats_count].name,
                   &stats[stats_count].games_played,
                   &stats[stats_count].wins,
                   &stats[stats_count].draws,
                   &stats[stats_count].losses) == 5)
        {
            stats_count++;
            if (stats_count >= MAX_STATS)
                break;
        }
    }
    fclose(f);
}

void save_stats(const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f)
        return;

    for (int i = 0; i < stats_count; i++)
    {
        fprintf(f, "%s %d %d %d %d\n",
                stats[i].name,
                stats[i].games_played,
                stats[i].wins,
                stats[i].draws,
                stats[i].losses);
    }
    fclose(f);
}

void update_stats(const char *south, const char *north, int result)
{
    // result : 0 = draw, 1 = south wins, 2 = north wins
    PlayerStats *p_s = NULL;
    PlayerStats *p_n = NULL;

    // Chercher ou créer les joueurs
    for (int i = 0; i < stats_count; i++)
    {
        if (strcmp(stats[i].name, south) == 0)
            p_s = &stats[i];
        if (strcmp(stats[i].name, north) == 0)
            p_n = &stats[i];
    }

    if (!p_s && stats_count < MAX_STATS)
    {
        p_s = &stats[stats_count++];
        strncpy(p_s->name, south, BUF_SIZE - 1);
        p_s->games_played = 0;
        p_s->wins = 0;
        p_s->draws = 0;
        p_s->losses = 0;
    }
    if (!p_n && stats_count < MAX_STATS)
    {
        p_n = &stats[stats_count++];
        strncpy(p_n->name, north, BUF_SIZE - 1);
        p_n->games_played = 0;
        p_n->wins = 0;
        p_n->draws = 0;
        p_n->losses = 0;
    }

    // Incrémenter les stats
    p_s->games_played++;
    p_n->games_played++;

    if (result == 0)
    { // égalité
        p_s->draws++;
        p_n->draws++;
    }
    else if (result == 1)
    { // sud gagne
        p_s->wins++;
        p_n->losses++;
    }
    else if (result == 2)
    { // nord gagne
        p_n->wins++;
        p_s->losses++;
    }

    // Sauvegarder
    save_stats("stats.txt");
}

static void trim_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[--len] = '\0';
    }
}

static int find_client_by_name(Client *clients, int actual, const char *name)
{
    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void handle_resign(Client *clients, int idx)
{
    Client *c = &clients[idx];

    if (c->state != STATE_PLAYING || c->game_id < 0)
    {
        write_client(c->sock, "You are not in a game.\n");
        return;
    }

    Game *g = &games[c->game_id];
    int opponent_idx = (g->player_south == idx) ? g->player_north : g->player_south;

    char msg[BUF_SIZE];
    snprintf(msg, sizeof msg, "%s resigned. You win!\n", c->name);
    write_client(clients[opponent_idx].sock, msg);
    write_client(c->sock, "You resigned. Game over.\n");

    // mettre à jour les stats
    int result = (g->player_south == idx) ? 2 : 1; // l’autre gagne
    update_stats(clients[g->player_south].name,
                 clients[g->player_north].name,
                 result);

    // remettre les états des joueurs
    clients[g->player_south].state = STATE_FREE;
    clients[g->player_south].game_id = -1;
    clients[g->player_north].state = STATE_FREE;
    clients[g->player_north].game_id = -1;

    // libérer la partie
    g->used = 0;
}

static void handle_client_message(Client *clients, int idx, int actual, char *buffer)
{
    Client *c = &clients[idx];

    if (buffer[0] != '/')
    {
        // Not a command, treat as chat
        send_message_to_all_clients(clients, *c, actual, buffer, 0);
        return;
    }

    if (strncmp(buffer, "/list", 5) == 0)
    {
        send_user_list(clients, idx, actual);
    }
    else if (strncmp(buffer, "/games", 6) == 0)
    {
        handle_games(clients, idx);
    }
    else if (strncmp(buffer, "/challenge ", 11) == 0)
    {
        handle_challenge(clients, idx, actual, buffer + 11);
    }
    else if (strncmp(buffer, "/accept ", 8) == 0)
    {
        handle_accept(clients, idx, actual, buffer + 8);
    }
    else if (strncmp(buffer, "/refuse ", 8) == 0)
    {
        handle_refuse(clients, idx, actual, buffer + 8);
    }
    else if (strncmp(buffer, "/move ", 6) == 0)
    {
        handle_move(clients, idx, buffer + 6);
    }
    else if (strncmp(buffer, "/stats", 6) == 0)
    {
        handle_stats(&clients[idx]);
    }
    else if (strncmp(buffer, "/resign", 7) == 0)
    {
        handle_resign(clients, idx);
    }
    else if (strncmp(buffer, "/help", 5) == 0)
    {
        char msg[BUF_SIZE];
        snprintf(msg, sizeof msg,
                 "Available commands:" CRLF
                 "/list              - Show connected users" CRLF
                 "/games             - List active games" CRLF
                 "/stats             - Show your statistics and top players" CRLF
                 "/challenge <user>  - Challenge another player" CRLF
                 "/accept <user>     - Accept a challenge" CRLF
                 "/refuse <user>     - Refuse a challenge" CRLF
                 "In a game:" CRLF
                 "/move <n>          - Play a move <1-6>" CRLF
                 "/resign            - Resign from the current game" CRLF
                 "/help              - Show this help message" CRLF);
        write_client(c->sock, msg);
    }
    else
    {
        char msg[BUF_SIZE];
        snprintf(msg, sizeof msg, "Unknown command: '%s'" CRLF "Type /help for available commands." CRLF, buffer);
        write_client(c->sock, msg);
    }
}

void handle_stats(Client *c)
{
    char msg[BUF_SIZE];
    msg[0] = '\0';

    // 1. Stats du joueur connecté
    PlayerStats *p = NULL;
    for (int i = 0; i < stats_count; i++)
    {
        if (strcmp(stats[i].name, c->name) == 0)
        {
            p = &stats[i];
            break;
        }
    }

    if (p)
    {
        char line[128];
        snprintf(line, sizeof(line),
                 "Your stats: %d games, %d wins, %d draws, %d losses\n",
                 p->games_played, p->wins, p->draws, p->losses);
        strncat(msg, line, BUF_SIZE - strlen(msg) - 1);
    }
    else
    {
        strcat(msg, "You have no recorded stats yet.\n");
    }

    // 2. Top 10 players par nombre de victoires
    // Copier les stats pour trier sans modifier l'original
    PlayerStats top[stats_count];
    memcpy(top, stats, stats_count * sizeof(PlayerStats));

    // Tri simple (bubble sort ou qsort)
    for (int i = 0; i < stats_count - 1; i++)
    {
        for (int j = i + 1; j < stats_count; j++)
        {
            if (top[j].wins > top[i].wins)
            {
                PlayerStats tmp = top[i];
                top[i] = top[j];
                top[j] = tmp;
            }
        }
    }

    strcat(msg, "\nTop players:\n");
    int limit = stats_count < 10 ? stats_count : 10;
    for (int i = 0; i < limit; i++)
    {
        char line[128];
        snprintf(line, sizeof(line), "%d. %s: %d wins (%d games)\n",
                 i + 1, top[i].name, top[i].wins, top[i].games_played);
        strncat(msg, line, BUF_SIZE - strlen(msg) - 1);
    }

    write_client(c->sock, msg);
}

static void handle_games(Client *clients, int idx)
{
    char msg[BUF_SIZE];
    msg[0] = '\0';
    strcat(msg, "Active games:\n");
    int found = 0;
    for (int g = 0; g < MAX_GAMES; g++)
    {
        if (games[g].used)
        {
            found = 1;
            const char *south_name = clients[games[g].player_south].name;
            const char *north_name = clients[games[g].player_north].name;
            char line[BUF_SIZE];
            snprintf(line, sizeof(line), "- Game %d: South=%s vs North=%s\n", g, south_name, north_name);
            strcat(msg, line);
        }
    }
    if (!found)
    {
        strcat(msg, "No active games.\n");
    }
    write_client(clients[idx].sock, msg);
}

static void send_user_list(Client *clients, int idx, int actual)
{
    char msg[BUF_SIZE];
    char line[BUF_SIZE];
    msg[0] = '\0';
    strcat(msg, "Users online:\n");
    for (int i = 0; i < actual; i++)
    {
        const char *state_str;
        switch (clients[i].state)
        {
        case STATE_FREE:
            state_str = "free";
            break;
        case STATE_WAITING:
            state_str = "waiting";
            break;
        case STATE_PLAYING:
            state_str = "playing";
            break;
        default:
            state_str = "unknown";
            break;
        }
        snprintf(line, sizeof(line), "- %s (%s)\n", clients[i].name, state_str);
        strcat(msg, line);
    }
    write_client(clients[idx].sock, msg);
}

static void handle_challenge(Client *clients, int idx, int actual, const char *target_name)
{
    int j = find_client_by_name(clients, actual, target_name);
    if (clients[idx].state != STATE_FREE)
    {
        write_client(clients[idx].sock, "You cannot issue a challenge while not free.\n");
        return;
    }
    if (j == -1)
    {
        write_client(clients[idx].sock, "Unknown user.\n");
        return;
    }
    if (idx == j)
    {
        write_client(clients[idx].sock, "You cannot challenge yourself.\n");
        return;
    }
    if (clients[j].state != STATE_FREE)
    {
        write_client(clients[idx].sock, "User is not available for a challenge.\n");
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
    for (int g = 0; g < max_games; g++)
    {
        if (!games[g].used)
        {
            games[g].used = 1;
            initialiser_jeu(&games[g].awale);

            // random qui est South/North
            int r = rand() % 2;
            if (r == 0)
            {
                games[g].player_south = idxA;
                games[g].player_north = idxB;
            }
            else
            {
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
    if (j == -1)
    {
        write_client(clients[idx].sock, "Unknown user." CRLF);
        return;
    }

    /* Vérifier qu'il y a bien un défi en cours entre idx et j */
    if (clients[idx].pending_with != j || clients[j].pending_with != idx)
    {
        write_client(clients[idx].sock, "No pending challenge with this user." CRLF);
        return;
    }

    /* Créer la partie */
    int game_id = create_game(games, MAX_GAMES, idx, j);
    if (game_id < 0)
    {
        write_client(clients[idx].sock, "Cannot create game (server full)." CRLF);
        write_client(clients[j].sock, "Cannot create game (server full)." CRLF);
        clients[idx].pending_with = -1;
        clients[j].pending_with = -1;
        clients[idx].state = STATE_FREE;
        clients[j].state = STATE_FREE;
        return;
    }

    clients[idx].state = STATE_PLAYING;
    clients[idx].game_id = game_id;
    clients[j].state = STATE_PLAYING;
    clients[j].game_id = game_id;
    clients[idx].pending_with = -1;
    clients[j].pending_with = -1;

    char msg[BUF_SIZE];

    /* Annonce de la partie */
    snprintf(msg, sizeof msg,
             "Game %d created between %s and %s\r\n" CRLF,
             game_id, clients[idx].name, clients[j].name);
    snprintf(msg, sizeof msg,
             "Play with /move <n>, where n is 1-6 on your side (left to right)." CRLF);
    write_client(clients[idx].sock, msg);
    write_client(clients[j].sock, msg);

    /* Dire à chacun s’il est South ou North et qui commence */
    if (games[game_id].player_south == idx)
    {
        write_client(clients[idx].sock, "You are South, you start." CRLF);
        write_client(clients[j].sock, "You are North, you play second." CRLF);
    }
    else
    {
        write_client(clients[j].sock, "You are South, you start." CRLF);
        write_client(clients[idx].sock, "You are North, you play second." CRLF);
    }

    awale_format_board(&games[game_id].awale, msg, sizeof msg);
    write_client(clients[idx].sock, msg);
    write_client(clients[j].sock, msg);
}

void handle_refuse(Client *clients, int idx, int actual, const char *from_name)
{
    char name[BUF_SIZE];

    /* On copie le nom reçu et on enlève les \r\n éventuels */
    strncpy(name, from_name, BUF_SIZE - 1);
    name[BUF_SIZE - 1] = '\0';
    trim_newline(name);

    int j = find_client_by_name(clients, actual, name);
    if (j == -1)
    {
        write_client(clients[idx].sock, "Unknown user." CRLF);
        return;
    }

    /* Vérifier qu'il y a bien un défi en cours entre idx et j */
    if (clients[idx].pending_with != j || clients[j].pending_with != idx)
    {
        write_client(clients[idx].sock, "No pending challenge with this user." CRLF);
        return;
    }

    /* Notifier le challenger */
    char msg[BUF_SIZE];
    snprintf(msg, sizeof msg, "%s refused your challenge." CRLF, clients[idx].name);
    write_client(clients[j].sock, msg);
    write_client(clients[idx].sock, "Challenge refused." CRLF);

    /* Réinitialiser les états */
    clients[idx].pending_with = -1;
    clients[j].pending_with = -1;
    clients[idx].state = STATE_FREE;
    clients[j].state = STATE_FREE;
}

static void handle_move(Client *clients, int idx, const char *arg)
{
    Client *c = &clients[idx];

    if (c->state != STATE_PLAYING || c->game_id < 0)
    {
        write_client(c->sock, "You are not in a game.\n");
        return;
    }

    Game *g = &games[c->game_id];
    Awale *jeu = &g->awale;

    // déterminer si idx est South ou North
    JoueurID player_role;
    if (g->player_south == idx)
        player_role = PLAYER_SOUTH;
    else if (g->player_north == idx)
        player_role = PLAYER_NORTH;
    else
    {
        write_client(c->sock, "You are not a player in this game.\n");
        return;
    }

    // vérifier que c'est son tour
    if (jeu->tour_de != player_role)
    {
        write_client(c->sock, "Not your turn.\n");
        return;
    }

    // lire le numéro de case (1 à 6 attendu)
    int n = atoi(arg);
    if (n < 1 || n > 6)
    {
        write_client(c->sock,
                     "Invalid move. Use: /move <1-6>\n"
                     "South plays on pits 1 to 6 (left to right).\n"
                     "North plays on pits 1 to 6 (left to right).\n");
        return;
    }

    // convertir 1–6 vers l’indice réel 0–11
    int pit;
    if (player_role == PLAYER_SOUTH)
        pit = n - 1; // 1 → 0, 6 → 5
    else
        pit = 12 - n; // 1 → 11, 6 → 6

    // vérifier la validité du coup
    if (!coup_valide(jeu, pit))
    {
        write_client(c->sock, "Invalid move for current board.\n");
        return;
    }

    // exécuter le coup
    jouer_coup(jeu, pit);
    verifier_statut(jeu);

    // informer les joueurs
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s played pit %d.\n", c->name, n);
    write_client(clients[g->player_south].sock, msg);
    write_client(clients[g->player_north].sock, msg);

    // afficher le plateau mis à jour
    awale_format_board(jeu, msg, sizeof(msg));
    write_client(clients[g->player_south].sock, msg);
    write_client(clients[g->player_north].sock, msg);

    // vérifier fin de partie
    if (jeu->statut != EN_COURS)
    {
        char endmsg[BUF_SIZE];
        if (jeu->statut == SUD_GAGNE)
            snprintf(endmsg, sizeof(endmsg), "Game over! South wins.\n");
        else if (jeu->statut == NORD_GAGNE)
            snprintf(endmsg, sizeof(endmsg), "Game over! North wins.\n");
        else
            snprintf(endmsg, sizeof(endmsg), "Game over! Draw.\n");

        write_client(clients[g->player_south].sock, endmsg);
        write_client(clients[g->player_north].sock, endmsg);

        // mise à jour des statistiques en fin de partie
        int result = 0;
        if (jeu->statut == SUD_GAGNE)
            result = 1;
        else if (jeu->statut == NORD_GAGNE)
            result = 2;
        update_stats(clients[g->player_south].name, clients[g->player_north].name, result);

        // remettre les états clients
        clients[g->player_south].state = STATE_FREE;
        clients[g->player_south].game_id = -1;
        clients[g->player_north].state = STATE_FREE;
        clients[g->player_north].game_id = -1;

        g->used = 0; // libère la partie
    }
    else
    {
        // indiquer de qui est le tour
        const char *who = (jeu->tour_de == PLAYER_SOUTH)
                              ? clients[g->player_south].name
                              : clients[g->player_north].name;
        snprintf(msg, sizeof(msg), "Turn: %s\n", who);
        write_client(clients[g->player_south].sock, msg);
        write_client(clients[g->player_north].sock, msg);
    }
}

static void clear_clients(Client *clients, int actual)
{
    int i = 0;
    for (i = 0; i < actual; i++)
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
    for (i = 0; i < actual; i++)
    {
        /* we don't send message to the sender */
        if (sender.sock != clients[i].sock)
        {
            if (from_server == 0)
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
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
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

    if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
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
    if (send(sock, buffer, strlen(buffer), 0) < 0)
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
