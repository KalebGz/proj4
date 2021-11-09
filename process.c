/*
TODO
-  Go through all commands that need to die gracefully and add warnigns
- figure out when you need to add exit(errno)
*/

#include <process.h>
#include <unistd.h>
int handle_process(const CMD *cmdList);
/* STRUCTS */

// Stack refactored from proj1
typedef struct node
{
    struct node *Next;
    char *Str;
    // int BeforeNode;
    // int AfterNode;
} Node;

typedef struct stack
{
    Node *Head;
    // String *OutputBuffer;
    // String *AfterOutputBuffer;
    // String *InputBuffer;
    // MacroList *DefMacros;
    // int ExpandCalls;
} Stack;

/*GLOBAL VARIABLES*/
Stack *DIR_STACK = NULL; //Global stack for pushd and popd

Stack *create_stack()
{
    Stack *s = (Stack *)malloc(sizeof(Stack));
    s->Head = NULL;
    // s->AfterOutputBuffer = create_string();
    // s->OutputBuffer = create_string();
    // s->InputBuffer = create_string();
    // s->DefMacros = create_macro_list();
    // s->ExpandCalls = 0;
    return s;
}

Node *create_node()
{
    Node *n = (Node *)malloc(sizeof(Node));
    n->Str = NULL;
    n->Next = NULL;
    // n->BeforeNode = 0;
    // n->AfterNode = 0;
    return n;
}

bool isEmpty(Stack *s)
{
    return (s->Head == NULL);
}

// Prints string values of the nodes in DIR_STACK
int print_stack(Stack *s)
{
    if (isEmpty(s))
    {
        return 1;
    }

    Node *print_node = s->Head;
    while (print_node)
    {
        (print_node->Next == NULL)? printf("%s", print_node->Str) : printf("%s ", print_node->Str);

        print_node = print_node->Next;
    }
    // printf("TEST HEAD:%s ", s->Head->Str);
    // printf("TEST NEXT: %s ", s->Head->Next->Str);
    printf("\n");
    return 0;
}

void free_node(Node *n)
{

    // free_string(n->Str);
    free(n->Str);
    free(n);
}

void free_stack(Stack *s)
{
    // free(s->Head); I think this already gets freed when the node it points to gets freed using freeNode
    // free_string(s->OutputBuffer);
    // free_string(s->InputBuffer);
    free(s);
}

void stack_push(Stack *st, Node *n)
{
    // if(st->Head)
    // {
    //     printf("TEST HEAD:%s \n", st->Head->Str);
    //     // printf("TEST NEXT: %s \n", DIR_STACK->Head->Next->Str);
    // }
    if (st->Head != NULL)
    {
        n->Next = st->Head;
    }
    st->Head = n;
    // if(st->Head->Next)
    // {
    //     printf("TEST HEAD:%s \n", st->Head->Str);
    //     printf("TEST NEXT: %s \n", st->Head->Next->Str);
    // }
}

// Called if head stack node's string is empty
bool stack_pop(Stack *st)
{
    if (isEmpty(st))
    {
        return false;
    }
    else
    {
        Node *newHead = st->Head->Next;
        free_node(st->Head);
        st->Head = newHead;
    }
    return true;
}

void push_string_to_stack(char *str, Stack *s)
{

    Node *newNode = create_node();
    // string_putString(newNode->Str, str->Content, str->Size);
    // newNode->Str = (char *)malloc(sizeof(char) * strlen(str));
    newNode->Str = strdup(str);
    stack_push(s, newNode);
}

