/*Some code is from courses exercises*/
/*CT30A3370 Käyttöjärjestelmät ja systeemiohjelmointi 2016 @ LUT
 *Toivo Mattila 0452499
 *Last modified 2017-01-08*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXNUM 40
#define MAXLEN 160
#define MAXCMD 20

void cd(char *argv[]){
  if(argv[1] == '\0'){
    chdir(getenv("HOME"));
  }else{
    char chwdto[1024];
    if(getcwd(chwdto, sizeof(chwdto)) == NULL){
      printf("cd: ERROR with getcwd()");
      return;
    }
    strcat(strcat(chwdto, "/"), argv[1]);
    chdir(chwdto);
  }
}

void help(){
	printf("\nShell created by Toivo Mattila as an exercise for the course\n");
	printf(" CT30A3370 Käyttöjärjestelmät ja systeemiohjelmointi @ LUT\n");
	printf("\t\t-----Features-----\n");
	printf("Exit by entering exit or sending EOF(CTRL+D)\n");
	printf("See info by typing help\n");
	printf("cd [path] to change directory\n");
	printf("\tIf path is not specified, goes to home directory\n");
	printf("Run single commands by typing the command\n");
	printf("Piping (|) and input/output redirection (< & >)\n");
	printf("\tPlease note that |, < and > need spaces around them\n");
	printf("Background processes (end command with &)\n\n");
}

void runcommand(int argc, char *argv[], int background){
	int pid;
	/*cmds[x] points to the start of xth command*/
	char **cmds[MAXCMD];
	/*Make cmds NULL to hopefully prevent some errors*/
	for(int i = 0; i < MAXCMD; i++){
		cmds[i] = NULL;
	}
	cmds[0] = &argv[0];
	int cmdindx = 0;
	for(int i = 1; i < argc; i++){
		if(argv[i][0] == '|'){
			cmdindx++;
			cmds[cmdindx] = &argv[i+1];
			argv[i] = NULL;
		}
	}

	int input = 0;
	int output = 1;
	int fd[2];
	for(int i = 0; cmds[i] != NULL; i++){
		argv = cmds[i];
		/* fork to run the command */
		/*Before fork, create pipes and set correct inputs and outputs*/
		pipe(fd);
		if(i == cmdindx){/*If last command, output to stdout*/
			output = 1;
		}else{
			output = fd[1];
		}
		/*Find input or output to a file*/
		for(int j = 0; argv[j] != NULL; j++){
			/*Input*/
			if(argv[j][0] == '<'){
				argv[j] = NULL;
				j++;
				input = open(argv[j],O_RDONLY);
			}
			/*Output*/
			if(argv[j][0] == '>'){
				argv[j] = NULL;
				j++;
				output = open(argv[j], O_WRONLY|O_CREAT|O_TRUNC, 0666);
			}
		}

		switch (pid = fork()) {
			case -1:
				/* error*/
				perror("fork");
				return;
			case 0:
				/* child process*/
				/*If process should be run in the background, create grandchild that continues to execute
				 *the wanted command and exit child so that parent may continue.
				 *This because the parent waits for all children. Possible solution would be not to wait for
				 *the last child if (background == 1) but that creates other problems.*/
				if(background == 1){
					if((pid = fork()) != 0){
            if(pid == -1){
              perror("fork");
            }
						exit(0);
					}
				}

				dup2(input, 0);
				dup2(output, 1);
				execvp(argv[0], argv);
				perror("execvp");
				exit(1);
			default:
				/* parent (shell)*/
				/*Handle closing pipes here*/
				/*Close write*/
				close(fd[1]);
				/*May point to file so close separately*/
				if(output != 1){
					close(output);
				}
				/*If input directs to open file descriptor, close it*/
				if(input != 0){
					close(input);
				}
				/*If last command, close read because no-one is going to read it*/
				if(i == cmdindx){
					close(fd[0]);
				}else{ /*Else make input the read end of the pipe*/
					input = fd[0];
				}
				wait(NULL);
				break;
		}
	}
}

int main(void){
	char * cmd, line[MAXLEN], * argv[MAXNUM];
  char cwd[1024];
	int background, i;

	printf("\n\tShell created by Toivo Mattila\n");
	printf("\tType \"help\" for more information\n\n");

	while (1) {
		background = 0;

		/* print the prompt and current path*/
    if(getcwd(cwd, sizeof(cwd)) != NULL){
      printf("%s> ", cwd);
    }else{
      printf("Unknown path ): ");
    }

		/* read the users command */
		if (fgets(line,MAXLEN,stdin) == NULL) {
			printf("\nlogout\n");
			exit(0);
		}
		/*Remove newline*/
		line[strlen(line) - 1] = '\0';

		if (strlen(line) == 0)
			continue;

		/* start to background */
		if (line[strlen(line)-1] == '&') {
			line[strlen(line)-1]=0;
			background = 1;
		}

		/* split the command line */
		i = 0;
		cmd = line;
		while ( (argv[i] = strtok(cmd, " ")) != NULL) {
			/*printf("arg %d: %s\n", i, argv[i]);*//*Uncomment to see the arguments*/
			i++;
			cmd = NULL;
		}

		if (strcmp(argv[0],"exit")==0) {
			exit(0);
		} else if(strcmp(argv[0], "cd") == 0){
      cd(argv);
      continue;
    }else if(strcmp(argv[0], "help") == 0){
			help();
			continue;
		}
		runcommand(i, argv, background);
	}
	return 0;
}
