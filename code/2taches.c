#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include"ipcsem.h"
#include <time.h>

int Num;
/* Fournit le numero Num du groupe de semaphores crees */
void CreerSem(key_t cle, int N) {
Num=semget(cle, N, 0600 | IPC_CREAT);
if(Num==-1)
{perror("Pb CreerSem");exit(1);}
}
/* Detruit le groupe de semaphores Num */
void DetruireSem() {
semctl(Num, 0, IPC_RMID, 0);
}
/* Initialier le semaphore N du groupe Num a la valeur V */
void InitSem(int N, int V) {
semctl(Num, N, SETVAL, V);
}
/* Realise P sur le semaphore N du groupe Num */
void P(int N) {
struct sembuf Tabop; /* une seule operation */
Tabop.sem_num=N;
Tabop.sem_op=-1;
Tabop.sem_flg=0; /* bloquant */
semop(Num, &Tabop,1);
}
/* Realise V sur le semaphore N du groupe Num */
void V(int N) {
struct sembuf Tabop; /* une seule operation */
Tabop.sem_num=N;
Tabop.sem_op=1;
Tabop.sem_flg=0;
semop(Num, &Tabop, 1); }

int perform_operation(int a, int b, char op) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            if (b == 0) {
                printf("Error: Division by zero\n");
                exit(1);
            }
            return a / b;
        default:
            printf("Invalid operator: %c\n", op);
            exit(1);
    }
}

int operator_priority(char op) {
    if (op == '*' || op == '/') {
        return 1;
    }
    return 0;
}

void child_process( int *shared_mem, int start_idx) {
   
    // Read the operands and the operator from the shared memory
    int a = shared_mem[start_idx];
    char op = (char)shared_mem[start_idx + 1];
    int b = shared_mem[start_idx + 2];

    // Perform the operation
    int result = perform_operation(a, b, op);

    // Write the result to the shared memory
    shared_mem[start_idx] = result;

    V(0);

    exit(0);
}

int main() {
	
    CreerSem(1000, 1);
    InitSem(0, 0); 

    int a[2], b[2];
    char op[2], inter_op[1];

    printf("Enter the first expression : ");
    scanf("%d %c %d", &a[0], &op[0], &b[0]);

    printf("Enter the operator between the first and second expressions (+,-,*,/): ");
    scanf(" %c", &inter_op[0]);

    printf("Enter the second expression : ");
    scanf("%d %c %d", &a[1], &op[1], &b[1]);

int shm_id = shmget(IPC_PRIVATE, 6 * sizeof(int), IPC_CREAT | 0666);

if (shm_id == -1 ) {
    perror("shmget or semget");
    exit(1);
}

int *shared_mem = (int *)shmat(shm_id, NULL, 0);

for (int i = 0; i < 2; i++) {
    shared_mem[3 * i] = a[i];
    shared_mem[3 * i + 1] = (int)op[i];
    shared_mem[3 * i + 2] = b[i];
}

// Create child processes
for (int i = 0; i < 2; i++) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        child_process(shared_mem, 3 * i);
    }
}
    clock_t start_time = clock();


    P(0);
    P(0);
 
    int final_result = perform_operation(shared_mem[0], shared_mem[3], inter_op[0]);

    // Print the result in the parent process
    printf("Resultat : %d\n", final_result);


    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("temps d'execution pour le calcul multiprocessus : %.7f seconds\n", elapsed_time);


    clock_t start_time1 = clock();

    int seqresult1 = perform_operation(a[0],b[0],op[0]);
    int seqresult2 = perform_operation(a[1],b[1],op[1]);
    int seqresult = perform_operation(seqresult1,seqresult2,inter_op[0]);

    printf("Resultat : %d\n", seqresult);



    clock_t end_time1 = clock();
    double elapsed_time1 = (double)(end_time1 - start_time1) / CLOCKS_PER_SEC;
    printf("temps d'execution pourle calucl en mode sequentiel : %.7f seconds\n", elapsed_time1);



    // Detach and remove the shared memory segment
    shmdt(shared_mem);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
