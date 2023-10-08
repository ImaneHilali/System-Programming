#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHM_KEY 12345

int main() {
    // Créer une zone de mémoire partagée pour communiquer avec le processus enfant
    int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    
    // Forker le processus
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid == 0) {
        // Processus enfant : attendre une demande de nettoyage de la mémoire partagée
        int *clear_requested = shmat(shmid, NULL, 0);
        int *program_finished = shmat(shmid, NULL, 0);
        while (1) {
            if (*clear_requested) {
                // Effacer l'affichage du terminal
                system("clear");
                // Réinitialiser la valeur dans la mémoire partagée
                *clear_requested = 0;
                
                //signaler que le programme a termine son execution
                
            }
            sleep(1);
        }
    }
    else {
        // Processus parent : lire l'entrée de l'utilisateur et exécuter les commandes
        int *clear_requested = shmat(shmid, NULL, 0);
        while (1) {
            // Afficher le prompt
            printf("hilali$ ");
            
            // Lire la commande de l'utilisateur
            char cmd[256];
            fgets(cmd, 256, stdin);
            
            // Effacer l'affichage du terminal si la commande est "clear"
            if (strcmp(cmd, "clear\n") == 0) {
                *clear_requested = 1;
            }
            else {
                // Exécuter la commande
                system(cmd);
            }
        }
        
        // Attendre la fin du processus enfant
        wait(NULL);
        
        // Supprimer la zone de mémoire partagée
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(1);
        }
    }
    return 0;
}

