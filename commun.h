#ifndef COMMUN_H
#define COMMUN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NB_RANGEES 4
#define NB_CARTES_MAIN 10
#define NB_CARTES_TOTAL 104
#define MAX_JOUEURS 10

// --------------------------------------------
// STRUCTURES DU JEU
// --------------------------------------------

typedef struct {
    int numero;       // 1 à 104
    int teteBoeufs;   // poids de la carte
} Carte;

typedef struct {
    Carte cartes[5];  // jusqu'à 5 cartes
    int taille;
} Rangee;

typedef struct {
    int scores[MAX_JOUEURS];
    int nbJoueurs;
    Rangee rangees[NB_RANGEES];
} EtatPartie;


// --------------------------------------------
// MESSAGES ÉCHANGÉS ENTRE PROCESSUS
// --------------------------------------------

typedef enum {
    MSG_INIT,      // Envoi de la main initiale
    MSG_RANGEES,   // Envoi de l’état des rangées
    MSG_TOUR,      // Le joueur doit jouer une carte
    MSG_CARTE,     // Réponse du joueur = une carte
    MSG_FIN,       // Fin de partie
    MSG_SCORE      // Mise à jour des scores (optionnel)
} TypeMessage;


typedef struct {
    TypeMessage type;
    int joueurID;

    Carte carte;                      // utilisé dans MSG_CARTE
    Carte mainJoueur[NB_CARTES_MAIN]; // pour MSG_INIT
    Rangee rangees[NB_RANGEES];       // pour MSG_RANGEES
    int scores[MAX_JOUEURS];          // pour MSG_SCORE
} Message;

#endif
