#include "commun.h"
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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

static void afficherRangees(Rangee r[]) {
    printf("\n===== RANGÉES =====\n");
    for (int i = 0; i < NB_RANGEES; i++) {
        printf("R%d : ", i + 1);
        for (int j = 0; j < r[i].taille; j++) {
            printf("[%d (%d)] ", r[i].cartes[j].numero, r[i].cartes[j].tete);
        }
        printf("\n");
    }
}

static void afficherMain(Carte main[], int nb) {
    printf("\n--- TA MAIN ---\n");
    for (int i = 0; i < nb; i++) {
        printf("%2d) [%d (%d)]\n", i + 1, main[i].numero, main[i].tete);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }

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

    printf(">>> Connecté au serveur.\n");

    Message msg;
    Carte mainJoueur[NB_CARTES_MAIN];
    int nbCartes = 0;
    int joueurID = -1;

    while (1) {
        int st = read_full(sockfd, &msg, sizeof(Message));
        if (st <= 0) {
            printf("Déconnexion serveur.\n");
            break;
        }

        switch (msg.type) {

            case MSG_INIT:
                joueurID = msg.joueurID;
                memcpy(mainJoueur, msg.mainJoueur, sizeof(mainJoueur));
                nbCartes = NB_CARTES_MAIN;

                // garantir que les têtes sont correctes
                for (int i = 0; i < nbCartes; i++)
                    mainJoueur[i].tete = calculerTetes(mainJoueur[i].numero);

                printf("\n=== Nouvelle manche ! Tu es J%d ===\n", joueurID);
                afficherMain(mainJoueur, nbCartes);
                break;

            case MSG_RANGEES:
                afficherRangees(msg.rangees);
                break;

            case MSG_SCORE:
                printf("\n--- SCORES ---\n");
                for (int i = 0; i < MAX_JOUEURS; i++) {
                    if (msg.scores[i] >= 0)
                        printf("J%d = %d\n", i, msg.scores[i]);
                }
                break;

            case MSG_TOUR: {
                afficherMain(mainJoueur, nbCartes);

                int choix = -1;
                do {
                    printf("\nChoisis une carte (1..%d): ", nbCartes);
                    fflush(stdout);
                    if (scanf("%d", &choix) != 1) {
                        int ch;
                        while ((ch = getchar()) != '\n' && ch != EOF) {}
                        choix = -1;
                    }
                } while (choix < 1 || choix > nbCartes);

                Carte c = mainJoueur[choix - 1];

                // retirer de la main
                for (int i = choix - 1; i < nbCartes - 1; i++)
                    mainJoueur[i] = mainJoueur[i + 1];
                nbCartes--;

                // sécurité têtes
                c.tete = calculerTetes(c.numero);

                Message rep;
                memset(&rep, 0, sizeof(rep));
                rep.type = MSG_CARTE;
                rep.joueurID = joueurID;
                rep.carte = c;

                if (write_full(sockfd, &rep, sizeof(rep)) <= 0) {
                    printf("Erreur envoi.\n");
                    close(sockfd);
                    return 1;
                }

                printf(">>> Tu as joué [%d (%d)]\n", c.numero, c.tete);
                break;
            }

            case MSG_FIN:
                printf("\n=== Fin de partie ===\n");
                close(sockfd);
                return 0;

            default:
                break;
        }
    }

    close(sockfd);
    return 0;
}
