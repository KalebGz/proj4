#include <process.h>
#include <unistd.h>

/* Helper Functions */
void handle_redirection(const CMD *cmd)
{
    // printf("IN HANDLE DIRECTION");
    // Standard input <
    if (cmd->fromType == RED_IN)
    {
        // printf("REDIRECT INPUT");
        unsigned int fd = open(cmd->toFile, O_RDONLY);
        if (fd < 0)
        {
            exit(errno);
        }

        dup2(fd, STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    // >
    if (cmd->toType == RED_OUT)
    {
        unsigned int fd = open(cmd->fromFile, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    //
    if (cmd->toType == RED_OUT_APP)
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

        // printf("LOCAL: %d", cmd->nLocal);
        // fflush(stdout);
        // Deal with local variables
        int numVals = cmd->nLocal;
        for(int i = 0; i < numVals; i++)
        {
            // printf("VAR: %s\n", cmd->locVar[i]);
            // printf("VAL %s\n", cmd->locVal[i]);
            // fflush(stdout);
            setenv(cmd->locVar[i], cmd->locVal[i], 1);
            // cmd->nLocal-=1;
        }

        printf("SIMPLE: %s\n", cmd->argv[0]);
        execvp(cmd->argv[0], cmd->argv); // where actual code for command is run
    }
    else if (p > 0)
    {
        waitpid(p, &status, WUNTRACED);
    }

    
    return STATUS(status);
}

int handle_pipeline(const CMD *cmdList)
{

    if (cmdList->type == PIPE)
    {
        int fd[2]; // [0] read end, [1] write end
        // fd =
        pipe(fd);

        int status_left;
        int status_right;
        // Process the left node
        pid_t left_p = fork();
        if (left_p == 0) // child process
        {
            dup2(fd[1], STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
            close(fd[1]);
            close(fd[0]);
            exit(process(cmdList->left));
        }

        pid_t right_p = fork();
        if (right_p == 0) // child process
        {
            dup2(fd[0], STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
            close(fd[0]);
            close(fd[1]);
            exit(process(cmdList->right));
        }

        close(fd[0]);
        close(fd[1]);
        waitpid(left_p, &status_left, WUNTRACED);
        waitpid(right_p, &status_right, WUNTRACED);
    }

    return 0;
}

int process(const CMD *cmdList)
{
    int status = 0;
    // printf("\tPROCESS: %s\n", cmdList->argv[0]);
    switch (cmdList->type)
    {

    case PIPE:
        status = handle_pipeline(cmdList);
        break;
    case SIMPLE:
        status = handle_simple(cmdList);
        break;
    }

    return status;
}
