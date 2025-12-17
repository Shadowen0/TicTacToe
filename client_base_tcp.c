#include <stdio.h>
#include <stdbool.h> 
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour read, write, close, sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */

#define LG_MESSAGE 256

// Affichage de la grille/ du plateau de jeu.
void affichage(char tableau[], int taille){
    for (int i = 0; i < taille; i++) {
        printf("| %c ", tableau[i]); 
        if ((i + 1) % 3 == 0) {  
            printf("|\n");  
            printf("|---|---|---|\n");  
        }
    }
    printf("\n");
}

// Vérifie que la position entrée par le joueur est correcte.
int check_position(int position, char plateau[], int joueur)  {
    if((position < 0) | (position > 8))  {
        printf("La position rentrée est incorrecte.");
        return 0;
    }
    else  {
        if(plateau[position] == 'X' || plateau[position] == 'O')  {
            printf("La position rentrée est déjà occupée.");
            return 0;
        }
        else {
            if(joueur == 1)  {
                plateau[position] = 'X';
                return 1;
            }
            else  {
                plateau[position] = 'O';
                return 1;
            }
        }        
    }
}

//Demande au joueur d'entrer une position et vérifie si elle est correcte.
void input_verificator(char tab[9], int *position, int joueur) {
    do {
        printf("Veuillez rentrer la position dans laquelle vous voulez jouer (1-9) : ");
        scanf("%i", position);
		printf("\n");
        (*position)--;
    } while (check_position(*position, tab, joueur) == 0); 
}

