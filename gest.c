#include "commun.h"

#include <string.h>
#include <time.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SCORE_CIBLE 66

static void error(const char *msg) {
    perror(msg);
    exit(1);
}

/* read/write "complets" (toujours avec read/write, pas send/recv) */
static int read_full(int fd, void *buf, size_t n) {
    size_t got = 0;
    char *p = (char*)buf;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r == 0) return 0;     // déconnexion
        if (r < 0) return -1;     // erreur
        got += (size_t)r;
    }
    return 1;
}

static int write_full(int fd, const void *buf, size_t n) {
    size_t sent = 0;
    const char *p = (const char*)buf;
    while (sent < n) {
        ssize_t w = write(fd, p + sent, n - sent);
        if (w <= 0) return -1;
        sent += (size_t)w;
    }
    return 1;
}

static void envoyerMessage(int fd, Message *m) {
    if (write_full(fd, m, sizeof(Message)) <= 0) error("write_full");
}

static int lireMessage(int fd, Message *m) {
    return read_full(fd, m, sizeof(Message));
}

// ---------------------------------------------------------
// REGLES / CARTES
// ---------------------------------------------------------
static int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0)  return 2;
    return 1;
}

static void genererPaquet(Carte *paquet) {
    for (int i = 0; i < NB_CARTES_TOTAL; i++) {
        paquet[i].numero = i + 1;
        paquet[i].tete   = calculerTetes(i + 1);
    }
}

static void melanger(Carte *paquet) {
    for (int i = NB_CARTES_TOTAL - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carte tmp = paquet[i];
        paquet[i] = paquet[j];
        paquet[j] = tmp;
    }
}

static void initialiserRangees(Rangee r[], Carte *paquet, int *idx) {
    for (int i = 0; i < NB_RANGEES; i++) {
        r[i].taille = 1;
        r[i].cartes[0] = paquet[(*idx)++];
    }
}

static void afficherRangeesServeur(Rangee r[]) {
    printf("\n===== RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("R%d : ", i + 1);
        for (int j = 0; j < r[i].taille; j++) {
            printf("[%d (%d)] ", r[i].cartes[j].numero, r[i].cartes[j].tete);
        }
        printf("\n");
    }
}

static int sommeTetes(Rangee *r) {
    int tot = 0;
    for (int i = 0; i < r->taille; i++) tot += r->cartes[i].tete;
    return tot;
}

