#include "commun.h"
#include <string.h>

// ---------------------------------------------------------
// AFFICHAGES POUR LE JOUEUR HUMAIN
// ---------------------------------------------------------

void afficherRangees(Rangee rangees[]) {
    printf("\n===== ETAT DES RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("Rangée %d : ", i+1);
        for (int j = 0; j < rangees[i].taille; j++) {
            printf("[%d (%d)] ",
                rangees[i].cartes[j].numero,
                rangees[i].cartes[j].tete
            );
        }
        printf("\n");
    }
    printf("============================\n");
}

void afficherMain(Carte main[], int taille) {
    printf("\n===== VOTRE MAIN =====\n");
    for (int i = 0; i < taille; i++) {
        printf("%d : Carte %d (%d têtes)\n",
            i,
            main[i].numero,
            main[i].tete
        );
    }
    printf("======================\n");
}


// ---------------------------------------------------------
// PROCESSUS PRINCIPAL DU JOUEUR HUMAIN
// ---------------------------------------------------------

int main(int argc, char *argv[]) {

    int fd_in = atoi(argv[1]);   // gestionnaire -> joueur
    int fd_out = atoi(argv[2]);  // joueur -> gestionnaire

    Message msg;
    Carte mainJoueur[NB_CARTES_MAIN];
    int nbCartes = NB_CARTES_MAIN;
    int joueurID = -1;

    while (1) {

        if (read(fd_in, &msg, sizeof(Message)) <= 0)
            break;

        switch (msg.type) {

            // ---------------------------------------------
            // Réception de la main initiale
            // ---------------------------------------------
            case MSG_INIT:
                joueurID = msg.joueurID;
                memcpy(mainJoueur, msg.mainJoueur, sizeof(mainJoueur));

                printf("\n>>> Vous êtes le joueur HUMAIN #%d\n", joueurID);
                break;


            // ---------------------------------------------
            // Mise à jour des rangées (affichage)
            // ---------------------------------------------
            case MSG_RANGEES:
                afficherRangees(msg.rangees);
                break;


            // ---------------------------------------------
            // Le gestionnaire demande de jouer une carte
            // ---------------------------------------------
            case MSG_TOUR: {

                afficherMain(mainJoueur, nbCartes);

                int choix = -1;
                printf("Choisissez une carte à jouer (index) : ");
                fflush(stdout);

                scanf("%d", &choix);

                while (choix < 0 || choix >= nbCartes) {
                    printf("Index invalide. Réessayez : ");
                    fflush(stdout);
                    scanf("%d", &choix);
                }

                Carte c = mainJoueur[choix];

                // retirer la carte de la main
                for (int i = choix; i < nbCartes - 1; i++)
                    mainJoueur[i] = mainJoueur[i + 1];
                nbCartes--;

                // envoyer la carte au gestionnaire
                Message rep;
                rep.type = MSG_CARTE;
                rep.joueurID = joueurID;
                rep.carte = c;

                write(fd_out, &rep, sizeof(Message));
                break;
            }


            // ---------------------------------------------
            // Fin du jeu
            // ---------------------------------------------
            case MSG_FIN:
                printf("\n>>> La partie est terminée. Merci d'avoir joué !\n");
                close(fd_in);
                close(fd_out);
                return 0;
        }
    }

    return 0;
}
