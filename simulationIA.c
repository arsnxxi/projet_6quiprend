#include "commun.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NB_EXEMPLES 10000

/* ===================== UTILITAIRES ====================== */

int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0) return 2;
    return 1;
}

/* Calcule la somme réelle des têtes dans une rangée */
int sommeTetesRangee(Rangee *r) {
    int sum = 0;
    for (int i = 0; i < r->taille; i++)
        sum += r->cartes[i].tete;
    return sum;
}


/* ===================== FEATURES ====================== */

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
        int risque = (taille == 5 || taille==4);

        info[i].top = top;
        info[i].dist = carte.numero - top;
        info[i].tetes = tetes;
        info[i].taille = taille;
        info[i].risque = risque;
    }

    // Tri des 4 rangées par carte top croissante
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (info[j].top < info[i].top) {
                RangInfo tmp = info[i];
                info[i] = info[j];
                info[j] = tmp;
            }
        }
    }

    // 0 = numéro de la carte jouée
    out[0] = carte.numero;

    for (int i = 0; i < 4; i++) out[1 + i] = info[i].top;
    for (int i = 0; i < 4; i++) out[5 + i] = info[i].dist;
    for (int i = 0; i < 4; i++) out[9 + i] = info[i].tetes;
    for (int i = 0; i < 4; i++) out[13 + i] = info[i].taille;
    for (int i = 0; i < 4; i++) out[17 + i] = info[i].risque;
}


/* ===================== IA SIMPLE ====================== */

int trouverRangeeCompatible(Carte c, Rangee rangees[])
{
    int best = -1;
    int ecartMin = 99999;

    for (int i = 0; i < 4; i++) {
        int top = rangees[i].cartes[rangees[i].taille - 1].numero;

        if (c.numero > top) {
            int e = c.numero - top;
            if (e < ecartMin) {
                ecartMin = e;
                best = i;
            }
        }
    }
    return best;
}

Carte decision_ia_simple(Carte main[], int n, Rangee r[])
{
    int bestIdx = -1;
    int bestVal = 99999;

    // Cherche plus petite compatible
    for (int i = 0; i < n; i++) {
        if (main[i].numero < bestVal) {
            int found = trouverRangeeCompatible(main[i], r);
            if (found != -1) {
                bestVal = main[i].numero;
                bestIdx = i;
            }
        }
    }

    // Sinon: plus petite carte
    if (bestIdx == -1) {
        bestVal = 99999;
        for (int i = 0; i < n; i++) {
            if (main[i].numero < bestVal) {
                bestIdx = i;
                bestVal = main[i].numero;
            }
        }
    }

    return main[bestIdx];
}


/* ===================== GENERATION ====================== */

void generer_situation(Carte *maCarte, Rangee r[4],
                       Carte main1[10], Carte main2[10], Carte main3[10])
{
    // Carte de test
    maCarte->numero = rand() % 104 + 1;
    maCarte->tete = calculerTetes(maCarte->numero);

    // Génération rangées réalistes
    for (int i = 0; i < 4; i++) {

        int top = rand() % 104 + 1;
        int taille = rand() % 5 + 1;

        r[i].taille = taille;

        // Remplit une rangée strictement croissante
        int value = top - (taille - 1);
        if (value < 1) value = 1;

        for (int k = 0; k < taille; k++) {
            r[i].cartes[k].numero = value + k;
            r[i].cartes[k].tete = calculerTetes(value + k);
        }
    }

    // Mains adverses
    for (int i = 0; i < 10; i++) {

        main1[i].numero = rand() % 104 + 1;
        main1[i].tete = calculerTetes(main1[i].numero);

        main2[i].numero = rand() % 104 + 1;
        main2[i].tete = calculerTetes(main2[i].numero);

        main3[i].numero = rand() % 104 + 1;
        main3[i].tete = calculerTetes(main3[i].numero);
    }
}


/* ===================== SIMULATION (LABEL) ====================== */

int calculer_label(Carte maCarte, Rangee rin[4],
                   Carte main1[10], Carte main2[10], Carte main3[10])
{
    // On copie les rangées pour ne PAS modifier celles du dataset
    Rangee r[4];
    for (int i = 0; i < 4; i++) r[i] = rin[i];

    Carte played[4];
    played[0] = maCarte;
    played[1] = decision_ia_simple(main1, 10, r);
    played[2] = decision_ia_simple(main2, 10, r);
    played[3] = decision_ia_simple(main3, 10, r);

    // Trier les cartes jouées
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (played[j].numero < played[i].numero) {
                Carte tmp = played[i];
                played[i] = played[j];
                played[j] = tmp;
            }
        }
    }

    int tetes_prises = 0;

    for (int p = 0; p < 4; p++) {

        Carte c = played[p];
        int estMoi = (c.numero == maCarte.numero);

        int idx = trouverRangeeCompatible(c, r);

        // Si aucune compatible → ramasse la moins coûteuse
        if (idx == -1) {

            int minT = 99999;
            int best = 0;

            for (int i = 0; i < 4; i++) {
                int t = sommeTetesRangee(&r[i]);
                if (t < minT) {
                    minT = t;
                    best = i;
                }
            }

            if (estMoi) tetes_prises += sommeTetesRangee(&r[best]);

            r[best].taille = 1;
            r[best].cartes[0] = c;
            continue;
        }

        // Compatible mais pleine ?
        if (r[idx].taille == 5) {

            if (estMoi) tetes_prises += sommeTetesRangee(&r[idx]);

            r[idx].taille = 1;
            r[idx].cartes[0] = c;
            continue;
        }

        // Sinon poser dans la rangée
        r[idx].cartes[r[idx].taille] = c;
        r[idx].taille++;
    }

    return tetes_prises;
}


/* ===================== MAIN ====================== */

int main() {

    srand(time(NULL));

    FILE *f = fopen("dataset.csv", "w");
    if (!f) {
        printf("Erreur ouverture fichier\n");
        return 1;
    }

    for (int i = 0; i < NB_EXEMPLES; i++) {

        Carte maCarte;
        Rangee rangees[4];
        Carte main1[10], main2[10], main3[10];

        // Génération situation
        generer_situation(&maCarte, rangees, main1, main2, main3);

        // Features
        float features[21];
        extract_features(maCarte, rangees, features);

        // Label
        int label = calculer_label(maCarte, rangees, main1, main2, main3);

        // CSV
        for (int j = 0; j < 21; j++)
            fprintf(f, "%f,", features[j]);

        fprintf(f, "%d\n", label);
    }

    fclose(f);
    return 0;
}