static int trouverRangeeCompatible(Carte c, Rangee r[]) {
    int idx = -1;
    int ecartMin = INT_MAX;

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

static int quelquunAtteint(int scores[], int nbJoueurs) {
    for (int i = 0; i < nbJoueurs; i++)
        if (scores[i] >= SCORE_CIBLE) return 1;
    return 0;
}

static void envoyerScores(int sockets[], int nbJoueurs, int scores[]) {
    Message m;
    memset(&m, 0, sizeof(m));
    m.type = MSG_SCORE;
    for (int i = 0; i < MAX_JOUEURS; i++) m.scores[i] = -1;
    for (int i = 0; i < nbJoueurs; i++) m.scores[i] = scores[i];

    for (int j = 0; j < nbJoueurs; j++) {
        envoyerMessage(sockets[j], &m);
    }
}

// ---------------------------------------------------------
// SOCKET SERVEUR (style TP)
// ---------------------------------------------------------
static int creerServeur(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("socket");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t)port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("bind");

    listen(sockfd, 10);
    return sockfd;
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    srand((unsigned)time(NULL));

    if (argc < 3) {
        printf("Usage : %s <nb_joueurs> <port>\n", argv[0]);
        return 1;
    }

    int nbJoueurs = atoi(argv[1]);
    int port = atoi(argv[2]);

    if (nbJoueurs < 2 || nbJoueurs > MAX_JOUEURS) {
        printf("Nombre de joueurs invalide.\n");
        return 1;
    }

    int servfd = creerServeur(port);
    printf(">>> Serveur prêt (port %d). Attente de %d joueurs...\n", port, nbJoueurs);

    int sockets[MAX_JOUEURS];
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    for (int j = 0; j < nbJoueurs; j++) {
        int newsock = accept(servfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsock < 0) error("accept");
        sockets[j] = newsock;
        printf(">>> Joueur %d connecté.\n", j);
    }

    close(servfd); // plus besoin d'accepter

    int scores[MAX_JOUEURS] = {0};
    int manche = 1;

    while (!quelquunAtteint(scores, nbJoueurs)) {

        printf("\n==============================\n");
        printf("========= MANCHE %d ==========\n", manche);
        printf("==============================\n");

        Carte paquet[NB_CARTES_TOTAL];
        genererPaquet(paquet);
        melanger(paquet);

        int idxCarte = 0;
        Rangee rangees[NB_RANGEES];
        initialiserRangees(rangees, paquet, &idxCarte);

        // Envoyer les mains (MSG_INIT) -> nouvelle manche
        for (int j = 0; j < nbJoueurs; j++) {
            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = MSG_INIT;
            msg.joueurID = j;
            for (int c = 0; c < NB_CARTES_MAIN; c++)
                msg.mainJoueur[c] = paquet[idxCarte++];

            envoyerMessage(sockets[j], &msg);
        }

        // scores au début de manche
        envoyerScores(sockets, nbJoueurs, scores);

        // 10 tours
        for (int tour = 0; tour < NB_CARTES_MAIN; tour++) {

            printf("\n----- Manche %d | Tour %d -----\n", manche, tour + 1);
            afficherRangeesServeur(rangees);

            // Envoyer rangées
            for (int j = 0; j < nbJoueurs; j++) {
                Message m;
                memset(&m, 0, sizeof(m));
                m.type = MSG_RANGEES;
                memcpy(m.rangees, rangees, sizeof(rangees));
                envoyerMessage(sockets[j], &m);
            }

            // Demander de jouer
            for (int j = 0; j < nbJoueurs; j++) {
                Message m;
                memset(&m, 0, sizeof(m));
                m.type = MSG_TOUR;
                memcpy(m.rangees, rangees, sizeof(rangees));
                envoyerMessage(sockets[j], &m);
            }

            // Lire les cartes (simple: 1 par 1)
            Message rep[MAX_JOUEURS];
            for (int j = 0; j < nbJoueurs; j++) {
                int st = lireMessage(sockets[j], &rep[j]);
                if (st <= 0) {
                    printf("!!! Joueur %d déconnecté.\n", j);
                    return 1;
                }
            }

            // Trier les cartes jouées par numéro croissant
            for (int a = 0; a < nbJoueurs - 1; a++) {
                for (int b = a + 1; b < nbJoueurs; b++) {
                    if (rep[b].carte.numero < rep[a].carte.numero) {
                        Message tmp = rep[a];
                        rep[a] = rep[b];
                        rep[b] = tmp;
                    }
                }
            }

            // Appliquer les règles
            for (int j = 0; j < nbJoueurs; j++) {
                Carte c = rep[j].carte;
                int id  = rep[j].joueurID;

                // IMPORTANT: le serveur impose la bonne valeur "tête"
                c.tete = calculerTetes(c.numero);

                int idx = trouverRangeeCompatible(c, rangees);

                if (idx == -1) {
                    int best = 0;
                    int bestScore = INT_MAX;
                    for (int r = 0; r < NB_RANGEES; r++) {
                        int s = sommeTetes(&rangees[r]);
                        if (s < bestScore) { bestScore = s; best = r; }
                    }

                    printf("J%d ne peut rien placer → ramasse rangée %d (+%d)\n",
                           id, best + 1, bestScore);

                    scores[id] += bestScore;
                    rangees[best].taille = 1;
                    rangees[best].cartes[0] = c;

                } else {
                    Rangee *r = &rangees[idx];

                    if (r->taille == 5) {
                        int s = sommeTetes(r);
                        printf("J%d complète rangée %d → ramasse (+%d)\n",
                               id, idx + 1, s);

                        scores[id] += s;
                        r->taille = 1;
                        r->cartes[0] = c;
                    } else {
                        r->cartes[r->taille++] = c;
                    }
                }
            }

            // Envoyer scores à chaque tour (pratique)
            envoyerScores(sockets, nbJoueurs, scores);

            if (quelquunAtteint(scores, nbJoueurs)) break;
        }

        manche++;
    }

    printf("\n===== FIN DE PARTIE (>= %d) =====\n", SCORE_CIBLE);
    for (int j = 0; j < nbJoueurs; j++)
        printf("Score J%d = %d\n", j, scores[j]);

    // Dire fin aux joueurs
    for (int j = 0; j < nbJoueurs; j++) {
        Message m;
        memset(&m, 0, sizeof(m));
        m.type = MSG_FIN;
        envoyerMessage(sockets[j], &m);
        close(sockets[j]);
    }

    return 0;
}