// Handle redirection commands ( >, >>, <, <<)
void handle_redirection(const CMD *cmd)
{

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

/* Built in commands*/
// handles cd command
int handle_cd(const CMD *cmdList)
{
    if (cmdList->argc > 1)
    {
        chdir(cmdList->argv[1]);
    }
    else
    {
        chdir(getenv("HOME"));
    }
    return 0;
}

int handle_pushd(const CMD *cmdList)
{
    // Node *newNode = create_node();
    // char* current_dir =  // buffer = largest value of size_t(65535)
    // char *currDir = getcwd(getenv("PWD"), (size_t) 65535);

    // char *paramDir = getcwd(cmdList->argv[1], (size_t)strlen(cmdList->argv[1]));
    // printf("PWD DIR: %s \n", getenv("PWD"));
    // printf("CURR DIR: %s \n", currDir);
    // printf("Param DIR:%s \n", paramDir);

    // push_string_to_stack(currDir, DIR_STACK);
    if (isEmpty(DIR_STACK))
    {
        char *currDir = getcwd(getenv("PWD"), PATH_MAX);
        push_string_to_stack(currDir, DIR_STACK);
    }

    // print_stack(DIR_STACK);
    // Change directory directory passed into command
    if (cmdList->argc > 1)
    {
        if (chdir(cmdList->argv[1]) < 0)
        {
            return 0; // chdir failed
            perror("chdir");
        }
    }

    char *paramDir = getcwd(getenv("PWD"), (size_t)65535);
    // printf("Param DIR: %s \n", paramDir);

    push_string_to_stack(paramDir, DIR_STACK);
    // printf("TESTS\n");
    // printf("TEST HEAD:%s \n", DIR_STACK->Head->Str);
    // printf("TEST NEXT: %s \n", DIR_STACK->Head->Next->Str);
    print_stack(DIR_STACK);

    return 0;
}

int handle_popd()
{
    if (isEmpty(DIR_STACK))
    {
        printf("STACK IS EMPTY\n");
    }
    else
    {
        char *dir_top_stack = getcwd(DIR_STACK->Head->Str, strlen(DIR_STACK->Head->Str));
        chdir(dir_top_stack);
        stack_pop(DIR_STACK);
    }
    print_stack(DIR_STACK);
    return 0;
}

// Handle simple(standard bash commands) and ()
int handle_simple_sub_cmd(const CMD *cmd)
{

    // Check if commands are built-in pushd {dir}, popd or cd
    if (cmd->argc > 0)
    {
        if (!strcmp(cmd->argv[0], "cd"))
        {
            return handle_cd(cmd);
        }

        if (!strcmp(cmd->argv[0], "pushd"))
        {
            return handle_pushd(cmd);
        }

        if (!strcmp(cmd->argv[0], "popd"))
        {
            return handle_popd();
        }
    }

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
        handle_process(cmdList->left->left);
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
        if (cmdList->right)
        {
            status = handle_process(cmdList->right);
        }
        fprintf(stderr, "Backgrounded: %d\n", p); // print to stderr
        // waitpid(p, &status, WUNTRACED);
    }
    // return status of right if it exists, otherwise return 0
   
    return (cmdList->right) ? STATUS(status) : 0;
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

    waitpid(left_p, &status_left, WUNTRACED);

    // Process right node if it exists
    if (cmdList->right)
    {
        pid_t right_p = fork();
        if (right_p == 0) // child process
        {
            handle_redirection(cmdList);
            exit(handle_process(cmdList->right));
        }
        waitpid(right_p, &status_right, WUNTRACED);
    }

    status_right = STATUS(status_right);
    status_left = STATUS(status_left);
    // exit/status code: 0=success, >0= failure
    return ((status_right > 0) ? status_right : status_left);
}

/* main functions */
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
    if (DIR_STACK == NULL)
    {
        DIR_STACK = create_stack();
    }

    int status = handle_process(cmdList);

    // reap zombies
    int reap_status;
    int reaped_pid = waitpid(-1, &reap_status, WNOHANG);
    while (reaped_pid > 0)
    {
        fprintf(stderr, "Completed: %d (%d)\n", reaped_pid, reap_status); // print to stderr
        reaped_pid = waitpid(-1, &reap_status, WNOHANG);
    }

    return status;
}
