#include <stdio.h>
#include "awale.h"

int main(void) {
    Awale jeu;
    initialiser_jeu(&jeu);
    afficher_jeu(&jeu);

    while (jeu.statut == EN_COURS) {
        int pit;
        printf("\nJoueur %s, choisis une case (0-11) : ",
               (jeu.tour_de == PLAYER_SOUTH) ? "Sud" : "Nord");
        scanf("%d", &pit);

        if (!coup_valide(&jeu, pit)) {
            printf("Coup invalide, réessaie.\n");
            continue;
        }

        jouer_coup(&jeu, pit);
        verifier_statut(&jeu);
        afficher_jeu(&jeu);
    }

    printf("\n=== FIN DE LA PARTIE ===\n");
    if (jeu.statut == SUD_GAGNE) printf("Victoire du Sud !\n");
    else if (jeu.statut == NORD_GAGNE) printf("Victoire du Nord !\n");
    else printf("Match nul.\n");

    return 0;
}