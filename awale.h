#ifndef AWALE_H
#define AWALE_H

typedef enum
{
    PLAYER_SOUTH = 0,
    PLAYER_NORTH = 1
} JoueurID;

typedef enum
{
    EN_COURS = 0,
    SUD_GAGNE,
    NORD_GAGNE,
    NUL
} StatutJeu;

typedef struct
{
    unsigned char plateau[12];
    unsigned char captures[2];
    JoueurID tour_de;
    unsigned short nombre_tours;
    StatutJeu statut;
} Awale;

typedef struct
{
    int used;    // 0 = libre, 1 = partie en cours
    Awale awale; // votre structure de jeu

    int player_south; // index dans le tableau clients[]
    int player_north; // index dans le tableau clients[]
} Game;

void initialiser_jeu(Awale *jeu);
void afficher_jeu(const Awale *jeu);
int coup_valide(const Awale *jeu, int pit);
void jouer_coup(Awale *jeu, int pit);
void verifier_statut(Awale *jeu);
void awale_format_board(const Awale *jeu, char *out, size_t out_size);
void configurer_plateau_demo(Awale *jeu);

#endif
