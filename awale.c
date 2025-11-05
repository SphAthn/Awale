#include <stdio.h>
#include <string.h>
#include "awale.h"

void initialiser_jeu(Awale *jeu) {
    for (int i = 0; i < 12; i++) jeu->plateau[i] = 4;
    jeu->captures[0] = 0;
    jeu->captures[1] = 0;
    jeu->tour_de = PLAYER_SOUTH;
    jeu->nombre_tours = 0;
    jeu->statut = EN_COURS;
}

void afficher_jeu(const Awale *jeu) {
    printf("\nNord: ");
    for (int i = 11; i >= 6; i--) printf("%2d ", jeu->plateau[i]);
    printf("\nSud : ");
    for (int i = 0; i <= 5; i++) printf("%2d ", jeu->plateau[i]);
    printf("\nCaptures: Sud=%d | Nord=%d\n",
           jeu->captures[PLAYER_SOUTH], jeu->captures[PLAYER_NORTH]);
}

int coup_valide(const Awale *jeu, int coup) {
    if (coup < 0 || coup >= 12) return 0;
    if (jeu->plateau[coup] == 0) return 0;
    if (jeu->tour_de == PLAYER_SOUTH && coup >= 6) return 0;
    if (jeu->tour_de == PLAYER_NORTH && coup < 6) return 0;

    // Obligation de nourrir l'adversaire
    if (jeu->tour_de == PLAYER_SOUTH) {
        int famine = 1;
        for (int i = 6; i < 12; i++) {
            if (jeu->plateau[i] != 0) {
                famine = 0;
            }
        }
        if (famine) {
            if (coup + jeu->plateau[coup] < 6) {
                return 0;
            }
        }
    } else {
        int famine = 1;
        for (int i = 0; i < 6; i++) {
            if (jeu->plateau[i] != 0) {
                famine = 0;
            }
        }
        if (famine) {
            if (coup + jeu->plateau[coup] < 12) {
                return 0;
            }
        }
    }

    return 1;
}

void jouer_coup(Awale *jeu, int coup) {
    int graines = jeu->plateau[coup];
    jeu->plateau[coup] = 0;
    int pos = coup;
    while (graines > 0) {
        pos = (pos + 1) % 12;
        if (pos != coup) { // ne resème pas dans la case d'origine
            jeu->plateau[pos]++;
            graines--;
        }
    }

    // capture simple : si la dernière case est dans le camp adverse et contient 2 ou 3
    int camp_depart = (coup < 6) ? PLAYER_SOUTH : PLAYER_NORTH;
    int camp_adv = 1 - camp_depart;
    while (((camp_adv == PLAYER_NORTH && pos >= 6) ||
            (camp_adv == PLAYER_SOUTH && pos < 6)) &&
           (jeu->plateau[pos] == 2 || jeu->plateau[pos] == 3)) {
        jeu->captures[camp_depart] += jeu->plateau[pos];
        jeu->plateau[pos] = 0;
        pos = (pos - 1 + 12) % 12;
    }

    jeu->nombre_tours++;
    jeu->tour_de = 1 - jeu->tour_de;
}

void verifier_statut(Awale *jeu) {
    if (jeu->captures[PLAYER_SOUTH] >= 25)
        jeu->statut = SUD_GAGNE;
    else if (jeu->captures[PLAYER_NORTH] >= 25)
        jeu->statut = NORD_GAGNE;
    else if (jeu->captures[PLAYER_SOUTH] == 24 &&
             jeu->captures[PLAYER_NORTH] == 24)
        jeu->statut = NUL;
}

void awale_format_board(const Awale *jeu, char *out, size_t out_size) {
    char tmp[256];
    out[0] = '\0';

    strcat(out, "Nord: ");
    for (int i = 11; i >= 6; i--) {
        snprintf(tmp, sizeof(tmp), "%2d ", jeu->plateau[i]);
        strncat(out, tmp, out_size - strlen(out) - 1);
    }
    strncat(out, "\nSud : ", out_size - strlen(out) - 1);
    for (int i = 0; i <= 5; i++) {
        snprintf(tmp, sizeof(tmp), "%2d ", jeu->plateau[i]);
        strncat(out, tmp, out_size - strlen(out) - 1);
    }
    snprintf(tmp, sizeof(tmp), "\nCaptures: Sud=%d | Nord=%d\n",
             jeu->captures[PLAYER_SOUTH], jeu->captures[PLAYER_NORTH]);
    strncat(out, tmp, out_size - strlen(out) - 1);
}
