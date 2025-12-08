#ifndef COMMUN_H
#define COMMUN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// --------------------------------------------
// CONSTANTES DU JEU
// --------------------------------------------

#define NB_RANGEES 4
#define NB_CARTES_MAIN 10
#define NB_CARTES_TOTAL 104

// --------------------------------------------
// STRUCTURES DE BASE
// --------------------------------------------

typedef struct {
    int numero;       // numéro de la carte (1 à 104)
    int teteBoeufs;   // pénalité de la carte
} Carte;

typedef struct {
    Carte cartes[5];  // 5 cartes max par rangée
    int taille;        // nombre de cartes dans la rangée (0-5)
} Rangee;

typedef struct {
    Rangee rangees[NB_RANGEES];
    int nbJoueurs;
    int scores[10]; // max 10 joueurs
} Partie;

// --------------------------------------------
// TYPES DE MESSAGES ECHANGES PAR PIPE
// --------------------------------------------

typedef enum {
    MSG_INIT,         // le gestionnaire envoie la main initiale
    MSG_TOUR,         // début d'un nouveau tour : le joueur doit renvoyer une carte
    MSG_RANGEES,      // le gestionnaire envoie l’état des rangées
    MSG_FIN,          // fin de partie
    MSG_CARTE         // message contenant une carte choisie par le joueur
} TypeMessage;


// Message générique circulant dans les pipes
typedef struct {
    TypeMessage type;
    Carte carte;      // utilisé pour MSG_CARTE
    Carte mainJoueur[NB_CARTES_MAIN];  // pour MSG_INIT
    Rangee rangees[NB_RANGEES];        // pour MSG_RANGEES
    int joueurID;     // numéro du joueur
} Message;

#endif
