#include "commun.h"
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

static void error(const char *msg) {
    perror(msg);
    exit(1);
}

static int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0)  return 2;
    return 1;
}

static int read_full(int fd, void *buf, size_t n) {
    size_t got = 0;
    char *p = (char*)buf;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r == 0) return 0;
        if (r < 0) return -1;
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

static int trouverRangeeCompatible(Carte c, Rangee rangees[]) {
    int idx = -1;
    int ecartMin = 999999;

    for (int i = 0; i < NB_RANGEES; i++) {
        int last = rangees[i].cartes[rangees[i].taille - 1].numero;
        if (c.numero > last) {
            int ecart = c.numero - last;
            if (ecart < ecartMin) { ecartMin = ecart; idx = i; }
        }
    }
    return idx;
}

static Carte choisirCarteIA(Carte main[], int nbCartes, Rangee rangees[]) {
    int meilleur = -1;
    int valeurMin = 999999;

    // plus petite compatible
    for (int i = 0; i < nbCartes; i++) {
        int idx = trouverRangeeCompatible(main[i], rangees);
        if (idx != -1 && main[i].numero < valeurMin) {
            valeurMin = main[i].numero;
            meilleur = i;
        }
    }

    // sinon plus petite
    if (meilleur == -1) {
        valeurMin = 999999;
        for (int i = 0; i < nbCartes; i++) {
            if (main[i].numero < valeurMin) {
                valeurMin = main[i].numero;
                meilleur = i;
            }
        }
    }

    return main[meilleur];
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    int port = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("socket");

    struct hostent *server = gethostbyname(argv[1]);
    if (!server) error("gethostbyname");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, (size_t)server->h_length);
    serv_addr.sin_port = htons((uint16_t)port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("connect");

    Message msg;
    Carte mainJoueur[NB_CARTES_MAIN];
    int nbCartes = 0;
    int joueurID = -1;

    while (1) {
        int st = read_full(sockfd, &msg, sizeof(Message));
        if (st <= 0) break;

        switch (msg.type) {

            case MSG_INIT:
                joueurID = msg.joueurID;
                memcpy(mainJoueur, msg.mainJoueur, sizeof(mainJoueur));
                nbCartes = NB_CARTES_MAIN;
                for (int i = 0; i < nbCartes; i++)
                    mainJoueur[i].tete = calculerTetes(mainJoueur[i].numero);
                break;

            case MSG_TOUR: {
                Carte c = choisirCarteIA(mainJoueur, nbCartes, msg.rangees);

                // retirer de la main
                int index = -1;
                for (int i = 0; i < nbCartes; i++) {
                    if (mainJoueur[i].numero == c.numero) { index = i; break; }
                }
                for (int i = index; i < nbCartes - 1; i++)
                    mainJoueur[i] = mainJoueur[i + 1];
                nbCartes--;

                c.tete = calculerTetes(c.numero);

                Message rep;
                memset(&rep, 0, sizeof(rep));
                rep.type = MSG_CARTE;
                rep.joueurID = joueurID;
                rep.carte = c;

                write_full(sockfd, &rep, sizeof(rep));
                break;
            }

            case MSG_FIN:
                close(sockfd);
                return 0;

            default:
                break;
        }
    }

    close(sockfd);
    return 0;
}
