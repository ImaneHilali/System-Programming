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

int create_or_get_message_queue(key_t key) {
int msqid;

// Create or get a message queue
msqid = msgget(key, IPC_CREAT | 0666);
if (msqid < 0) {
perror("msgget");
exit(1);
}
printf("Created or got message queue with id %d\n", msqid);

return msqid;
}


void send_message(int msqid, char* message) {
message_buf sbuf;
size_t buflen;

// Send a message to the message queue
sbuf.mtype = 1;
strncpy(sbuf.mtext, message, MSGSZ);

buflen = strlen(sbuf.mtext) + 1;

if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0) {
perror("msgsnd");
exit(1);
}
printf("Sent message: %s\n", message);
}

void receive_message(int msqid) {
message_buf rbuf;

// Receive a message from the message queue
if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
perror("msgrcv");
exit(1);
}
printf("Received message: %s\n", rbuf.mtext);
}
void destroy_message_queue(int msqid) {
// Remove the message queue
if (msgctl(msqid, IPC_RMID, NULL) < 0) {
perror("msgctl");
exit(1);
}
printf("Destroyed message queue with id %d\n", msqid);
}
int main()
{
int msqid;
key_t key;
pid_t pid;
char message[MSGSZ];

// Generate a unique key for the message queue
key = ftok(".", 'm');
// Create or get a message queue
msqid = create_or_get_message_queue(key);
// Fork a child process
pid = fork();
if (pid < 0) {
perror("fork");
exit(1);
}
else if (pid == 0) {
// Child process receives the message from the parent process
receive_message(msqid);
// Destroy the message queue
destroy_message_queue(msqid);
}
else {
// Parent process sends a message to the child process
printf("Enter a message to send to the child process: ");
fgets(message, MSGSZ, stdin);
send_message(msqid, message);
} 
return 0;
}

