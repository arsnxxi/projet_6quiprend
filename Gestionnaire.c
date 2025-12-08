#include "commun.h"
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>

// ------------------------------------------------------
// OUTILS POUR LES CARTES
// ------------------------------------------------------

int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0) return 2;
    return 1;
}

void genererPaquet(Carte *paquet) {
    for (int i = 0; i < NB_CARTES_TOTAL; i++) {
        paquet[i].numero = i + 1;
        paquet[i].teteBoeufs = calculerTetes(i + 1);
    }
}

void melanger(Carte *paquet) {
    srand(time(NULL));
    for (int i = NB_CARTES_TOTAL - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carte tmp = paquet[i];
        paquet[i] = paquet[j];
        paquet[j] = tmp;
    }
}

// ------------------------------------------------------
// RANGÉES
// ------------------------------------------------------

void initialiserRangees(Rangee rangees[], Carte *paquet, int *indice) {
    for (int i = 0; i < NB_RANGEES; i++) {
        rangees[i].taille = 1;
        rangees[i].cartes[0] = paquet[(*indice)++];
    }
}

void afficherRangeesServeur(Rangee r[]) {
    printf("\n===== RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("R%d: ", i + 1);
        for (int j = 0; j < r[i].taille; j++) {
            printf("[%d (%d)] ",
                   r[i].cartes[j].numero,
                   r[i].cartes[j].teteBoeufs);
        }
        printf("\n");
    }
}

// ------------------------------------------------------
// GESTION D'UN TOUR
// ------------------------------------------------------

int trouverRangeeCompatible(Carte c, Rangee r[]) {
    int meilleur = -1;
    int ecartMin = 99999;

    for (int i = 0; i < NB_RANGEES; i++) {
        Carte dernier = r[i].cartes[r[i].taille - 1];
        if (c.numero > dernier.numero) {
            int ecart = c.numero - dernier.numero;
            if (ecart < ecartMin) {
                ecartMin = ecart;
                meilleur = i;
            }
        }
    }
    return meilleur;
}

int sommeTetes(Rangee r) {
    int s = 0;
    for (int i = 0; i < r.taille; i++)
        s += r.cartes[i].teteBoeufs;
    return s;
}

// ------------------------------------------------------
// COMMUNICATION PROCESSUS
// ------------------------------------------------------

void envoyerMessage(int fd, Message *msg) {
    write(fd, msg, sizeof(Message));
}

void lireMessage(int fd, Message *msg) {
    read(fd, msg, sizeof(Message));
}

