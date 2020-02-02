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

void executeCommand(char **args, int flag) {
    if(fork() == 0) {
        if(execvp(args[0], args) == -1) {
            perror("Error occured");
        }
        exit(EXIT_FAILURE);
    }
    else {
        if(flag == 0)
            wait(0);
    }
}

void opredExec(char **args, int p) {
    char **temp;
    int fd, i;
    int stdout_copy;
    
    stdout_copy = dup(1);
    close(1);
    fd = open(args[p], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("Open failed ");
    }

    temp = (char**)malloc(sizeof(char*) * p + 1);
    for(i = 0; i < p; i++) {
        temp[i] = (char*)malloc(sizeof(char) * (strlen(args[i] + 1)));
        strcpy(temp[i], args[i]);
    }
    temp[i] = NULL;
    
    executeCommand(temp, 0);

    close(fd);
    dup2(stdout_copy, 1);
    close(stdout_copy);
    
}

void ipredExec(char **args, int p) {

    char **temp;
    int fd, i;
    int stdin_copy;
    
    stdin_copy = dup(0);
    close(0);
    fd = open(args[p], O_RDONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("Open failed ");
    }

    temp = (char**)malloc(sizeof(char*) * p + 1);
    for(i = 0; i < p; i++) {
        temp[i] = (char*)malloc(sizeof(char) * (strlen(args[i] + 1)));
        strcpy(temp[i], args[i]);
    }
    temp[i] = NULL;
    
    executeCommand(temp, 0);

    close(fd);
    dup2(stdin_copy, 0);
    close(stdin_copy);
    
}