int main(int argc, char *argv[]){
	int descripteurSocket;
	struct sockaddr_in sockaddrDistant;
	socklen_t longueurAdresse;

	char buffer[LG_MESSAGE]; // buffer stockant le message
	char reponse[LG_MESSAGE];
	int nb; /* nb d’octets écrits et lus */

	char tab[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

	char ip_dest[16];
	int  port_dest;

	// Pour pouvoir contacter le serveur, le client doit connaître son adresse IP et le port de comunication
	// Ces 2 informations sont passées sur la ligne de commande
	// Si le serveur et le client tournent sur la même machine alors l'IP locale fonctionne : 127.0.0.1
	// Le port d'écoute du serveur est 5000 dans cet exemple, donc en local utiliser la commande :
	// ./client_base_tcp 127.0.0.1 5000
	if (argc>1) { // si il y a au moins 2 arguments passés en ligne de commande, récupération ip et port
		strncpy(ip_dest,argv[1],16);
		sscanf(argv[2],"%d",&port_dest);
	}else{
		printf("USAGE : %s ip port\n",argv[0]);
		exit(-1);
	}

	// Crée un socket de communication
	descripteurSocket = socket(AF_INET, SOCK_STREAM, 0);
	// Teste la valeur renvoyée par l’appel système socket()
	if(descripteurSocket < 0){
		perror("Erreur en création de la socket..."); // Affiche le message d’erreur
		exit(-1); // On sort en indiquant un code erreur
	}
	printf("Socket créée! (%d)\n", descripteurSocket);


	// Remplissage de sockaddrDistant (structure sockaddr_in identifiant la machine distante)
	// Obtient la longueur en octets de la structure sockaddr_in
	longueurAdresse = sizeof(sockaddrDistant);
	// Initialise à 0 la structure sockaddr_in
	// memset sert à faire une copie d'un octet n fois à partir d'une adresse mémoire donnée
	// ici l'octet 0 est recopié longueurAdresse fois à partir de l'adresse &sockaddrDistant
	memset(&sockaddrDistant, 0x00, longueurAdresse);
	// Renseigne la structure sockaddr_in avec les informations du serveur distant
	sockaddrDistant.sin_family = AF_INET;
	// On choisit le numéro de port d’écoute du serveur
	sockaddrDistant.sin_port = htons(port_dest);
	// On choisit l’adresse IPv4 du serveur
	inet_aton(ip_dest, &sockaddrDistant.sin_addr);

	// Débute la connexion vers le processus serveur distant
	if((connect(descripteurSocket, (struct sockaddr *)&sockaddrDistant,longueurAdresse)) == -1){
		perror("Erreur de connection avec le serveur distant...");
		close(descripteurSocket);
		exit(-2); // On sort en indiquant un code erreur
	}
	printf("Connexion au serveur %s:%d réussie!\n",ip_dest,port_dest);

	char role;
	do {
		printf("Que voulez-vous faire ? (J pour Joueur / S pour Spectateur) : ");
		scanf(" %c", &role);
	} while (role != 'J' && role != 'S');

	send(descripteurSocket, &role, sizeof(role), 0);

	if (role == 'S') {
		char parties_disponibles[LG_MESSAGE];
		recv(descripteurSocket, parties_disponibles, LG_MESSAGE, 0);
		printf("Parties disponibles :\n%s", parties_disponibles);

		int choix;
		printf("Entrez le numéro de la partie à rejoindre : ");
		scanf("%d", &choix);
		send(descripteurSocket, &choix, sizeof(choix), 0);

		printf("Vous êtes maintenant spectateur de la partie %d\n", choix);

		printf("\nVous allez pouvoir voir la partie a partir du prochain tour.\n\n");
	
	} else {
		printf("\nAttendez le debut de la partie.....\n\n");
	}


	int joueur, ordinateur, etat;
	switch (nb = recv(descripteurSocket, buffer, LG_MESSAGE, 0)){
		case -1:
				perror("Erreur de réception");
				close(descripteurSocket);
				exit(-3);
			break;

		case 0:
				fprintf(stderr, "Le socket a été fermée par le serveur !\n\n");
				return 0;
			break;
		
		default:
				//printf ("%s\n\n",buffer);
				// Définit l'indice du joueur.
				if (strcmp(buffer, "startPlayer1") == 0) {	
				printf ("Vous êtes le joueur numéro 1 !\n\n");
				affichage(tab, sizeof(tab));
					do  {
						affichage(tab, sizeof(tab));
						
						input_verificator(tab,&joueur,1);
						
						affichage(tab, sizeof(tab));
						
						send(descripteurSocket, &joueur, sizeof(joueur),0);

						etat = recv(descripteurSocket, &reponse, LG_MESSAGE, 0);
						if (strcmp(reponse, "continue") == 0){
							nb = recv(descripteurSocket, &ordinateur, sizeof(ordinateur),0);
							tab[ordinateur] = 'O';

							etat = recv(descripteurSocket, &reponse, LG_MESSAGE, 0);
						}

						//printf("%s\n", reponse);
					}
					while(strcmp(reponse, "continue") == 0);

					if(strcmp(reponse, "Xwins") == 0)  {
						printf("Vous avez gagné la partie !\n\n");
					}
					else if((strcmp(reponse, "Xends") == 0) | (strcmp(reponse, "Oends") == 0))  {
						printf("Égalité ! Personne n'a gagné la partie, tout le monde a perdu.\n\n");
					}
					else if(strcmp(reponse, "Owins") == 0)  {
						printf("Vous avez perdu la partie.\n\n");
					}
					affichage(tab, sizeof(tab));

				}  else  if (strcmp(buffer, "startPlayer2")==0) {
					printf ("Vous êtes le joueur numéro 2 !\n\n");
					
					do  {
						printf("\n----------------------------------------------\n");
						nb = recv(descripteurSocket, &ordinateur, sizeof(ordinateur),0);
						tab[ordinateur] = 'X';
						
						affichage(tab, sizeof(tab));

						etat = recv(descripteurSocket, &reponse, LG_MESSAGE, 0);

						if (strcmp(reponse, "continue") == 0){
							input_verificator(tab,&joueur,0);

							send(descripteurSocket, &joueur, sizeof(joueur),0);

							affichage(tab, sizeof(tab)); 
						}				

						etat = recv(descripteurSocket, &reponse, LG_MESSAGE, 0);
						//printf("%s\n", reponse);
					}
					while(strcmp(reponse, "continue") == 0);
					
					if(strcmp(reponse, "Xwins") == 0)  {
						printf("Vous avez perdu la partie.\n");
					}
					else if((strcmp(reponse, "Xends") == 0) | (strcmp(reponse, "Oends") == 0))  {
						printf("Égalité ! Personne n'a gagné la partie, tout le monde a perdu.\n");
					}
					else if(strcmp(reponse, "Owins") == 0)  {
						printf("Vous avez gagné la partie !\n");
					}
					// Plus que 2 joueurs, donc cette partie du code gère les spectateurs.
				} else {


					do  {
						recv(descripteurSocket, tab, sizeof(tab), 0);

						printf("Tableau actuel:\n");

						affichage(tab, sizeof(tab));

						recv(descripteurSocket, &reponse, LG_MESSAGE, 0);

						//printf("%s\n", reponse);
					} while(strcmp(reponse, "continue") == 0);

					if(strcmp(reponse, "Xwins") == 0)  {
						printf("Vous avez perdu la partie.\n");
					}
					else if((strcmp(reponse, "Xends") == 0) | (strcmp(reponse, "Oends") == 0))  {
						printf("Égalité ! Personne n'a gagné la partie, tout le monde a perdu.\n");
					}
					else if(strcmp(reponse, "Owins") == 0)  {
						printf("Vous avez gagné la partie !\n");
					}

				}
			break;
	}

	// On ferme la ressource avant de quitter
	close(descripteurSocket);
	return 0;
}


/*
switch(nb = send(descripteurSocket, &joueur, sizeof(joueur),0)){
	case -1 : // une erreur ! 
			perror("Erreur en écriture...");
			close(descripteurSocket);
			exit(-3);
	case 0 : // le socket est fermée 
		fprintf(stderr, "Le socket a été fermée par le serveur !\n\n");
		return 0;
	default: // envoi de n octets  
		printf("Envoyé! \n\n");
}
*/