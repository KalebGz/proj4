/*
TODO
-  Go through all commands that need to die gracefully and add warnigns
- figure out when you need to add exit(errno)
*/


#include <process.h>
#include <unistd.h>

// Handle redirection commands ( >, >>, <, <<)
void handle_redirection(const CMD *cmd)
{

    // if (fd < 0)
    // {
    //     exit(errno);
    // }
    // Standard input <
    if (cmd->fromType == RED_IN)
    {
        unsigned int fd = open(cmd->fromFile, O_RDONLY);
        dup2(fd, STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    // <<
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

    // >
    if (cmd->toType == RED_OUT)
    {
        // printf("REDIR OUT: %s", cmd->toFile);
        // fflush(stdout);
        unsigned int fd = open(cmd->toFile, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }

    // >>
    if (cmd->toType == RED_OUT_APP)
    {
        unsigned int fd = open(cmd->toFile, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(fd, STDOUT_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd);
    }
}

// Handle simple(standard bash commands) and ()
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
            exit(handle_process(cmd->left));
        }

        execvp(cmd->argv[0], cmd->argv); // where actual code for command is run
        perror("execvp");
        exit(errno); // makes sure that child exists
    }
    else if (p > 0)
    {

        waitpid(p, &status, WUNTRACED);
    }

    return STATUS(status);
}

// Handle | command
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
        exit(handle_process(cmdList->left));
    }

    // Process thr right node
    pid_t right_p = fork();
    if (right_p == 0) // child process
    {
        dup2(fd[0], STDIN_FILENO); //close whatever is in the descriptor for stdout, and replace it with a copy of the new_stdout_fd descriptor.
        close(fd[0]);
        close(fd[1]);
        exit(handle_process(cmdList->right));
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

// Handle && and || commands
int handle_sep_and_or(const CMD *cmdList)
{
    int status_left;
    int status_right;

    // Process the left node
    pid_t left_p = fork();
    if (left_p == 0) // child process
    {
        exit(handle_process(cmdList->left));
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
        exit(handle_process(cmdList->right));
    }

    waitpid(right_p, &status_right, WUNTRACED);
    status_right = STATUS(status_right);

    // Exit code: 0=success, >0= failure
    return ((status_right > 0) ? status_right : status_left);
}

// handle & command
int handle_sep_bg(const CMD *cmdList)
{
    int status;

    // Deal with backgrounded process not being in leftmost node due to SEP_END(;) operator
    if (cmdList->left->type == SEP_END)
    {
        exit(handle_process(cmdList->left->left));
    }
    pid_t p = fork();

    if (p < 0)
    {
        perror("fork");
    }

    if (p == 0) // child process
    {
        handle_redirection(cmdList);

        // Deal with local variables
        // int numVals = cmd->nLocal;
        // for (int i = 0; i < numVals; i++)
        // {
        //     setenv(cmd->locVar[i], cmd->locVal[i], 1);
        // }

        //  bcakground proceeding node(location determined based on wheter or not SEP_END(;) is present)
        if (cmdList->left->type == SEP_END)
        {
            exit(handle_process(cmdList->left->right));
        }
        else
        {
            exit(handle_process(cmdList->left));
        }
    }
    else if (p > 0)
    {
        // Process simple(or whatever) command on right node if left node is SEP_END(;)
        if (cmdList->left->type == SEP_END)
        {
            exit(handle_process(cmdList->right));
        }
        fprintf(stderr, "Backgrounded: %d\n", p); // print to stderr
        // waitpid(p, &status, WUNTRACED);
    }

    return STATUS(status);
}

// Handle ; command
int handle_sep_end(const CMD *cmdList)
{

    int status_left;
    int status_right;

    pid_t left_p = fork();
    if (left_p == 0) // child process
    {
        handle_redirection(cmdList);
        // Process left node
        exit(handle_process(cmdList->left));
    }

    // Process right node if it exists
    if (cmdList->right)
    {
        pid_t right_p = fork();
        if (right_p == 0) // child process
        {
            exit(handle_process(cmdList->right));
        }
        waitpid(right_p, &status_right, WUNTRACED);
    }

    waitpid(left_p, &status_left, WUNTRACED);
    

    status_right = STATUS(status_right);
    status_left = STATUS(status_left);
    // exit/status code: 0=success, >0= failure
    return ((status_right > 0) ? status_right : status_left);

}

// Funtion for mapping parsed command with approproate function
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
        break;
    case SEP_END:
        status = handle_sep_end(cmdList);
        break;
    }

    return status;
}
int process(const CMD *cmdList)
{

    int status = handle_process(cmdList);
    // REAP ZOMBIES

    // LOOP
    // returns something // waitpid -1
    // om
    return status;
}
