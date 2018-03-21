
/****************************************************************
 * Name        :  Divam Hiren Shah                              *
 * Class       :  CSC 415                                       *
 * Date        :  02/27/2018                                    *
 * Description :  Writting a simple bash shell program          *
 * 	          that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFERSIZE 256
#define PROMPT "myShell~"
#define PROMPTSIZE sizeof(PROMPT)
#define ARGVMAX 64
#define PIPECNTMAX 10
#define DELIM " \n\t"
#define EXIT "exit"
#define CD "cd"
#define PWD "pwd"



pid_t fork_called(char** myargv, int myargc)
{
		int flag;
		int out_check=0;
		int in_check=0;
		int background =0;
		char* file_name = (char*)malloc(sizeof(int)*BUFFERSIZE);
		pid_t pid;
		int output_fd=-1, input_fd=-1;
		int found_at=0;
		//Checking for characters- &,>, >>,<
		for (int i=0;i<myargc-1;i++)
		{
				if(strcmp(myargv[i],">")==0)
				{
					 out_check=1;
					 myargv[i]='\0';
					 found_at=i+1;
				}
				else if(strcmp(myargv[i],">>")==0)
				{
					out_check=1;
					myargv[i]='\0';
					found_at=i+1;
				}
				else if(strcmp(myargv[i],"<")==0)
				{
					in_check=1;
					myargv[i]='\0';
					found_at=i+1;
				}
				else if(strcmp(myargv[i],"&")==0)
				{
					background=1;
					myargv[i]='\0';
				}
		}

		//Fork	
		pid=fork();
		//Check for child process
		if(pid==0)
		{
			
			//Redirect output
			if(out_check==1)
			{
				
				strcpy(file_name,myargv[found_at]);
				output_fd= open(file_name, O_CREAT|O_WRONLY|O_APPEND,S_IRWXU);
				if(output_fd<0)
					perror("File cannot be opened\n");
				//Writing the output to output_fd
				int dup_fd=dup2(output_fd,STDOUT_FILENO);
				if(dup_fd<0)
					perror("Bad Dup FD\n");
				close(output_fd);
				execvp(*myargv, myargv);
				perror("Error writing\n");
				
			}
			//Redirect input
			else if(in_check==1)
			{
			 	strcpy(file_name,myargv[found_at]);
			 	input_fd=open(file_name, O_RDONLY,S_IRWXU);
			 	if(input_fd<0)
					perror("File cannot be opened for reading\n");
				//Reading the input to input_fd
			 	int dup_fd=dup2(input_fd,STDIN_FILENO);
				if(dup_fd<0)
					perror("Bad Dup FD\n");
			 	close(input_fd);
				execvp(*myargv, myargv);
			 	perror("Error reading\n");
			}
			//Normal 
 			else
			{
				execvp(*myargv, myargv);
				perror("Execution failed\n");
				return 0;
			}		
					
		}
		else if(pid>0)
		{
				if(!background)
				{		
					do
					{
					  int process_id= waitpid(pid,&flag,WUNTRACED);
					  if(process_id<0)
						perror("Error terminating process\n");
					}while(!WIFEXITED(flag)&&!WIFSIGNALED(flag));
				}	
		}
		else
		{
			perror("Fork failed\n");
		}
	return 1;
}


pid_t fork_pipe(char** myargv, int myargc,int right_pipe)
{
	
	int pipe_fd[2];
	int flag;
	int background=0;
	pid_t left_pid,right_pid;
	
	int pipe_id=pipe(pipe_fd);
	if(pipe_id<0)
		perror("Error in piping\n");
	left_pid=fork();
	

	
	//Checking for & at the end of command
	if(strcmp(myargv[myargc-2],"&")==0)
	{
			background=1;
			myargv[myargc-2]='\0';
	}
	
	//Left-Child Process
	if(left_pid==0)
	{

		  int dup_fd=dup2(pipe_fd[1], STDOUT_FILENO);
		  if(dup_fd<0)
			perror("Bad Dup FD\n");
		  close(pipe_fd[0]);
		  execvp(myargv[0],&myargv[0]);
		  perror("Error in left child\n");
	}
	//Right-Child Process
	right_pid=fork();		
	if(right_pid==0)
	{
		  int dup_fd=dup2(pipe_fd[0], STDIN_FILENO);
		  if(dup_fd<0)
			perror("Bad Dup FD\n");
		  close(pipe_fd[1]);
		  execvp(myargv[right_pipe],&myargv[right_pipe]);
		  perror("Error in right child\n");	
	}
	//Parent-waiting for left-child
	else if(right_pid>0)
	{
		if(!background)				
			do
			{
				int process_id=waitpid(left_pid,&flag,WUNTRACED);
				if(process_id<0)
					perror("Error terminating process\n");
			}while(!WIFEXITED(flag)&&!WIFSIGNALED(flag));
		  	
	}
	else
	{
		perror("Fork failed\n");
	}
	return 1;
}


int main(int argc, char** argv)
{
	char* user_input = (char*)malloc(sizeof(int)*BUFFERSIZE);
	char** myargv=(char**)malloc(sizeof(int)*BUFFERSIZE);
	pid_t pid; 
	int myargc=0;
	int update;
	int cd_check;
	int flag_check=0;
	int pipe_counter=0;
	
	do
	{
			flag_check=0;
			int pipe_check=0;
			int right_pipe=0;
			int index_cwd=0;
			char cwd[255];
			char buffer2[255];
			char*c_wd=getcwd(cwd,sizeof(cwd));
			
			memset(buffer2,'\0',255);
			int counter = 0;
			for(int i=0,j=0;j<3;i++)
			{
				counter = i;
				if(c_wd[i] == '/'){
					j++;
				}
				
			}
			for(int i = counter,j=0; i <BUFFERSIZE;i++,j++){
				buffer2[j] = c_wd[i];	
			}

			printf("%s%s>> ", PROMPT,buffer2);
			fgets(user_input,BUFFERSIZE,stdin);
			if(user_input[0]=='\n')
			{
				update=1;
			}
			else if(user_input!=NULL)
			{
				char* token_argument=strtok(user_input,DELIM);
				myargc=0;
				myargv[myargc]=token_argument;	
				myargc++;
				if(strcmp(myargv[0],EXIT)==0)
				{
					update=0;
				}
				else
				{	//Parsing takes place here	
					while(token_argument!=NULL&&myargc<ARGVMAX)
					{
						token_argument=strtok(NULL,DELIM);
						myargv[myargc]=token_argument;
						myargc++;
					}
					//terminating string with null byte
					if(myargv[myargc]!=NULL)
					{	
						myargv[myargc]='\0';
					}
					//checking for pipe
					for(int i=0;i<myargc-1;i++)
					{
						if(strcmp(myargv[i],"|")==0)
						{
						pipe_check=1;
						myargv[i]='\0';
						pipe_counter++;
						right_pipe=i+1;
						}
				
					}
					//checking for cd
					if(strcmp(myargv[0],CD)==0)
				 	{
						if(myargv[1]!=NULL)
						{
						 
					 	 cd_check=chdir(myargv[1]);
						 if(cd_check<0)
							perror("Error in CD\n");
						 getcwd(cwd,sizeof(cwd));
						 flag_check=1;
						}
					}
					//checking for pwd
					if(strcmp(myargv[0],PWD)==0)
				 	{
						getcwd(cwd,sizeof(cwd));
					}
					//Calling fork
					if(flag_check==0)
					{	
						if(pipe_check==1)
						{
							update=fork_pipe(myargv,myargc,right_pipe);
						}
						else if(pipe_check!=1)
							update=fork_called(myargv,myargc);
					}
				}
			}
		}while(update);	
	free(user_input);
	free(myargv);
	return 0;
}
