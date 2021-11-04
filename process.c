#include <process.h>
#include <unistd.h>


/* Helper Functions */
void handle_redirection(const CMD *cmd)
{
    printf("IN HANDLE DIRECTION");
        // Standard input <
        if(cmd->fromType == RED_IN)
        {
            // printf("REDIRECT INPUT");
            unsigned int fd = open(cmd->toFile, O_RDONLY);
            if (fd < 0) {
                exit(errno);
            }
            dup2(fd, STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor. 
            close(fd);
        }

        // >
        if(cmd->toType == RED_OUT)
        {
            printf("test\n");
            // DIE(%s, "RIP")
            unsigned int fd = open(cmd->fromFile, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor. 
            close(fd);
        }

        if(cmd->toType == RED_OUT_APP)
        {
            unsigned int fd = open(cmd->fromFile, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor. 
            close(fd);
        }
}



/* Main functions */
int handle_simple(const CMD *cmd)
{
    // printf("Parent Process\n");

    int status;

    pid_t p = fork();

    if (p == 0) // child process
    {
        handle_redirection(cmd);
        execvp(cmd->argv[0], cmd->argv); // where actual code is run
    } else if (p > 0) {
        waitpid(p, &status, WUNTRACED);
    }

     // wait for child process to finish
    return 0;
}

int handle_pipeline(const CMD *cmdList)
{


    handle_simple(cmdList);
}

int process(const CMD *cmdList)
{
    
    handle_simple(cmdList);
}


