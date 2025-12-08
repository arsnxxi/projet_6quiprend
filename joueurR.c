#include "commun.h"

// ---------------------------------------------------------
// FONCTIONS UTILISANT L'IA SIMPLE (issues de ton ancien code)
// ---------------------------------------------------------

// Retourne 1 si la carte peut être placée dans la rangée
int peutPlacerCarteDansRangee(Carte carte, Rangee r) {
    if (r.taille == 0) return 1;
    return carte.numero > r.cartes[r.taille - 1].numero;
}

// Trouve la rangée compatible avec le plus petit écart
int trouverIndiceRangeeCompatible(Carte carte, Rangee rangees[]) {
    int meilleurIndice = -1;
    int meilleurEcart = 9999;

    for (int i = 0; i < NB_RANGEES; i++) {
        if (peutPlacerCarteDansRangee(carte, rangees[i])) {
            int dernier = rangees[i].cartes[rangees[i].taille - 1].numero;
            int ecart = carte.numero - dernier;

            if (ecart < meilleurEcart) {
                meilleurEcart = ecart;
                meilleurIndice = i;
            }
        }
    }
    return meilleurIndice;
}

// Choisir une carte simple : la plus petite jouable, ou la plus petite tout court
Carte choisirCarteIA(Carte main[], int tailleMain, Rangee rangees[]) {
    int meilleurIndex = -1;
    int meilleureValeur = 9999;

    for (int i = 0; i < tailleMain; i++) {
        Carte c = main[i];

        // Si jouable, on préfère les petites cartes jouables
        int idx = trouverIndiceRangeeCompatible(c, rangees);
        if (idx != -1) {
            if (c.numero < meilleureValeur) {
                meilleureValeur = c.numero;
                meilleurIndex = i;
            }
        }
    }

    // Si aucune carte n'est jouable, on joue la plus petite carte tout court
    if (meilleurIndex == -1) {
        for (int i = 0; i < tailleMain; i++) {
            if (main[i].numero < meilleureValeur) {
                meilleureValeur = main[i].numero;
                meilleurIndex = i;
            }
        }
    }

    return main[meilleurIndex];
}


// ---------------------------------------------------------
// PROCESSUS PRINCIPAL DU JOUEUR ROBOT
// ---------------------------------------------------------

int main(int argc, char *argv[]) {

    int fd_in = atoi(argv[1]);   // pipe lecture : gestionnaire -> joueur
    int fd_out = atoi(argv[2]);  // pipe écriture : joueur -> gestionnaire

    Message msg;
    Carte mainJoueur[NB_CARTES_MAIN];

    int cartesRestantes = NB_CARTES_MAIN;
    int joueurID = -1;

    // Boucle principale du joueur
    while (1) {

        // Lire un message du gestionnaire
        if (read(fd_in, &msg, sizeof(Message)) <= 0)
            break;

        switch (msg.type) {

            case MSG_INIT:
                joueurID = msg.joueurID;
                for (int i = 0; i < NB_CARTES_MAIN; i++)
                    mainJoueur[i] = msg.mainJoueur[i];
                break;

            case MSG_RANGEES:
                // Le gestionnaire envoie les rangées : juste les stocker dans msg
                break;

            case MSG_TOUR: {
                // Choisir une carte
                Carte c = choisirCarteIA(mainJoueur, cartesRestantes, msg.rangees);

                // Retirer la carte de la main
                int index = -1;
                for (int i = 0; i < cartesRestantes; i++) {
                    if (mainJoueur[i].numero == c.numero) {
                        index = i;
                        break;
                    }
                }
                for (int j = index; j < cartesRestantes - 1; j++)
                    mainJoueur[j] = mainJoueur[j + 1];

                cartesRestantes--;

                // Envoyer la carte au gestionnaire
                Message rep;
                rep.type = MSG_CARTE;
                rep.carte = c;
                rep.joueurID = joueurID;

                write(fd_out, &rep, sizeof(Message));
                break;
            }

            case MSG_FIN:
                // Le gestionnaire termine le joueur
                close(fd_in);
                close(fd_out);
                return 0;
        }
    }

    return 0;
}
