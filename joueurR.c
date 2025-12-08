#include "commun.h"
#include <string.h>

// ---------------------------------------------------------
// IA SIMPLE : version propre et conforme au 6 qui prend
// ---------------------------------------------------------

int peutPlacer(Carte c, Rangee *r) {
    if (r->taille == 0) return 1;
    return c.numero > r->cartes[r->taille - 1].numero;
}

int trouverRangeeCompatible(Carte c, Rangee rangees[]) {
    int idx = -1;
    int ecartMin = 99999;

    for (int i = 0; i < NB_RANGEES; i++) {
        if (rangees[i].taille == 0) continue;

        int dernier = rangees[i].cartes[rangees[i].taille - 1].numero;

        if (c.numero > dernier) {
            int ecart = c.numero - dernier;
            if (ecart < ecartMin) {
                ecartMin = ecart;
                idx = i;
            }
        }
    }
    return idx;
}

Carte choisirCarteIA(Carte main[], int nbCartes, Rangee rangees[]) {
    int meilleur = -1;
    int valeurMin = 99999;

    // 1) essayer de jouer la plus petite carte compatible
    for (int i = 0; i < nbCartes; i++) {
        int idx = trouverRangeeCompatible(main[i], rangees);
        if (idx != -1) {
            if (main[i].numero < valeurMin) {
                valeurMin = main[i].numero;
                meilleur = i;
            }
        }
    }

    // 2) si aucune carte n'est compatible, jouer la plus petite
    if (meilleur == -1) {
        for (int i = 0; i < nbCartes; i++) {
            if (main[i].numero < valeurMin) {
                valeurMin = main[i].numero;
                meilleur = i;
            }
        }
    }

    return main[meilleur];
}

// ---------------------------------------------------------
// PROCESSUS PRINCIPAL DU JOUEUR ROBOT
// ---------------------------------------------------------

int main(int argc, char *argv[]) {

    int fd_in = atoi(argv[1]);   // pipe lecture (gestionnaire -> joueur)
    int fd_out = atoi(argv[2]);  // pipe écriture (joueur -> gestionnaire)

    Message msg;
    Carte mainJoueur[NB_CARTES_MAIN];
    int nbCartes = NB_CARTES_MAIN;
    int joueurID = -1;

    while (1) {

        if (read(fd_in, &msg, sizeof(Message)) <= 0)
            break;

        switch (msg.type) {

            case MSG_INIT:
                joueurID = msg.joueurID;
                memcpy(mainJoueur, msg.mainJoueur, sizeof(mainJoueur));
                break;

            case MSG_RANGEES:
                // juste mis à jour dans msg, rien à faire
                break;

            case MSG_TOUR: {

                // choisir une carte via IA
                Carte c = choisirCarteIA(mainJoueur, nbCartes, msg.rangees);

                // la retirer de la main
                int index = -1;
                for (int i = 0; i < nbCartes; i++) {
                    if (mainJoueur[i].numero == c.numero) {
                        index = i;
                        break;
                    }
                }
                for (int i = index; i < nbCartes - 1; i++)
                    mainJoueur[i] = mainJoueur[i + 1];
                nbCartes--;

                // envoyer la réponse
                Message rep;
                rep.type = MSG_CARTE;
                rep.joueurID = joueurID;
                rep.carte = c;

                write(fd_out, &rep, sizeof(Message));
                break;
            }

            case MSG_FIN:
                close(fd_in);
                close(fd_out);
                return 0;
        }
    }

    return 0;
}
