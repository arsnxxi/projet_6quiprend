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

int sommeTetesRangee(Rangee *r) {
    int sum = 0;
    for (int i = 0; i < r->taille; i++)
        sum += r->cartes[i].tete;
    return sum;
}

typedef struct {
    float x[21];
    int label;
} Exemple;

typedef struct {
    int top;
    int dist;
    int tetes;
    int taille;
    int risque;
} RangInfo;

void extract_features(Carte carte, Rangee r[4], float out[21])
{
    RangInfo info[4];

    for (int i = 0; i < 4; i++) {

        int top = r[i].cartes[r[i].taille - 1].numero;
        int tetes = sommeTetesRangee(&r[i]);
        int taille = r[i].taille;
        int risque = (taille == 5 || taille == 4);

        info[i].top = top;
        info[i].dist = carte.numero - top;
        info[i].tetes = tetes;
        info[i].taille = taille;
        info[i].risque = risque;
    }

    // Tri par top croissant
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (info[j].top < info[i].top) {
                RangInfo tmp = info[i];
                info[i] = info[j];
                info[j] = tmp;
            }
        }
    }

    // ======== Remplissage des 21 features normalisées ========

    out[0] = carte.numero / 104.0f;

    for (int i = 0; i < 4; i++)
        out[1 + i] = info[i].top / 104.0f;

    for (int i = 0; i < 4; i++)
        out[5 + i] = (info[i].dist + 100.0f) / 200.0f;

    for (int i = 0; i < 4; i++)
        out[9 + i] = info[i].tetes / 15.0f;

    for (int i = 0; i < 4; i++)
        out[13 + i] = info[i].taille / 5.0f;

    for (int i = 0; i < 4; i++)
        out[17 + i] = info[i].risque;
}


int compterCartes(Carte main[10]) {
    int n = 0;
    for (int i = 0; i < 10; i++) {
        if (main[i].numero != 0) {  // carte valide
            n++;
        }
    }
    return n;
}



float distance21(float a[21], float b[21]) {
    float s = 0;
    for (int i = 0; i < 21; i++) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s; // pas besoin de sqrt pour comparer
}


int charger_dataset(const char *filename, Exemple **outData) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir %s\n", filename);
        return 0;
    }

    Exemple *data = malloc(sizeof(Exemple) * 200000); // assez large
    int count = 0;

    while (!feof(f)) {
        Exemple e;
        int r = fscanf(f,
            "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d",
            &e.x[0], &e.x[1], &e.x[2], &e.x[3], &e.x[4],
            &e.x[5], &e.x[6], &e.x[7], &e.x[8],
            &e.x[9], &e.x[10], &e.x[11], &e.x[12],
            &e.x[13], &e.x[14], &e.x[15], &e.x[16],
            &e.x[17], &e.x[18], &e.x[19], &e.x[20],
            &e.label
        );
        if (r == 22) data[count++] = e;
    }

    fclose(f);
    *outData = data;
    return count;
}


int predict_knn(Carte main[10], Rangee r[4], int indexCarte, int k) {
    Exemple *dataset;
    int total = charger_dataset("dataset.csv", &dataset);
    if (total == 0) return -1;

    // 1) Extraire les features de la carte demandée
    float X[21];
    extract_features(main[indexCarte], r, X);

    // 2) Trouver les k plus proches voisins
    Exemple voisins[k];
    float distVoisins[k];

    // Initialisation
    for (int i = 0; i < k; i++) distVoisins[i] = 1e30f;

    for (int i = 0; i < total; i++) {
        float d = distance21(X, dataset[i].x);

        // Vérifier si distance < plus mauvais voisin actuel
        int worst = 0;
        for (int j = 1; j < k; j++)
            if (distVoisins[j] > distVoisins[worst])
                worst = j;

        if (d < distVoisins[worst]) {
            distVoisins[worst] = d;
            voisins[worst] = dataset[i];
        }
    }

     // 3) Moyenne des labels des k voisins
    float sum = 0.0f;
    for (int i = 0; i < k; i++)
        sum += voisins[i].label;

    free(dataset);

    // arrondi à l'entier le plus proche
    return (int)(sum / k + 0.5f);
}



Carte choisirCarteIA(Carte main[10], Rangee r[4]){
    int meilleur=-1;
    int meilleurpred=9999;
    int nbCartes = compterCartes(main);

    for (int i = 0; i < nbCartes; i++) {
        int prediction = predict_knn(main, r, i, 4);
        if (prediction<meilleurpred){
            meilleur=i;
            meilleurpred=prediction;
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
                Carte c = choisirCarteIA(mainJoueur, msg.rangees);

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
