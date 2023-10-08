#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGSZ 128

// Define a structure for the message
typedef struct msgbuf {
long mtype;
char mtext[MSGSZ];
} message_buf;

int main()
{
int msqid;
key_t key;
message_buf sbuf;
size_t buflen;
pid_t pid;

// Generate a unique key for the message queue
key = ftok(".", 'm');

// Create a message queue
msqid = msgget(key, IPC_CREAT | 0666);

// Fork a child process
pid = fork();

if (pid < 0) {
perror("fork");
exit(1);
}
else if (pid == 0) {
// Child process reads commands from the message queue and executes them
while (1) {
if (msgrcv(msqid, &sbuf, MSGSZ, 1, 0) < 0) {
perror("msgrcv");
exit(1);
}
system(sbuf.mtext);
}
}
else {
// Parent process reads commands from the user and puts them in the message queue
while (1) {
printf("Enter a command: ");
fgets(sbuf.mtext, MSGSZ, stdin);

// Strip the newline character from the input
sbuf.mtext[strcspn(sbuf.mtext, "\n")] = 0;
sbuf.mtype = 1;
buflen = strlen(sbuf.mtext) + 1;
if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0) {
perror("msgsnd");
exit(1);
}
}
}
// Remove the message queue
if (msgctl(msqid, IPC_RMID, NULL) < 0) {
perror("msgctl");
exit(1);
}
return 0;
}

