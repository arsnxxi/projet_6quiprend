#include "commun.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



#define NB_EXEMPLES 10

int calculerTetes(int numero) {
    if (numero == 55) return 7;
    if (numero % 11 == 0) return 5;
    if (numero % 10 == 0) return 3;
    if (numero % 5 == 0) return 2;
    return 1;
}

typedef struct {
    int top;
    int tetes;
    int taille;
    int risque;
} RangInfo;

void extract_features(Carte carte, Rangee r[], float out[21]) {

    RangInfo info[4];

    // remplir la structure
    for (int i = 0; i < 4; i++) {
        info[i].top = r[i].cartes[r[i].taille - 1].numero;
        info[i].tetes = r[i].cartes[r[i].taille-1].tete;
        info[i].taille = r[i].taille;
        info[i].risque = (r[i].taille >= 4) ? 1 : 0;
    }

    // trier selon info.top (croissant)
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (info[j].top < info[i].top) {
                RangInfo tmp = info[i];
                info[i] = info[j];
                info[j] = tmp;
            }
        }
    }

    // 0 : carte jouée
    out[0] = carte.numero;

    // features 1–4 : tops triés
    for (int i = 0; i < 4; i++)
        out[1 + i] = info[i].top;

    // features 5–8 : distances
    for (int i = 0; i < 4; i++)
        out[5 + i] = carte.numero - info[i].top;

    // features 9–12 : têtes
    for (int i = 0; i < 4; i++)
        out[9 + i] = info[i].tetes;

    // features 13–16 : taille
    for (int i = 0; i < 4; i++)
        out[13 + i] = info[i].taille;

    // features 17–20 : risque
    for (int i = 0; i < 4; i++)
        out[17 + i] = info[i].risque;
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

Carte decision_ia_simple(Carte main[], int n, Rangee r[]) {
    int meilleur = -1;
    int valeurMin = 99999;

    // 1) essayer de jouer la plus petite carte compatible
    for (int i = 0; i < n; i++) {
        int idx = trouverRangeeCompatible(main[i], r);
        if (idx != -1) {
            if (main[i].numero < valeurMin) {
                valeurMin = main[i].numero;
                meilleur = i;
            }
        }
    }

    // 2) si aucune carte n'est compatible, jouer la plus petite
    if (meilleur == -1) {
        for (int i = 0; i < n; i++) {
            if (main[i].numero < valeurMin) {
                valeurMin = main[i].numero;
                meilleur = i;
            }
        }
    }

    return main[meilleur];
}













void generer_situation(Carte *maCarte, Rangee r[4],Carte main1[10], Carte main2[10], Carte main3[10]) 
{
    // 1) Générer LA carte de l'IA (test)
    maCarte->numero = rand() % 104 + 1;
    maCarte->tete = calculerTetes(maCarte->numero);

    // 2) Générer 4 rangées cohérentes
    for (int i = 0; i < 4; i++) {
        int top = rand() % 104 + 1;       // top card anywhere 1-104
        int taille = rand() % 5 + 1;      // profondeur 1..5

        // Remplir la rangée minimale (UNIQUEMENT top)
        r[i].taille = taille;
        r[i].cartes[r[i].taille-1].numero = top;
        r[i].cartes[r[i].taille-1].tete = calculerTetes(top);

        // Générer la somme des têtes en fonction de la taille
        int totalTetes = r[i].cartes[r[i].taille-1].tete;
        for (int k = 0; k < taille-1; k++) {
            int t = calculerTetes(rand() % 104 + 1);
            totalTetes += t;
        }

        // Stocker les têtes dans toutes les cartes par la dernière
        
        r[i].cartes[r[i].taille-1].tete = totalTetes;
    }

    // 3) Générer 3 mains adverses (cartes aléatoires)
    for (int i = 0; i < 10; i++) {
        main1[i].numero = rand() % 104 + 1;
        main1[i].tete = calculerTetes(main1[i].numero);

        main2[i].numero = rand() % 104 + 1;
        main2[i].tete = calculerTetes(main2[i].numero);

        main3[i].numero = rand() % 104 + 1;
        main3[i].tete = calculerTetes(main3[i].numero);
    }
}


