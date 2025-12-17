#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour read, write, close, sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <time.h>

#include <pthread.h>

#define PORT 5000 //(ports >= 5000 réservés pour usage explicite)

#define LG_MESSAGE 256

#define MAX_PARTIES 100

typedef struct {
    int joueur1;
    int joueur2;
    int spectateurs[LG_MESSAGE];
    int nb_spectateurs;
    char tab[9];
} Partie;

Partie parties[MAX_PARTIES];
int nb_parties = 0;

/* Choisi une position aléatoirement */
void position_alea(char grille[9], int *pos_x) {
    int vide[9];
    int nb_vide = 0;

    for (int i = 0; i < 9; i++) {
        if (grille[i] != 'X' && grille[i] != 'O') {
            vide[nb_vide] = i;
            nb_vide++;
        }
    }

    int choix = (rand() % nb_vide);

    *pos_x = vide[choix];
}

/* Compte le nombre d'espace vide contenu dans le tableau */
int check_empty(char grille[9]){
	int nb_vide = 0;

	for ( int i = 0; i < 9; i++){
		if ((grille[i] != 'X') | (grille[i] == 'O')){
			nb_vide = nb_vide + 1;
		}
	}

	return nb_vide;
}

/* Verifie une victoire */
int check_victory(char tab[9], char symbol) {

    for (int i = 0; i < 9; i = i + 3) {
        if (tab[i] == symbol && tab[i + 1] == symbol && tab[i + 2] == symbol) {
            return 1; 
        }
    }

    for (int i = 0; i < 3; i++) {
        if (tab[i] == symbol && tab[i + 3] == symbol && tab[i + 6] == symbol) {
            return 1;
        }
    }

    if (tab[0] == symbol && tab[4] == symbol && tab[8] == symbol) {
        return 1; 
    }
    if (tab[2] == symbol && tab[4] == symbol && tab[6] == symbol) {
        return 1; 
    }

    return 0; 
}

/* Verfication victoire */
void verifier_etat_jeu(char tab[9], char *messageEnvoye) {
    if (check_victory(tab, 'X')) {
        strncpy(messageEnvoye, "Xwins", LG_MESSAGE - 1);
    } else if (check_empty(tab) == 0) {
        strncpy(messageEnvoye, "Xends", LG_MESSAGE - 1);
    } else if (check_victory(tab, 'O')) {
        strncpy(messageEnvoye, "Owins", LG_MESSAGE - 1);
    } else if (check_empty(tab) == 0) {
        strncpy(messageEnvoye, "Oends", LG_MESSAGE - 1);
    } else {
        strncpy(messageEnvoye, "continue", LG_MESSAGE - 1);
    }
}

// Réinitialise une partie
void supprimer_partie(Partie *partie) {
    partie->joueur1 = -1;
    partie->joueur2 = -1;
    for (int i = 0; i < partie->nb_spectateurs; i++) {
        partie->spectateurs[i] = -1;
    }
    partie->nb_spectateurs = 0;
    for (int i = 0; i < 9; i++) {
        partie->tab[i] = '1' + i;
    }
}

// S'occupe du jeu, le modère.
void gerer_partie(Partie *partie) {
    char messageEnvoye[LG_MESSAGE];
    int indiceJoueur1, indiceJoueur2;
    int lus;

    send(partie->joueur1, "startPlayer1", strlen("startPlayer1") + 1, 0);
    send(partie->joueur2, "startPlayer2", strlen("startPlayer2") + 1, 0);

    for (int i = 0; i < partie->nb_spectateurs; i++) {
        send(partie->spectateurs[i], "startSpectateur", strlen("startSpectateur") + 1, 0);
    }

    while (1) {
        lus = recv(partie->joueur1, &indiceJoueur1, sizeof(indiceJoueur1), 0);
        if (lus <= 0) {  
            partie->joueur1 = -1;  
        } else {
            partie->tab[indiceJoueur1] = 'X';
            send(partie->joueur2, &indiceJoueur1, sizeof(indiceJoueur1), 0);
        }

        verifier_etat_jeu(partie->tab, messageEnvoye);
        send(partie->joueur2, messageEnvoye, strlen(messageEnvoye) + 1, 0);
        send(partie->joueur1, messageEnvoye, strlen(messageEnvoye) + 1, 0);

        for (int i = 0; i < partie->nb_spectateurs; i++) {
            send(partie->spectateurs[i], partie->tab, sizeof(partie->tab), 0);
            send(partie->spectateurs[i], messageEnvoye, strlen(messageEnvoye) + 1, 0);
        }

        if (strcmp(messageEnvoye, "continue") != 0) {
            break;
        }

        lus = recv(partie->joueur2, &indiceJoueur2, sizeof(indiceJoueur2), 0);
        if (lus <= 0) {  
            partie->joueur2 = -1;  
        } else {
            partie->tab[indiceJoueur2] = 'O';
            send(partie->joueur1, &indiceJoueur2, sizeof(indiceJoueur2), 0);
        }

        verifier_etat_jeu(partie->tab, messageEnvoye);
        send(partie->joueur1, messageEnvoye, strlen(messageEnvoye) + 1, 0);
        send(partie->joueur2, messageEnvoye, strlen(messageEnvoye) + 1, 0);

        for (int i = 0; i < partie->nb_spectateurs; i++) {
            send(partie->spectateurs[i], partie->tab, sizeof(partie->tab), 0);
            send(partie->spectateurs[i], messageEnvoye, strlen(messageEnvoye) + 1, 0);
        }

        if (strcmp(messageEnvoye, "continue") != 0) {
            break;
        }
    }

    // Fermeture des sockets des joueurs et spectateurs
    if (partie->joueur1 != -1) {
        close(partie->joueur1);
    }
    if (partie->joueur2 != -1) {
        close(partie->joueur2);
    }
    for (int i = 0; i < partie->nb_spectateurs; i++) {
        if (partie->spectateurs[i] != -1) {
            close(partie->spectateurs[i]);
        }
    }

    supprimer_partie(partie);
}

