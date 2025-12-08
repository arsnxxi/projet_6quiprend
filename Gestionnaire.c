#include "commun.h"
#include <string.h>
#include <time.h>
#include <sys/wait.h>

// ---------------------------------------------------------
// CALCUL DES TÊTES DE BOEUFS
// ---------------------------------------------------------

int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0) return 2;
    return 1;
}

// ---------------------------------------------------------
// GÉNÉRATION & MÉLANGE DU PAQUET
// ---------------------------------------------------------

void genererPaquet(Carte *paquet) {
    for (int i = 0; i < NB_CARTES_TOTAL; i++) {
        paquet[i].numero = i + 1;
        paquet[i].tete = calculerTetes(i + 1);
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

// ---------------------------------------------------------
// INITIALISATION DES RANGÉES
// ---------------------------------------------------------

void initialiserRangees(Rangee r[], Carte *paquet, int *idx) {
    for (int i = 0; i < NB_RANGEES; i++) {
        r[i].taille = 1;
        r[i].cartes[0] = paquet[(*idx)++];
    }
}

void afficherRangeesServeur(Rangee r[]) {
    printf("\n===== RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("R%d : ", i + 1);
        for (int j = 0; j < r[i].taille; j++) {
            printf("[%d (%d)] ",
                r[i].cartes[j].numero,
                r[i].cartes[j].tete
            );
        }
        printf("\n");
    }
}

// ---------------------------------------------------------
// OUTILS DE CALCUL
// ---------------------------------------------------------

int sommeTetes(Rangee *r) {
    int tot = 0;
    for (int i = 0; i < r->taille; i++)
        tot += r->cartes[i].tete;
    return tot;
}

// Trouve la rangée compatible au plus petit écart
int trouverRangeeCompatible(Carte c, Rangee r[]) {
    int idx = -1;
    int ecartMin = 99999;

    for (int i = 0; i < NB_RANGEES; i++) {
        int last = r[i].cartes[r[i].taille - 1].numero;
        if (c.numero > last) {
            int ecart = c.numero - last;
            if (ecart < ecartMin) {
                ecartMin = ecart;
                idx = i;
            }
        }
    }
    return idx;
}

// ---------------------------------------------------------
// COMMUNICATION (READ/WRITE SIMPLIFIÉS)
// ---------------------------------------------------------

void envoyerMessage(int fd, Message *m) {
    write(fd, m, sizeof(Message));
}

void lireMessage(int fd, Message *m) {
    read(fd, m, sizeof(Message));
}

// ---------------------------------------------------------
// GESTIONNAIRE (PROCESSUS PRINCIPAL)
// ---------------------------------------------------------

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage : %s <nb_joueurs>\n", argv[0]);
        return 1;
    }

    int nbJoueurs = atoi(argv[1]);
    if (nbJoueurs < 2 || nbJoueurs > MAX_JOUEURS) {
        printf("Nombre de joueurs invalide.\n");
        return 1;
    }

    printf(">>> Démarrage d'une partie avec %d joueurs...\n", nbJoueurs);

    int pipesIn[nbJoueurs][2];   // joueur -> gestionnaire
    int pipesOut[nbJoueurs][2];  // gestionnaire -> joueur
    pid_t pids[nbJoueurs];

    // -----------------------------------------------------
    // CRÉATION DES PIPES ET LANCEMENT DES PROCESSUS JOUEURS
    // -----------------------------------------------------

    for (int j = 0; j < nbJoueurs; j++) {
        pipe(pipesIn[j]);
        pipe(pipesOut[j]);

        pids[j] = fork();

        if (pids[j] == 0) {

            close(pipesIn[j][0]);
            close(pipesOut[j][1]);

            char inStr[10], outStr[10];
            sprintf(inStr, "%d", pipesOut[j][0]);
            sprintf(outStr, "%d", pipesIn[j][1]);

            if (j == 0)
                execl("./joueurH", "joueurH", inStr, outStr, NULL);
            else
                execl("./joueurR", "joueurR", inStr, outStr, NULL);

            perror("execl");
            exit(1);
        }

        // gestionnaire conserve les bons côtés
        close(pipesIn[j][1]);
        close(pipesOut[j][0]);
    }

    // -----------------------------------------------------
    // GÉNÉRATION DU PAQUET ET DISTRIBUTION
    // -----------------------------------------------------

    Carte paquet[NB_CARTES_TOTAL];
    genererPaquet(paquet);
    melanger(paquet);

    Rangee rangees[NB_RANGEES];
    int idxCarte = 0;

    initialiserRangees(rangees, paquet, &idxCarte);

    int scores[MAX_JOUEURS] = {0};

    // Distribution des mains
    for (int j = 0; j < nbJoueurs; j++) {
        Message msg;
        msg.type = MSG_INIT;
        msg.joueurID = j;

        for (int c = 0; c < NB_CARTES_MAIN; c++)
            msg.mainJoueur[c] = paquet[idxCarte++];

        envoyerMessage(pipesOut[j][1], &msg);
    }

    // -----------------------------------------------------
    // LES 10 TOURS DE LA PARTIE
    // -----------------------------------------------------

    for (int tour = 0; tour < NB_CARTES_MAIN; tour++) {

        printf("\n===== TOUR %d =====\n", tour + 1);
        afficherRangeesServeur(rangees);

        // Envoyer rangées
        for (int j = 0; j < nbJoueurs; j++) {
            Message m;
            m.type = MSG_RANGEES;
            memcpy(m.rangees, rangees, sizeof(rangees));
            envoyerMessage(pipesOut[j][1], &m);
        }

        // Demander de jouer
        for (int j = 0; j < nbJoueurs; j++) {
            Message m;
            m.type = MSG_TOUR;
            memcpy(m.rangees, rangees, sizeof(rangees));
            envoyerMessage(pipesOut[j][1], &m);
        }

        // Recevoir les cartes
        Message rep[nbJoueurs];
        for (int j = 0; j < nbJoueurs; j++)
            lireMessage(pipesIn[j][0], &rep[j]);

        // Trier les cartes (croissant)
        for (int a = 0; a < nbJoueurs - 1; a++) {
            for (int b = a + 1; b < nbJoueurs; b++) {
                if (rep[b].carte.numero < rep[a].carte.numero) {
                    Message tmp = rep[a];
                    rep[a] = rep[b];
                    rep[b] = tmp;
                }
            }
        }

        // TRAITEMENT des cartes
        for (int j = 0; j < nbJoueurs; j++) {

            Carte c = rep[j].carte;
            int id = rep[j].joueurID;

            int idx = trouverRangeeCompatible(c, rangees);

            // CAS 1 : aucune rangée compatible -> le joueur ramasse la moins pénalisante
            if (idx == -1) {
                int best = 0, bestScore = 99999;
                for (int r = 0; r < NB_RANGEES; r++) {
                    int s = sommeTetes(&rangees[r]);
                    if (s < bestScore) {
                        bestScore = s;
                        best = r;
                    }
                }

                printf("J%d ne peut rien placer → ramasse la rangée %d (+%d points)\n",
                       id, best + 1, bestScore);

                scores[id] += bestScore;

                rangees[best].taille = 1;
                rangees[best].cartes[0] = c;
            }

            // CAS 2 : la rangée est compatible
            else {
                Rangee *r = &rangees[idx];

                // Si elle est pleine → ramasser
                if (r->taille == 5) {
                    int s = sommeTetes(r);
                    printf("J%d complète la rangée %d → ramasse (+%d points)\n",
                           id, idx + 1, s);

                    scores[id] += s;

                    r->taille = 1;
                    r->cartes[0] = c;
                }

                // Sinon on ajoute normalement
                else {
                    r->cartes[r->taille++] = c;
                }
            }
        }
    }

    // -----------------------------------------------------
    // FIN DE PARTIE
    // -----------------------------------------------------

    printf("\n===== FIN DE PARTIE =====\n");
    for (int j = 0; j < nbJoueurs; j++)
        printf("Score J%d = %d\n", j, scores[j]);

    // dire aux joueurs de se terminer
    for (int j = 0; j < nbJoueurs; j++) {
        Message m;
        m.type = MSG_FIN;
        envoyerMessage(pipesOut[j][1], &m);
    }

    // attendre la terminaison des joueurs
    for (int j = 0; j < nbJoueurs; j++)
        waitpid(pids[j], NULL, 0);

    printf(">>> Tous les joueurs ont terminé.\n\n");

    return 0;
}
