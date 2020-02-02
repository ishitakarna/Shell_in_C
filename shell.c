#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void color() {
	printf("\033[1;36m");
}

void reset() {
	printf("\033[0m");
}

int cd(char *path) {
    return chdir(path);
}

int main() {
	char input[128], buf[128];
	char *ptr = input;	

	int pid;
	int i, j, k;
	int stdin_copy, stdout_copy, pin_copy, pout_copy;
	int opred = 0, ipred = 0, pipered = 0;
	int fd, fd2;
	int pfd[2];

	signal(SIGINT, SIG_IGN); //ignore control c

	while(1) {
		char *args[64] = { NULL };

		if(opred == 1) {
			opred = 0;
			close(fd);
			dup2(stdout_copy, 1);
			close(stdout_copy);
		}

		if(ipred == 1) {
			ipred = 0;
			close(fd);
			dup2(stdin_copy, 0);
			close(stdin_copy);
		}

		if(pipered == 1) {
			pipered = 0;
		}

		color();
		printf("prompt> ");
		reset();
		fgets(input, 128, stdin);
		ptr = input;
		
		for(i = 0; i < sizeof(args) && *ptr; ptr++) {
			if(*ptr == ' ')
				continue;
			if(*ptr == '\n')
				break;
			for(args[i++] = ptr; *ptr && *ptr != ' ' && *ptr != '\n'; ptr++);
			*ptr = '\0';
		}

		for(j = 0; j < i; j++) {
			if(strcmp(args[j], ">") == 0)
				opred = 1;
			if(strcmp(args[j], "<") == 0)
				ipred = 1;
			if(strcmp(args[j], "|") == 0) {
				pipered = 1;
				k = j;
			}
		}

		if(pipered == 1) {
			//inst before pipe = args[0 to k - 1]
			//inst after pipe = args[k + 1 to i - 1]
			int z;
			char **args1 = malloc(sizeof(char*) * k+1);
			char **args2 = malloc(sizeof(char*) * (i-k+1));

			for(z = 0; z < k; z++) {
				args1[z] = args[z];
			}
			args1[z] = NULL;

			for(z = 0; z < (i-k); z++) {
				args2[z] = args[k + z + 1];
			}
			args2[z] = NULL;

			int pid1, pid2;

			if(fork() == 0) {
				pipe(pfd);
				if(fork() == 0) {
					
					close(1);
					dup(pfd[1]);
					close(pfd[0]);

					if(execvp(args1[0], args1) == -1) {
						perror("Error occured");
					}
					exit(EXIT_FAILURE);
				}
				else {

					close(0);
					dup(pfd[0]);
					close(pfd[1]);

					if(execvp(args2[0], args2) == -1) {
						perror("Error occured");
					}
					exit(EXIT_FAILURE);

				}
			}
			else {
				wait(0);
			}
		}

		else {
			if(strcmp("exit", args[0]) == 0)
				return 0;

			if(strcmp("cd", args[0]) == 0) {
				if (cd(args[1]) < 0) {
					perror(args[1]);
				}
				continue;
			}	

			if(opred == 1) {
				//output redirection
				//file is args[i-1]
				stdout_copy = dup(1);
				close(1);
				fd = open(args[i-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
				if(fd == -1) {
					perror("Open failed ");
				}
				
				args[i-1] = NULL;
				args[i-2] = NULL;
				
			}

			if(ipred == 1) {
				//input redirection
				//file is args[i-1]
				stdin_copy = dup(0);
				close(0);
				fd = open(args[i-1], O_RDONLY, S_IRUSR | S_IWUSR);
				if(fd == -1) {
					perror("Open failed ");
				}
				
				args[i-1] = NULL;
				args[i-2] = NULL;
			}	

			pid = fork();

			if(pid == 0) {
				//child process
				 signal(SIGINT, SIG_DFL); //restore control c in child
				if(execvp(args[0], args) == -1) {
					perror("Error occured");
				}
				exit(EXIT_FAILURE);
			}

			else {
				//parent process
				wait(0);
			}
		}
	}
	return 0;
}

