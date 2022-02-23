/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de
données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>


int sock_source_udp(int port, char *dest, int nb_message);
int sock_puit_udp(int port, int nb_message);
int sock_puit_tcp(int port, int nb_message);
int sock_source_tcp(int port, char *dest, int nb_message);

int sock, sock_bis ;
struct sockaddr_in adr_local, adr_distant, adr_client ; 
int lg_adr_distant = sizeof(adr_distant) ;
int lg_adr_client = sizeof(adr_client) ;
struct hostent *hp ;
char message[30] ;  
char motif = 'a' ;
char num ;
int lg_emis ;
int lg_rec ;
int connexion ;
int lg = 30 ; //longueur du message

void construire_message(char *message, char motif, int lg){
    int i ;
    for (i=0; i<lg; i++) message[i] = motif ; 
    } 

void afficher_message(char *message, int lg) {
    int i;
    printf("message construit : ");
    for (i=0;i<lg;i++) printf("%c", message[i]); printf("\n");
    }

void main (int argc, char **argv){
    int c;
    extern char *optarg;
    extern int optind;
    int nb_message = -1; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
    int source = -1 ; /* 0=puits, 1=source */

    int protocole = 1 ; //TCP par défaut 
    int port = atoi(argv[argc-1]) ;
    port = htons(port) ; 
    char * dest = argv[argc-2];   
    
    while ((c = getopt(argc, argv, "pn:sul")) != -1) { 
        switch (c) {
        case 'p':
            if (source == 1) {
                printf("usage: cmd [-p|-s][-n ##]\n");
                exit(1);
            }
            source = 0;
            break;

        case 's':
            if (source == 0) {
                printf("usage: cmd [-p|-s][-n ##]\n");
                exit(1) ;
            }
            source = 1;
            break;

        case 'n':
            nb_message = atoi(optarg);
            break;
        
        case 'u':
            protocole = 0; //UDP
            break;
        
        case 'l':
            lg = atoi(optarg) ;
            break ;
        
        default:
            printf("usage: cmd [-p|-s][-n ##]\n");
            break;
        }
    }

    if (source == -1) {
        printf("usage: cmd [-p|-s][-n ##]\n");
        exit(1) ;
    }

    if (nb_message != -1) {
        if (source == 1)
            printf("nb de tampons à envoyer : %d\n", nb_message);
        else
            printf("nb de tampons à recevoir : %d\n", nb_message);
    } else {
        if (source == 1) {
            nb_message = 10 ;
            printf("nb de tampons à envoyer = 10 par défaut\n");
        } else
        printf("nb de tampons à recevoir = infini\n");

    }

    if (source == 1){
        printf("on est dans le source\n");
        
        if (protocole == 1){
            printf("on est dans TCP\n");
            sock_source_tcp(port, dest, nb_message);}

        else{
            printf("on est dans UDP\n");
                sock_source_udp(port, dest, nb_message) ;
        }
    }   
    else{
        printf("on est dans le puit\n");
        
        if (protocole == 1){
            printf("on est dans TCP\n");
            sock_puit_tcp(port, nb_message);}
        else{
            printf("on est dans UDP\n");
            sock_puit_udp(port, nb_message);
        }
    }   
}

int sock_puit_udp(int port, int nb_message){
    //on crée le socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("échec de création de socket\n") ;
        exit(1) ;
    }

    //on construit l'adresse du socket
    memset((char*)&adr_local,0,sizeof(adr_local)) ; 
    adr_local.sin_family = AF_INET ;
    adr_local.sin_port = port ;
    adr_local.sin_addr.s_addr = INADDR_ANY ;

    //on associe l'adresse au socket
    if (bind(sock, (struct sockaddr*)&adr_local,sizeof(adr_local)) == -1){
        perror("échec du bind") ;
        exit(1) ;
    }

    //on reçoit un message
    if (nb_message == -1){
        while(1){
            if (recvfrom(sock, message, lg, 0, (struct sockaddr*)&(adr_distant), &lg_adr_distant) == -1){
               perror("erreur réception") ;
                exit(1) ;
            }
        printf("PUIT : Reception n°%d (%d) [%s]\n",nb_message, lg, message) ;
        }
    }
    else{
        int i = nb_message ;
        while (i>0){
            if (recvfrom(sock, message, lg, 0, (struct sockaddr*)&(adr_distant), &lg_adr_distant) == -1){
               perror("erreur réception") ;
                exit(1) ;
            }
            printf("PUIT : Reception n°%d (%d) [%s]\n",nb_message, lg, message) ;
            i -= 1 ;
        }
    }  

    //on détruit le socket 
    if (close(sock) == -1){
        perror("erreur du close\n") ;
        exit(1) ;
    } 

}

