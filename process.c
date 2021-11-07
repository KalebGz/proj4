/*
TODO
-  Go through all commands that need to die gracefully and add warnigns

*/
#include <process.h>
#include <unistd.h>

/* Helper Functions */
void handle_redirection(const CMD *cmd)
{
    // Standard input <
    if (cmd->fromType == RED_IN)
    {
        dup2(fd, STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    if (cmd->fromType == RED_IN_HERE)
    {
        char file_name[] = "/tmp/myTmpFile-XXXXXX";
        unsigned int temp_fd = mkstemp(file_name);
        write(temp_fd, cmd->fromFile, strlen(cmd->fromFile));
        close(temp_fd);

        unsigned int fd = open(file_name, O_RDONLY);
        dup2(fd, STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    if (cmd->toType == RED_OUT)
    {
        // printf("REDIR OUT: %s", cmd->toFile);
        fflush(stdout);
        unsigned int fd = open(cmd->toFile, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    //
    if (cmd->toType == RED_OUT_APP)
    {
        unsigned int fd = open(cmd->toFile, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }
}

/* Main functions */
int handle_simple_sub_cmd(const CMD *cmd)
{
    int status;
    pid_t p = fork();
    if (p < 0)
    {
        perror("fork");
    }

    if (p == 0) // child process
    {
        handle_redirection(cmd);

        // Deal with local variables
        int numVals = cmd->nLocal;
        for (int i = 0; i < numVals; i++)
        {
            setenv(cmd->locVar[i], cmd->locVal[i], 1);
        }
        

        // dumpTree(cmd);
        if (cmd->type == SUBCMD)
        {
            // printf("Commands");
            exit(process(cmd->left));
        }

        execvp(cmd->argv[0], cmd->argv); // where actual code for command is run
        perror("execvp");
        exit(errno); // makes sure that child exists
    }
    else if (p > 0)
    {   

        // waitpid(p, &status, WUNTRACED);
    }

    return STATUS(status);
}

int handle_pipeline(const CMD *cmdList)
{

    int status_left;
    int status_right;
    int fd[2]; // [0] read end, [1] write end
    pipe(fd);
    // Process the left node
    pid_t left_p = fork();
    if (left_p == 0) // child process
    {
        dup2(fd[1], STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd[1]);
        close(fd[0]);
        exit(process(cmdList->left));
    }

    // Process thr right node
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

    status_right = STATUS(status_right);
    status_left = STATUS(status_left);
    // exit/status code: 0=success, >0= failure
    return ((status_right > 0) ? status_right : status_left);
}

int handle_sep_and_or(const CMD *cmdList)
{
    int status_left;
    int status_right;

    // Process the left node
    pid_t left_p = fork();
    if (left_p == 0) // child process
    {
        exit(process(cmdList->left));
    }

    waitpid(left_p, &status_left, WUNTRACED);
    status_left = STATUS(status_left);

    if (cmdList->type == SEP_AND)
    {
        if (status_left > 0)
        {
            return status_left;
        }
    }
    else
    {
        if (status_left == 0)
        {
            return status_left;
        }
    }

    // Process thr right node
    pid_t right_p = fork();
    if (right_p == 0) // child process
    {
        exit(process(cmdList->right));
    }

    waitpid(right_p, &status_right, WUNTRACED);
    status_right = STATUS(status_right);

    // Exit code: 0=success, >0= failure
    return ((status_right > 0) ? status_right : status_left);
}


int handle_process(const CMD *cmdList)
{
    int status = 0;
    switch (cmdList->type)
    {
    case PIPE:
        status = handle_pipeline(cmdList);
        break;
    case SIMPLE:
    case SUBCMD:
        status = handle_simple_sub_cmd(cmdList);
        break;
    case SEP_AND:
    case SEP_OR:
        status = handle_sep_and_or(cmdList);
        break;
    case SEP_BG:
        status = handle_sep_bg(cmdList);
    }

    return status;
}

int process(const CMD *cmdList)
{

    handle_process(cmdList);
    // REAP ZOMBIES

    // LOOP
        // returns something // waitpid -1 
        // om
}
