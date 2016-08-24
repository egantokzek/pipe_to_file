/* Copyright (c) 2016, Egan Tokzek
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>

#include <linux/stat.h>

#define PID_FILE		"/var/run/pipe_to_file.pid"
#define BUFF_SIZE		1024

const char *argp_program_version =
  "pipe_to_file 0.9";
const char *argp_program_bug_address =
  "<egan.tokzek@gmail.com>";
  
/* Program documentation. */
static char doc[] =
  "pipe_to_file -d -p /var/run/pidfile.pid fifo_file out_file";

/* A description of the arguments we accept. */
static char args_doc[] = "PIPE_FILE OUT_FILE";

/* The options we understand. */
static struct argp_option options[] = {
  {"debug",  'd', 0,      0,  "Add debug messages" },
  {"verbose",  'v', 0,      0,  "Produce verbose output" },
  {"pidfile",   'p', "FILE", 0,
   "PID FILE instead of default value" },
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  char *args[2];                /* arg1 & arg2 */
  int debug,verbose;
  char *pid_file;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'v':
      arguments->verbose = 1;
      break;
      
     case 'd':
      arguments->debug = 1;
      break;
      
     case 'p':
      arguments->pid_file = arg;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
        /* Too many arguments. */
        argp_usage (state);

      arguments->args[state->arg_num] = arg;

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };


struct stat buf;
int reload=0;

void sig_handler(int signo)
{
    if (signo == SIGHUP){
		printf("received SIGHUP\n");
		reload=1;
	}
}

int file_exists(const char* file) {
    return (stat(file, &buf) == 0);
}

int main(int argc, char **argv)
{
		struct arguments arguments;

		/* Default values. */
		arguments.debug = 0;
		arguments.verbose = 0;
		arguments.pid_file = NULL;

		/* Parse our arguments; every option seen by parse_opt will
		be reflected in arguments. */
		argp_parse (&argp, argc, argv, 0, 0, &arguments);

		if(arguments.debug){
			printf ("ARG1 = %s\nARG2 = %s\nDEBUG = %s\nVERBOSE = %s\nPIDFILE=%s\n",
			arguments.args[0], arguments.args[1],
			arguments.debug ? "yes" : "no",arguments.verbose ? "yes" : "no",
			arguments.pid_file);
		}
		
        FILE *fp,*fout,*fpid;
        char readbuf[BUFF_SIZE];
        char pid_file[BUFF_SIZE];
        
        if (signal(SIGHUP, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGHUP\n");
        
        if(arguments.pid_file!=NULL){
			strcpy(pid_file,arguments.pid_file);
		} else {
			strcpy(pid_file,PID_FILE);
		}
        
        /* Write pid to pidfile */
        fpid = fopen(pid_file, "w");
        printf("PID file = %s\n",pid_file);
        if(fpid == NULL) 
		{
			perror("Error opening PIDFILE");
			return(-1);
		}
		if(fprintf(fpid, "%d", getpid())<=0){
			perror("Error writting PID to PIDFILE");
			return(-1);
		}
		fclose(fpid);

		/* Create the FIFO if it does not exist */
        umask(0);
        mknod(arguments.args[0], S_IFIFO|0666, 0);
        
        fp = fopen(arguments.args[0], "r");
        if(fp == NULL) 
		{
			perror("Error opening FIFO");
			return(-1);
		}
		
		printf("FIFO file opened\n");
		
		fout = fopen(arguments.args[1], "a");
        if(fout == NULL) 
		{
			perror("Error opening LOG file");
			return(-1);
		}

        while(1)
        {
			if( fgets(readbuf, BUFF_SIZE, fp)!=NULL ){
				
				if(reload || !file_exists(arguments.args[1])){
					if(arguments.debug)
						printf("Line in new file: %s\n", readbuf);
					fclose(fout);
					fout = fopen(arguments.args[1], "a");
					if(fout == NULL) 
					{
						perror("Error re-opening LOG file after receiving SIGHUP or after LOG file has been deleted");
						return(-1);
					}
					reload=0;
				}
				
                if(fputs(readbuf, fout)>0){
					if(fputs("\n", fout)>0){
						// continue work
					} else {
						printf("Write new line to log file failed\n");
						break;
					}
				} else {
					printf("Write line to log file failed\n");
					break;
				}
             
			} else {
				// re-open FIFO, this will block until the FIFO is re-opened for writing
				
				if(arguments.debug)
					printf("Last read line from FIFO: %s\n", readbuf);
				
				fp = fopen(arguments.args[0], "r");
				if(fp == NULL) 
				{
					perror("Error re-opening FIFO");
					return(-1);
				}
		
				printf("FIFO file re-opened\n");
			}
        }
        
        fclose(fp);
        return(0);
}
