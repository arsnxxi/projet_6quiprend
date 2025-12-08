#include "commun.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



/* Calcule la somme réelle des têtes dans une rangée */
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

int knn_vote(const Exemple voisins[], int k) {
    int bestLabel = 0;
    int bestScore = -1;

    for (int i = 0; i < k; i++) {
        int label = voisins[i].label;
        int count = 0;

        for (int j = 0; j < k; j++)
            if (voisins[j].label == label) count++;

        if (count > bestScore) {
            bestScore = count;
            bestLabel = label;
        }
    }
    return bestLabel;
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





int main() {


    
    // === EXEMPLE D’UNE MAIN ===
    Carte mainJoueur[10] = {
        {92,1}, {104,1}, {0,0}, {0,0}, {0,0},
        {0,0}, {0,0}, {0,0}, {0,0}, {0,0}  // <- 0 signifie carte absente
    };

    // === EXEMPLE DE RANGÉES ===
    Rangee rangees[4];

    rangees[0].taille = 5;
    rangees[0].cartes[0] = (Carte){14,1};
    rangees[0].cartes[1] = (Carte){20,3};
    rangees[0].cartes[2] = (Carte){22,5};
    rangees[0].cartes[3] = (Carte){24,1};
    rangees[0].cartes[4] = (Carte){29,1};

    rangees[1].taille = 5;
    rangees[1].cartes[0] = (Carte){55,7};
    rangees[1].cartes[1] = (Carte){63,1};
    rangees[1].cartes[2] = (Carte){77,5};
    rangees[1].cartes[3] = (Carte){87,1};
    rangees[1].cartes[4] = (Carte){89,1};

    rangees[2].taille = 1;
    rangees[2].cartes[0] = (Carte){26,1};
    //rangees[2].cartes[1] = (Carte){104,1};
    //rangees[2].cartes[2] = (Carte){52,1};

    rangees[3].taille = 2;
    rangees[3].cartes[0] = (Carte){81,1};
    rangees[3].cartes[1] = (Carte){83,1};
    //rangees[3].cartes[2] = (Carte){70,3};
    //rangees[3].cartes[3] = (Carte){74,1};
    //rangees[3].cartes[4] = (Carte){78,1};





    int nbCartes = compterCartes(mainJoueur);

    for (int i = 0; i < nbCartes; i++) {
        int prediction = predict_knn(mainJoueur, rangees, i, 4);
        printf("Carte %d = prediction : %d tetes\n",
               mainJoueur[i].numero, prediction);
    }
    printf("Appuyez sur Entrée pour quitter...");
    getchar();
    getchar();

    return 0;

}