void *gerer_partie_thread(void *arg) {
    Partie *partie = (Partie *)arg;
    gerer_partie(partie);
    return NULL; // fin du thread
}

int main(int argc, char *argv[]){
	int socketEcoute;

	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;

	int socketDialogue;
	int joueur1, joueur2, nb_spectateur;
	int spectateurs[LG_MESSAGE];
	
	struct sockaddr_in pointDeRencontreDistant;
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
	char messageEnvoye[LG_MESSAGE];
	int ecrits, lus; /* nb d’octets ecrits et lus */
	int retour;

	srand(time(NULL));

	// Crée un socket de communication
	socketEcoute = socket(AF_INET, SOCK_STREAM, 0); 
	// Teste la valeur renvoyée par l’appel système socket() 
	if(socketEcoute < 0){
		perror("socket"); // Affiche le message d’erreur 
	exit(-1); // On sort en indiquant un code erreur
	}
	printf("Socket créée avec succès ! (%d)\n", socketEcoute); // On prépare l’adresse d’attachement locale
	//setsockopt()


	// Remplissage de sockaddrDistant (structure sockaddr_in identifiant le point d'écoute local)
	longueurAdresse = sizeof(pointDeRencontreLocal);
	// memset sert à faire une copie d'un octet n fois à partir d'une adresse mémoire donnée
	// ici l'octet 0 est recopié longueurAdresse fois à partir de l'adresse &pointDeRencontreLocal
	memset(&pointDeRencontreLocal, 0x00, longueurAdresse); pointDeRencontreLocal.sin_family = PF_INET;
	pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // attaché à toutes les interfaces locales disponibles
	pointDeRencontreLocal.sin_port = htons(PORT); // = 5000 ou plus
	
	// On demande l’attachement local de la socket
	if((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0) {
		perror("bind");
		exit(-2); 
	}
	printf("Socket attachée avec succès !\n");

	// On fixe la taille de la file d’attente à 5 (pour les demandes de connexion non encore traitées)
	if(listen(socketEcoute, 5) < 0){
   		perror("listen");
   		exit(-3);
	}
	printf("Socket placée en écoute passive ...\n");


	/*for (int i = 0; i < MAX_PARTIES; i++){
		parties[i].joueur1 = -1;
		parties[i].joueur2 = -1;
	}*/
	

	while (1){

		printf("Attente des connexions ...\n");
        int client = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);
        if (client < 0) {
            perror("accept");
            continue;
        }

        char role;
        recv(client, &role, sizeof(role), 0);

        if (role == 'J') {
            // Joueur
            int assigned = 0;
            for (int i = 0; i < nb_parties; i++) {
                if (parties[i].joueur1 == 0 || parties[i].joueur2 == 0) {
                    if (parties[i].joueur1 == 0) {
                        parties[i].joueur1 = client;
                    } else {
                        parties[i].joueur2 = client;
                    }
                    assigned = 1;

                    if (parties[i].joueur1 != 0 && parties[i].joueur2 != 0) {
                        pthread_t thread_id;
                        pthread_create(&thread_id, NULL, gerer_partie_thread, (void *)&parties[i]);
                        pthread_detach(thread_id);
                    }
                    break;
                }
            }

            if (!assigned) {
                Partie nouvellePartie = {0};
                nouvellePartie.joueur1 = client;
                for (int i = 0; i < 9; i++) {
					nouvellePartie.tab[i] = '1' + i;
				}
                parties[nb_parties++] = nouvellePartie;
            }
        } else if (role == 'S') {
            // Spectateur
            char partie_dispo[LG_MESSAGE] = "";
            for (int i = 0; i < nb_parties; i++) {
                if (parties[i].joueur1 == -1 || parties[i].joueur2 == -1) { // la partie est termine 
                    continue;
                }
                char buf[32];
                snprintf(buf, sizeof(buf), "Partie %d\n", i);
                strncat(partie_dispo, buf, sizeof(partie_dispo) - strlen(partie_dispo) - 1);
            }

            send(client, partie_dispo, strlen(partie_dispo) + 1, 0);

            int choix;
            recv(client, &choix, sizeof(choix), 0);
            if (choix >= 0 && choix < nb_parties) {
                Partie *partie = &parties[choix];
                partie->spectateurs[partie->nb_spectateurs++] = client;
            }
        }
		        
	}
	

	// On ferme la ressource avant de quitter
   	close(socketEcoute);
	return 0; 
}
