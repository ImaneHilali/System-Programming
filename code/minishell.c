#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>  
#include <sys/wait.h> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include"memoire_partagee.h"  
#define SHM_KEY 12345 

//ls
void list_files() {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(".");
    if (dir == NULL) {
        perror("Unable to open directory.");
        return;
    }
    printf("Files in the current directory:\n");
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    closedir(dir);
}
//pwd
void show_current_directory() {
    char current_directory[PATH_MAX];
    if (getcwd(current_directory, sizeof(current_directory)) != NULL) {
        printf("Current directory: %s\n", current_directory);
    } else {
        perror("getcwd() error");
    }
}
//ping
void network_ping(const char *ip_address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024] = "ping";
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket() error");
        return;
    }
    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        close(sockfd);
        return;
    }
    // Send "ping" message to the server
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto() error");
        close(sockfd);
        return;
    }
    printf("Ping sent to %s:%d\n", ip_address, port);
    // Close the socket
    close(sockfd);
}
//cat
void display_file(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char c;
    while ((c = fgetc(file)) != EOF) {
        printf("%c", c);
    }
    fclose(file);
}
//cp
void copy_file(char *source, char *destination) {
    int input_file = open(source, O_RDONLY);
    if (input_file == -1) {
        perror("Error opening source file");
        return;
    }
    int output_file = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_file == -1) {
        perror("Error opening destination file");
        close(input_file);
        return;
    }
    char buffer[4096];
    ssize_t read_size;
    while ((read_size = read(input_file, buffer, sizeof(buffer))) > 0) {
        if (write(output_file, buffer, read_size) != read_size) {
            perror("Error writing to destination file");
            close(input_file);
            close(output_file);
            return;
        }
    }
    close(input_file);
    close(output_file);
    printf("File %s copied to %s\n", source, destination);
}
//mv
void move_file(const char *source, const char *destination) {
    if (rename(source, destination) < 0) {
        perror("Unable to move file");
    }
} 
//mkdir
void make_directory(char *directory_name) {
    int status = mkdir(directory_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status != 0) {
        perror("mkdir error");
        exit(EXIT_FAILURE);
    } else {
        printf("Directory '%s' created successfully.\n", directory_name);
    }
}
//touch
void create_file(char *filename) {
    FILE *fp = fopen(filename, "a");
    if (fp != NULL) {
        fclose(fp);
        printf("File %s has been updated/created.\n", filename);
    } else {
        printf("Error creating/updating file %s.\n", filename);
    }
}
//kill
void execute_kill(char *input) {
    char command[10];
    int pid, signal_num;
    if (sscanf(input, "%s %d %d", command, &pid, &signal_num) == 3) {
        if (strcmp(command, "kill") == 0) {
            if (kill(pid, signal_num) == -1) {
                printf("Failed to send signal to process %d.\n", pid);
            } else {
                printf("Signal %d sent to process %d.\n", signal_num, pid);
            }
        } else {
            printf("Invalid command. Available command: kill <PID> <SIGNAL_NUMBER>\n");
        }
    } else {
        printf("Invalid arguments. Usage: kill <PID> <SIGNAL_NUMBER>\n");
    }
}
//echo
void echo(char *message) {
    printf("%s\n", message);
}
// Fonction pour gérer le signal SIGINT
void handle_signal(int signum) {
if (signum == SIGINT) {
printf("\nInterruption de la commande en cours\n");
// Réattacher la fonction handle_signal au signal SIGINT
signal(SIGINT, handle_signal);
}
else if (signum == SIGQUIT) {
printf("\nFin du shell et création d'un fichier core\n");
// Créer un fichier core
abort();
}
}
//rm
void remove_file(char* filename) {
    if (remove(filename) == 0) {
        printf("File '%s' deleted successfully.\n", filename);
    } else {
        perror("Unable to delete file.");
    }
}
// implementation de |
void pipe_cmds(char* cmd1, char* cmd2, char* output_file)  {
    // Create a pipe
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
        // Fork a child process to execute the first command
    pid_t pid1 = fork();
    if (pid1 ==0){
	// Child process 1: redirect stdout to the write end of the pipe
	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
	execlp(cmd1, cmd1, NULL);
	perror("execlp");
	exit(EXIT_FAILURE);
    } else if (pid1 > 0) {
	// Parent process: fork a child process to execute the second command
	pid_t pid2 = fork();
	if (pid2 == 0) {
	// Child process 2: redirect stdin to the read end of the pipe
	close(fd[1]);
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);

	//redirect stdout to the output file
	int output_fd =open(output_file,O_WRONLY|O_CREAT|O_TRUNC,0644);
 	if (output_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    	}
	if (dup2(output_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
    	}
   
	close(output_fd);
	execlp(cmd2, cmd2, NULL);
	perror("execlp");
	exit(EXIT_FAILURE);
	} else if (pid2 > 0) {
	// Parent process: wait for both child processes to finish
	close(fd[0]);
	close(fd[1]);
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
	} else {
	perror("fork");
	exit(EXIT_FAILURE);
 	}
   } else {
	perror("fork");
	exit(EXIT_FAILURE);
	}
	}