int sock_source_udp(int port, char * dest, int nb_message){
    //on crée le socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    perror("échec de création de socket\n") ;
    exit(1) ;
    }

    //on construit l'adresse du socket auquel on souhaite s'addresser 
    memset((char*)&adr_distant, 0, sizeof(adr_distant)) ;
    adr_distant.sin_family = AF_INET ;
    adr_distant.sin_port = port ;  
    if((hp=gethostbyname(dest)) == NULL){ 
        perror("erreur gethostbyname\n") ;
        exit(1) ;
    }
    memcpy((char*)&(adr_distant.sin_addr.s_addr), hp -> h_addr, hp -> h_length);

    //on envoie n messages et on affiche ce qui est demandé
    int nb_envoi = 0 ;
    while(nb_envoi<nb_message){
        construire_message(message, motif, lg) ;
        if (sendto(sock, message, lg, 0, (struct sockaddr*)&adr_distant, sizeof(adr_distant)) == -1){
            perror("erreur envoi\n") ;
            exit(1) ;
        }
        nb_envoi++ ;
        printf("SOURCE : Envoi n°%d (%d) [%s]\n",nb_envoi, lg, message) ;
    } 

    //on détruit le socket 
    if (close(sock) == -1){
        perror("erreur du close\n") ;
        exit(1) ;
    } 
}

int sock_source_tcp(int port, char * dest, int nb_message){
    //on crée le socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("échec de création de socket\n") ;
        exit(1) ;
    }

    //on construit l'adresse du socket auquel on souhaite s'addresser 
    memset((char*)&adr_distant, 0, sizeof(adr_distant)) ;
    adr_distant.sin_family = AF_INET ;
    adr_distant.sin_port = port ;  
    if((hp=gethostbyname(dest)) == NULL){ 
        perror("erreur du gethostbyname\n") ;
        exit(1) ;
    }
    memcpy((char*)&(adr_distant.sin_addr.s_addr), hp -> h_addr, hp -> h_length);

    //on effectue une demande de connexion 
    if ((connexion = connect(sock, (struct sockaddr*)&adr_distant, sizeof(adr_distant))) == -1){
        perror("erreur du connect\n") ;
        exit(1) ;
    } 

    //on envoie n messages et on affiche ce qui est demandé
    int nb_envoi = 0 ;
    while(nb_envoi<nb_message){
        construire_message(message, motif, lg) ;
        if (write(sock, message, lg) == -1){
            perror("erreur envoi\n") ;
            exit(1) ;
        } 
        nb_envoi++ ;
        printf("SOURCE : Envoi n°%d (%d) [%s]\n",nb_envoi, lg, message) ;
    }    

    //on détruit le socket 
    if (close(sock) == -1){
        perror("erreur du close\n") ;
        exit(1) ;
    } 
}

int sock_puit_tcp(int port, int nb_message){
    //on crée le socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("échec de création de socket\n") ;
        exit(1) ;
    }

    //on construit l'adresse du socket
    memset((char*)&adr_local,0,sizeof(adr_local)) ; //reset
    adr_local.sin_family = AF_INET ;
    adr_local.sin_port = port ;
    adr_local.sin_addr.s_addr = INADDR_ANY ;

    //on associe l'adresse au socket
    if (bind(sock, (struct sockaddr*)&adr_local,sizeof(adr_local)) == -1){
        perror("échec du bind") ;
        exit(1) ;
    }

    //on dimensionne la file des demandes de connexion 
    listen(sock, 5) ;

    //on se met en état d'acceptation d'une demande de connexion  
    if ((sock_bis = accept(sock, (struct sockaddr*)&adr_client, &lg_adr_client)) == -1){
        perror("échec du accept\n") ;
        exit(1) ;
    }  

    //on traite la requête 
    if (nb_message == -1){
        while(1){
            if((lg_rec = read(sock_bis, message, lg)) < 0){
                perror("échec du read\n") ;
                exit(1) ;
            } 
        printf("PUIT : Reception n°%d (%d) [%s]\n",nb_message, lg, message) ;
        } 
    }
    else{
        int i = nb_message ;
        while (i>0){
            if((lg_rec = read(sock_bis, message, lg)) < 0){
                perror("échec du read\n") ;
                exit(1) ;
            }   
        printf("PUIT : Reception n°%d (%d) [%s]\n",nb_message, lg, message) ;
        i -= 1 ; 
        }    
    }   

    //on détruit le socket 
    if (close(sock) == -1){
        perror("erreur du close\n") ;
        exit(1) ;
    } 
}