void ipOpRed(char **args, int *pos) {
    int i;
    int p1, p2;
    int fd1, fd2;
    int stdin_copy, stdout_copy;
    char **temp;

    p1 = pos[0];
    p2 = pos[1];

    temp = (char**)malloc(sizeof(char*) * p1 + 2);
    for(i = 0; i < (p1 + 1); i++) {
        temp[i] = args[i];
    }
    temp[i] = NULL;

    stdin_copy = dup(0);
    close(0);
    fd1 = open(args[p1 + 1], O_RDONLY, S_IRUSR | S_IWUSR); //input file
    if(fd1 == -1) {
        perror("Open failed ");
    }

    stdout_copy = dup(1);
    close(1);
    fd2 = open(args[p2 + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); //output file 
    if(fd2 == -1) {
        perror("Open failed ");
    }

    executeCommand(temp, 0);

    close(fd1);
    dup2(stdin_copy, 0);
    close(stdin_copy);

    close(fd2);
    dup2(stdout_copy, 1);
    close(stdout_copy);   
}

void pipeExec(char **args, int total, int *pos, int n) {

    int z, pfd[2];
    int p = pos[0];

    char **temp1 = malloc(sizeof(char*) * (p + 2));
	char **temp2 = malloc(sizeof(char*) * (total - (p + 2)));

    for(z = 0; z < (p + 1); z++) {
        temp1[z] = args[z];
    }
    temp1[z] = NULL;

    for(z = 0; z < (total - (p + 1)); z++) {
        temp2[z] = args[p + z + 1];
    }
    temp2[z] = NULL;

    if(fork() == 0) {
        pipe(pfd);
        if(fork() == 0) {
            
            close(1);
            dup(pfd[1]);
            close(pfd[0]);

            if(execvp(temp1[0], temp1) == -1) {
                perror("Error occured");
            }
            exit(EXIT_FAILURE);
        }
        else {

            close(0);
            dup(pfd[0]);
            close(pfd[1]);

            if(execvp(temp2[0], temp2) == -1) {
                perror("Error occured");
            }
            exit(EXIT_FAILURE);

        }
    }
    else {
        wait(0);
    }


}

char** nextCommand(char **args, int total, int *pos, int p_no, int num) {
    //num = 0 -> 0th command ie 0 to pos[0]
    //num = 1 -> 1st command ie pos[0] to pos[1]
    char **temp;

    int l, i;
    if(num == 0) {
        l = pos[0] + 2;
    }
    else {
        if(num >= p_no)
            l = total - pos[num - 1];
        else
            l = pos[num] - pos[num - 1] + 1;
    }

    temp = (char**)malloc(sizeof(char*) * l);
    for(i = 0; i < (l-1); i++) {
        if(num == 0)
            temp[i] = args[i];
        else 
            temp[i] = args[i + pos[num - 1] + 1];

    }
    temp[i] = NULL;


    return temp;
    
}

void multiPipeExec(char **args, int total, int *pos, int n) {
    int noOfCmd = n + 1;
    char **cmd;
    int stdin_copy = 0;
    int c;

    int pfd[2];

    for(c = 0; c < noOfCmd; c++) {

        cmd = nextCommand(args, total, pos, n, c);

        pipe(pfd);

		if (fork() == 0) {

			dup2(stdin_copy, 0);
			if (c != noOfCmd - 1) { 
                //not the last command
				dup2(pfd[1], 1);
			}

			close(pfd[0]);
			if(execvp(cmd[0], cmd) == -1) {
                perror("Error occured ");
            }
			exit(EXIT_FAILURE);
		}
		else {
			wait(NULL); 
			close(pfd[1]);
			stdin_copy = pfd[0];
		}
    }
    

}

void redExec(char **args, int *pos, char *ele, int n) {
    int i, p;
    char ch;

    for(i = 0; i < n; i++) {
        p = pos[i];
        ch = ele[i];
        
        switch (ch) {
            case '<': 
                ipredExec(args, p + 1);
                break;
            
            case '>':
                opredExec(args, p + 1);
                break;
            
            default:
                break;
        }
    }
}

int cd(char *path) {
    return chdir(path);
}

void color() {
	printf("\033[1;36m");
}

void reset() {
	printf("\033[0m");
}

void printPrompt() {
    color();
    printf("prompt> ");
    reset();
}

int i, j, no, flag;

void ctrlc_handler() {
    printf("\n");
    i = 0;
    j = 0;
    no = 0;
    flag = 0;
}

void handleCtrllC() {
    struct sigaction new;
    new.sa_handler = ctrlc_handler;
    new.sa_flags = 0;
    sigemptyset(&new.sa_mask);
    sigaction(SIGINT, &new, NULL);
}

int main() {
    char ch, prevch;
    char *args[64] = { NULL };
    char temp[128];

    i = 0, j = 0, no = 0, flag = 0;
    int opred = 0, ipred = 0, pipered = 0;
    int pos[64] = { 0 }, z = 0;
    char ele[64];
    int backflag = 0;
    int k;

    printf("---- Welcome to Shell ------\n");
    printf("Working options : \n");
    printf("1. Read commands in a loop\n");
    printf("2. Show prompt\n");
    printf("3. Run commands normal and in background using '&'\n");
    printf("4. Input and output redirection individual\n");
    printf("5. Single combination of input and output redirection Ex : head -5 < prog.c > ans.c\n");
    printf("6. Single & Multiple Pipe Handling\n");
    printf("7. Control C signal handling\n");
    printf("8. Built in commands : cd and exit\n\n");


    while(1) {

        handleCtrllC();

        if(flag == 0) {
            printPrompt();
            flag = 1;
            for(k = 0; k < j; k++) {
                free(args[k]);
                args[k] = NULL;
            }
            j = 0;
            i = 0;
            no = 0;
            backflag = 0;
        }

        prevch = ch;
        ch = getchar();
        
        switch (ch) {
            case ' ':
                if(i != 0) {
                    temp[i] = '\0';
                    args[j] = (char*)malloc(sizeof(char) * (no + 1));
                    strcpy(args[j], temp);
                    i = 0;
                    no = 0;
                    j++;
                }
                break;

            case '<':
                ipred = 1;

                if(prevch != ' ') {
                    temp[i] = '\0';
                    args[j] = (char*)malloc(sizeof(char)* (no + 1));
                    strcpy(args[j], temp);
                    j++;
                    i = 0; 
                    no = 0;
                }
                
                pos[z] = j - 1;
                ele[z] = '<';
                z++;

                break;
                

            case '>':
                opred = 1;

                if(prevch != ' ') {
                    temp[i] = '\0';
                    args[j] = (char*)malloc(sizeof(char)* (no + 1));
                    strcpy(args[j], temp);
                    j++;
                    i = 0; 
                    no = 0;
                }

                pos[z] = j - 1;
                ele[z] = '>';
                z++;

                break;

            case '|':
                pipered = 1;

                if(prevch != ' ') {
                    temp[i] = '\0';
                    args[j] = (char*)malloc(sizeof(char)* (no + 1));
                    strcpy(args[j], temp);
                    j++;
                    i = 0; 
                    no = 0;
                }

                pos[z] = j - 1;
                ele[z] = '|';
                z++;

                break;
            
            case '&':
                backflag = 1;
                break;

            case '\n': 
                if(prevch != ' ' && prevch != '&') {
                    temp[i] = '\0';
                    args[j] = (char*)malloc(sizeof(char)* (no + 1));
                    strcpy(args[j], temp);
                    j++;
                }

                /*Built in commands*/
                if(strcmp(args[0], "exit") == 0) 
                    return 0;
                
                if(strcmp("cd", args[0]) == 0) {
                    if (cd(args[1]) < 0) {
                        perror(args[1]);
                    }
                    flag = 0;
                    continue;
                }

                /*Redirection*/
                if(ipred == 1 && opred == 1) {
                    ipOpRed(args, pos);
                    ipred = 0;
                    opred = 0;
                    z = 0;
                }

                else if(ipred == 1 || opred == 1) {
                    redExec(args, pos, ele, z);
                    ipred = 0;
                    opred = 0;
                    z = 0;
                }

                else if(pipered == 1 && z > 1) {
                    //multiple pipes 
                    multiPipeExec(args, j, pos, z);
                    pipered = 0;
                    z = 0;

                }

                else if(pipered == 1) {
                    pipeExec(args, j, pos, z);
                    pipered = 0;
                    z = 0;
                }

                else 
                    executeCommand(args, backflag);	
                
                flag = 0;
                backflag = 0;
                break;
            
            default:
                temp[i] = ch;
                no++;
                i++;
                break;
        }
    }
    return 0;
}