/*
void red_cmds(char* cmd1, char* output_file){

// Create a named pipe (FIFO)
mkfifo("myfifo", 0666);
// Fork a child process to execute the command
pid_t pid = fork();
if (pid == 0) {
// Child process: redirect stdout to the named pipe
int fd = open("myfifo", O_WRONLY);
dup2(fd, STDOUT_FILENO);
close(fd);
execlp(cmd1, cmd1, NULL);
exit(EXIT_FAILURE);
} else if (pid > 0) {
// Parent process: wait for the child to finish
wait(NULL);
// Read the output from the named pipe and write it to the output file
int fd = open("myfifo", O_RDONLY);
int out_fd =open(output_file,O_WRONLY|O_CREAT|O_TRUNC,0644); 
 if (out_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
char buf[BUFSIZ];
ssize_t nread;
while ((nread = read(fd, buf, BUFSIZ)) > 0) {

write(out_fd, buf, nread);
} 
close(fd);
close(out_fd);
// Remove the named pipe
unlink("myfifo");
} else {
perror("fork");
exit(EXIT_FAILURE);
} 
}
*/

#define SHM_SIZE 1024

void saisie() {
	key_t key = 1234;
	int shmid;
	char *shmaddr;

	shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
	if (shmid == -1) {
	perror("Erreur lors de la création de la mémoire partagée");
	exit(1);
	}
	shmaddr = shmat(shmid, NULL, 0);
	if (shmaddr == (char *) -1) {
	perror("Erreur lors de l'attachement de la mémoire partagée");
	exit(1);
	}
	printf("Entrez une chaîne de caractères : ");
	fgets(shmaddr, SHM_SIZE, stdin);
	shmaddr[strlen(shmaddr) - 1] = '\0';

	printf("La chaîne de caractères saisie est : %s\n", shmaddr);

	if (shmdt(shmaddr) == -1) {
	perror("Erreur lors du détachement de la mémoire partagée");
	exit(1);
	}
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
	perror("Erreur lors de la suppression de la mémoire partagée");
	exit(1);
	}
	}

