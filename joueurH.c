#include "commun.h"

// ---------------------------------------------------------
// AFFICHAGE DU JOUEUR HUMAIN
// ---------------------------------------------------------

void afficherRangees(Rangee rangees[]) {
    printf("\n===== ETAT DES RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("Rangée %d : ", i+1);
        for (int j = 0; j < rangees[i].taille; j++) {
            printf("[%d (%d)] ", 
                rangees[i].cartes[j].numero,
                rangees[i].cartes[j].teteBoeufs);
        }
        printf("\n");
    }
    printf("============================\n");
}

void afficherMain(Carte main[], int taille) {
    printf("\n===== VOTRE MAIN =====\n");
    for (int i = 0; i < taille; i++) {
        printf("%d : Carte %d (%d têtes)\n", 
            i, main[i].numero, main[i].teteBoeufs);
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
    int cartesRestantes = NB_CARTES_MAIN;
    int joueurID = -1;

    while (1) {

        // Lire le message envoyé par le gestionnaire
        if (read(fd_in, &msg, sizeof(Message)) <= 0)
            break;

        switch (msg.type) {

            case MSG_INIT:
                joueurID = msg.joueurID;
                for (int i = 0; i < NB_CARTES_MAIN; i++)
                    mainJoueur[i] = msg.mainJoueur[i];
                printf(">>> Vous êtes le joueur humain #%d\n", joueurID);
                break;

            case MSG_RANGEES:
                afficherRangees(msg.rangees);
                break;

            case MSG_TOUR: {
                afficherMain(mainJoueur, cartesRestantes);

                int choix = -1;
                printf("Choisissez une carte à jouer (index) : ");
                scanf("%d", &choix);

                while (choix < 0 || choix >= cartesRestantes) {
                    printf("Index invalide, recommencez : ");
                    scanf("%d", &choix);
                }

                Carte selected = mainJoueur[choix];

                // Retirer la carte choisie de la main
                for (int j = choix; j < cartesRestantes - 1; j++)
                    mainJoueur[j] = mainJoueur[j + 1];
                cartesRestantes--;

                // Envoyer la carte au gestionnaire
                Message rep;
                rep.type = MSG_CARTE;
                rep.carte = selected;
                rep.joueurID = joueurID;

                write(fd_out, &rep, sizeof(Message));

                break;
            }

            case MSG_FIN:
                printf("\n>>> PARTIE TERMINÉE !\n");
                close(fd_in);
                close(fd_out);
                return 0;
        }
    }

    return 0;
}