// ------------------------------------------------------
// MAIN GESTIONNAIRE
// ------------------------------------------------------

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage : %s <nb_joueurs>\n", argv[0]);
        return 1;
    }

    int nbJoueurs = atoi(argv[1]);
    if (nbJoueurs < 2 || nbJoueurs > 10) {
        printf("Nombre de joueurs invalide (2-10)\n");
        return 1;
    }

    int pipesIn[nbJoueurs][2];   // gestionnaire lit ici
    int pipesOut[nbJoueurs][2];  // gestionnaire écrit ici
    pid_t pids[nbJoueurs];

    // --------------------------------------------
    // CREATION DES PIPES + FORK DES JOUEURS
    // --------------------------------------------

    for (int i = 0; i < nbJoueurs; i++) {
        pipe(pipesIn[i]);
        pipe(pipesOut[i]);

        pids[i] = fork();

        if (pids[i] == 0) {
            // Processus joueur
            close(pipesIn[i][0]);    // fermé côté lecture gestionnaire
            close(pipesOut[i][1]);   // fermé côté écriture gestionnaire

            char fd_in_str[10], fd_out_str[10];
            sprintf(fd_in_str, "%d", pipesOut[i][0]);
            sprintf(fd_out_str, "%d", pipesIn[i][1]);

            if (i == 0)
                execl("./joueurH", "joueurH", fd_in_str, fd_out_str, NULL);
            else
                execl("./joueurR", "joueurR", fd_in_str, fd_out_str, NULL);

            exit(1);
        }

        // Processus gestionnaire
        close(pipesIn[i][1]);
        close(pipesOut[i][0]);
    }

    // --------------------------------------------
    // GENERATION DU PAQUET ET DISTRIBUTION
    // --------------------------------------------

    Carte paquet[NB_CARTES_TOTAL];
    genererPaquet(paquet);
    melanger(paquet);

    int indiceCarte = 0;

    Rangee rangees[NB_RANGEES];
    initialiserRangees(rangees, paquet, &indiceCarte);

    // Distribuer les mains
    for (int j = 0; j < nbJoueurs; j++) {
        Message msg;
        msg.type = MSG_INIT;
        msg.joueurID = j;

        for (int c = 0; c < NB_CARTES_MAIN; c++)
            msg.mainJoueur[c] = paquet[indiceCarte++];

        envoyerMessage(pipesOut[j][1], &msg);
    }

    // --------------------------------------------
    // JEU SUR 10 TOURS
    // --------------------------------------------

    for (int tour = 0; tour < NB_CARTES_MAIN; tour++) {

        printf("\n===== TOUR %d =====\n", tour + 1);
        afficherRangeesServeur(rangees);

        // Envoyer l'état des rangées à tous
        for (int j = 0; j < nbJoueurs; j++) {
            Message msg;
            msg.type = MSG_RANGEES;
            memcpy(msg.rangees, rangees, sizeof(rangees));
            envoyerMessage(pipesOut[j][1], &msg);
        }

        // Demander une carte
        for (int j = 0; j < nbJoueurs; j++) {
            Message msg;
            msg.type = MSG_TOUR;
            memcpy(msg.rangees, rangees, sizeof(rangees));
            envoyerMessage(pipesOut[j][1], &msg);
        }

        // Réception des cartes
        Message reponses[nbJoueurs];
        for (int j = 0; j < nbJoueurs; j++)
            lireMessage(pipesIn[j][0], &reponses[j]);

        // Trier les cartes par ordre croissant
        for (int i = 0; i < nbJoueurs - 1; i++) {
            for (int j = i + 1; j < nbJoueurs; j++) {
                if (reponses[j].carte.numero < reponses[i].carte.numero) {
                    Message tmp = reponses[i];
                    reponses[i] = reponses[j];
                    reponses[j] = tmp;
                }
            }
        }

        // Placement des cartes
        for (int j = 0; j < nbJoueurs; j++) {
            Carte cj = reponses[j].carte;
            int idx = trouverRangeeCompatible(cj, rangees);

            if (idx == -1) {
                int piresTetes = 9999;
                int pireIdx = 0;
                for (int r = 0; r < NB_RANGEES; r++) {
                    int s = sommeTetes(rangees[r]);
                    if (s < piresTetes) {
                        piresTetes = s;
                        pireIdx = r;
                    }
                }
                printf("J%d prend la rangée %d !\n", reponses[j].joueurID, pireIdx+1);
                rangees[pireIdx].taille = 1;
                rangees[pireIdx].cartes[0] = cj;
            } else {
                Rangee *r = &rangees[idx];
                if (r->taille == 5) {
                    printf("J%d remplit la rangée %d !\n", reponses[j].joueurID, idx+1);
                    r->taille = 1;
                    r->cartes[0] = cj;
                } else {
                    r->cartes[r->taille++] = cj;
                }
            }
        }
    }

    // Fin de partie
    for (int j = 0; j < nbJoueurs; j++) {
        Message msg;
        msg.type = MSG_FIN;
        envoyerMessage(pipesOut[j][1], &msg);
    }

    for (int j = 0; j < nbJoueurs; j++)
        waitpid(pids[j], NULL, 0);

    printf("\n===== PARTIE TERMINÉE =====\n");

    return 0;
}