int calculer_label(Carte maCarte, Rangee r[4],
                   Carte main1[10], Carte main2[10], Carte main3[10])
{
    Carte played[4];

    // 1) Ta carte
    played[0] = maCarte;

    // 2) Cartes adverses
    played[1] = decision_ia_simple(main1,10, r);
    played[2] = decision_ia_simple(main2,10, r);
    played[3] = decision_ia_simple(main3,10, r);

    // 3) Trier les cartes jouées par ordre croissant
    for (int i = 0; i < 4; i++) {
        for (int j = i+1; j < 4; j++) {
            if (played[j].numero < played[i].numero) {
                Carte tmp = played[i];
                played[i] = played[j];
                played[j] = tmp;
            }
        }
    }

    int tetes_prises = 0;

    // 4) Simuler les 4 placements
    for (int p = 0; p < 4; p++) {
        Carte c = played[p];
        int joueur_est_moi = (c.numero == maCarte.numero);

        int bestIdx = -1;
        int ecartMin = 9999;

        // Chercher rangée compatible
        for (int i = 0; i < 4; i++) {
            int top = r[i].cartes[r[i].taille - 1].numero;
            if (c.numero > top) {
                int ecart = c.numero - top;
                if (ecart < ecartMin) {
                    ecartMin = ecart;
                    bestIdx = i;
                }
            }
        }

        // Si aucune compatible → ramasser la rangée la moins coûteuse
        if (bestIdx == -1) {
            int minTetes = 9999;
            int idxMin = 0;
            for (int i = 0; i < 4; i++) {
                if (r[i].cartes[r[i].taille-1].tete < minTetes) {
                    minTetes = r[i].cartes[r[i].taille-1].tete;
                    idxMin = i;
                }
            }

            if (joueur_est_moi) tetes_prises += r[idxMin].cartes[r[idxMin].taille-1].tete;

            // Réinitialiser la rangée avec la carte jouée
            r[idxMin].taille = 1;
            r[idxMin].cartes[0] = c;
            r[idxMin].cartes[r[idxMin].taille-1].tete = c.tete;

            continue;
        }

        // Rangée compatible → vérifier si pleine
        if (r[bestIdx].taille == 5) {
            // ramassage
            if (joueur_est_moi) tetes_prises += r[bestIdx].cartes[r[bestIdx].taille-1].tete;

            r[bestIdx].taille = 1;
            r[bestIdx].cartes[0] = c;
            r[bestIdx].cartes[r[bestIdx].taille].tete = c.tete;

            continue;
        }

        // Sinon on pose simplement la carte
        int pos = r[bestIdx].taille;
        r[bestIdx].cartes[pos] = c;
        r[bestIdx].cartes[r[bestIdx].taille-1].tete += c.tete;
        r[bestIdx].taille++;
    }

    return tetes_prises;
}


int main() {
    srand(time(NULL));
    FILE *f = fopen("dataset.csv", "w");

    for (int i = 0; i < NB_EXEMPLES; i++) {

        Carte maCarte;
        Rangee rangees[4];

        Carte main1[10];
        Carte main2[10];
        Carte main3[10];

        // Génère une situation complète
        generer_situation(&maCarte, rangees, main1, main2, main3);

        // Extract features
        float features[21];
        extract_features(maCarte, rangees, features);

        // Simule le tour et calcule le label
        int label = calculer_label(maCarte, rangees, main1, main2, main3);

        // Écriture CSV
        for (int j = 0; j < 21; j++)
            fprintf(f, "%f,", features[j]);

        fprintf(f, "%d\n", label);
    }

    fclose(f);
    return 0;
}