int main() {
    char input[100];
    
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    
    char cmd1[50];
    char cmd2[50];
    char output_file[50];
    
    int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
    perror("shmget");
    exit(1);
    }
    while (1) {
    
        printf("hilali$ ");
        
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
        
        
        if (strcmp(input, "lister") == 0) {
            list_files();
        } else if (strcmp(input, "pwd") == 0) {
            show_current_directory();
        } else if (strncmp(input, "cat", 3) == 0) {
            char filename[100];
            if (sscanf(input + 4, "%99s", filename) == 1) {
                display_file(filename);
            } else {
                printf("Invalid arguments. Usage: cat <FILE>\n");
            }
        } else if (strncmp(input, "supprimer", 9) == 0) {
            char filename[100];

            if (sscanf(input + 9, "%99s", filename) == 1) {
                remove_file(filename);
            } else {
                printf("Invalid arguments. Usage: rm <filename>\n");
            }
        } else if (strncmp(input, "copier", 6) == 0) {
            char *source_file = strtok(input + 3, " ");
            char *destination_file = strtok(NULL, " ");
            copy_file(source_file, destination_file);
        } else if (strncmp(input, "move", 4) == 0) {
            char *source_file = strtok(input + 3, " ");
            char *destination_file = strtok(NULL, " ");
            move_file(source_file, destination_file);
        } else if (strncmp(input, "mkdir", 5) == 0) {
            char *dirname = input + 6; // Skip "mkdir " at the beginning
            make_directory(dirname);
        } else if (strncmp(input, "touch", 5) == 0) {
            char *filename = input + 6; // Skip "touch " at the beginning
            create_file(filename);
        }  else if (strncmp(input, "kill", 4) == 0) {
        int pid, signal_num;
        if (sscanf(input + 5, "%d %d", &pid, &signal_num) == 2) {
        if (kill(pid, signal_num) == -1) {
            printf("Failed to send signal to process %d.\n", pid);
        } else {
            printf("Signal %d sent to process %d.\n", signal_num, pid);
        }
        } else {
        printf("Invalid arguments. Usage: kill <PID> <SIGNAL_NUMBER>\n");
        }   
        } else if (strcmp(input, "clear") == 0) {
         return system("./memoire_partagee"); 
    	// Attacher la zone de mémoire partagée
    	int *clear_requested = shmat(shmid, NULL, 0);
    	if (clear_requested == (void *) -1) {
        perror("shmat");
        exit(1);
    	}
	// Initialiser la valeur à 1
	*clear_requested = 1;

	// Attendre 1 seconde pour laisser le processus enfant effacer l'affichage du terminal
	sleep(1); 
	} else if (strncmp(input, "saisir", 6) == 0) {
	saisie();
	} else if (strcmp(input, "filemsg1") == 0) {
         return system("./filemsg1");
	} else if (strcmp(input, "filemsg2") == 0) {
         return system("./filemsg2");
	} else if (strcmp(input, "calculer") == 0) {
	int userinput;
	printf("entrer le nombre de taches (2,3 ou 4) : ");
	scanf("%d",&userinput);
	
	if(userinput == 2){
         return system("./2taches");
         } else if (userinput == 3){
         return system("./3taches");
         }else if (userinput == 4){
         return system("./4taches");
         } else {
         printf ("Invalid input \n");}
	} else if (strncmp(input, "echo", 4) == 0) {
            char message[100];
            if (sscanf(input + 4, "%[^\n]", message) == 1) {
                echo(message);
            } else {
                printf("Invalid arguments. Usage: echo <MESSAGE>\n");
            }
        } else if (sscanf(input, "%49s | %49s %49s", cmd1, cmd2, output_file) == 3) {
	pipe_cmds(cmd1,cmd2,output_file);
 	}
       else if (sscanf(input, "%49s > %49s", cmd1, output_file) == 2) {
	red_cmds(cmd1,output_file);

        } 
        else if (strncmp(input, "network_ping", 12) == 0) {
            char ip_address[16];
            int port;
            if (sscanf(input + 12, "%15s %d", ip_address, &port) == 2) {
                network_ping(ip_address, port);
            } else {
                printf("Invalid arguments. Usage: network_ping <IP> <PORT>\n");
            }
        } else if (strcmp(input, "exit") == 0) {
            printf("Exiting the mini shell.\n");
            break;
        } else {
            printf("Invalid command. Available commands: lister, supprimer, cat, copier, pwd, move, mkdir, touch, kill, echo, network_ping <IP> <PORT>, 0 (to exit)\n");
        } 
        
 // Lire la commande de l'utilisateur
            char cmd[256];
            fgets(cmd, 256, stdin);
            // Exécuter la commande
            system(cmd);
            
    
} 
    return 0;
}


