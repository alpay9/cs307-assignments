#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    printf("I'm SHELL process, with PID: %i - Main command is: man grep | grep -A 2 -e \"-A\"\n", getpid());
    int pid_grep = fork();
    if(pid_grep < 0) {
        fprintf(stderr, "fork failed\n");
		exit(1);
    } else if(pid_grep == 0) {
        int fd[2];
        pipe(fd); // man -> grep
        int fd_read = fd[0]; // grep reads
        int fd_write = fd[1]; // man writes
        int signal[2];
        pipe(signal);
        int signal_receive = signal[0]; // grep receives signal
        int signal_send = signal[1]; // man sends signal

        int pid_grep = fork();
        if(pid_grep < 0){
            fprintf(stderr, "fork failed\n");
		    exit(1);
        } else if(pid_grep == 0){ // man
            close(fd_read);
            printf("I'm MAN process, with PID: %i - My command is: man grep\n", getpid());
            write(signal_receive, "S", 1);
            close(signal_receive);
            close(signal_send);

            dup2(fd_write, STDOUT_FILENO);
            char* args[] = {"man", "grep", NULL};
            execvp("man", args);
            wait(NULL);
        } else { // grep
            close(fd_write);
            close(signal_send);
            char buf[1];
            read(signal_receive, buf, 1); // will wait till there is no write descriptor remaining
            printf("I'm GREP process, with PID: %i - My command is: grep -A 2 -e \"-A\"\n", getpid());

            int fd_output = open("./output.txt", O_WRONLY | O_TRUNC | O_CREAT, 0640);
            dup2(fd_output, STDOUT_FILENO);
            dup2(fd_read, STDIN_FILENO);
            char* args[] = {"grep", "-A", "2", " -A ", NULL};
            execvp("grep", args);
        }
    } else {
        waitpid(pid_grep, NULL, 0);
        printf("I'm SHELL process, with PID: %i - execution is completed, you can find the results in output.txt\n", getpid());
    }
    return 0;
}