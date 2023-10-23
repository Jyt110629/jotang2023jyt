#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MYSH_TOK_DELIM " \t\r\n"//声明分词的依据 回车换行制表符
#define MYSH_TOK_BUFFER_SIZE 64//规定分完词后的最大size
#define ALIAS_BUFFER_SIZE 64//规定alias缓冲区的大小
#define RED_COLOR "\033[0;31m"
#define RESET_COLOR "\033[0m"

typedef struct {
    char* alias;
    char* command;
} Alias;//定义了一个结构体类型Alias


int mysh_cd(char **args);
int mysh_help(char **args);
int mysh_exit(char **args);
int mysh_which(char **args);
int mysh_alias(char **args);



int mysh_builtin_nums();

char * builtin_cmd[] =//内置命令数组
{
    "cd",
    "help",
    "exit",
    "which",
    "alias",
    
};

int (*builtin_func[])(char **) = //定义函数指针数组把内置命令存储起来
{
    &mysh_cd,
    &mysh_help,
    &mysh_exit,
    &mysh_which,
    &mysh_alias,
    
};

int mysh_cd(char **args) {
    if(args[1] == NULL)
    {//即cd后面是空的不知道打开啥
        perror(RED_COLOR"Mysh error at cd, lack of args\n"RESET_COLOR);
    } else {
        if(chdir(args[1]) != 0) {
            perror(RED_COLOR"Mysh error at chdir\n"RESET_COLOR);
        }
    }
    return 1;
}

int mysh_help(char **args) {
    puts("This is JYT's shell");
    puts("Here are some builtin cmd:");
    for (int i = 0; i < mysh_builtin_nums(); i++) //展示我的内置函数
    {   
        printf("%s\n", builtin_cmd[i]);
    }
    return 1;
}

int mysh_exit(char **args) {
    return 0;
}

int mysh_which(char **args) {
    if (args[1] == NULL) 
    {//表明只有which后面是空的
        printf("Usage: which <command>\n");
        return 1;
    }

    for (int i = 0; i < mysh_builtin_nums(); i++) 
    {
        if (strcmp(args[1], builtin_cmd[i]) == 0) 
        {////比较内置函数和输入的内容
        //检查是否是内置命令    
            printf("%s is a builtin command\n", args[1]);
            return 1;
        }
    }

    char *path = getenv("PATH");//getenv大概是获取环境变量 具体的内容暂时不太清楚
    char *token = strtok(path, ":");//分词
    while (token != NULL) {
        char command[100];
        sprintf(command, "%s/%s", token, args[1]);////将token 和args[1]的内容写入command中
        if (access(command, F_OK) == 0)//判断文件是否存在 
        {
            printf("%s\n", command);
            return 1;
        }
        token = strtok(NULL, ":");
    }

    printf("%s not found\n", args[1]);
    return 1;
}

Alias alias_list[ALIAS_BUFFER_SIZE];//alias_list数组的元素类型都是Alias结构体
int alias_count = 0;

int mysh_alias(char **args) 
{
    if (args[1] == NULL || args[2] == NULL) //当alias指令后面没有足够的字符串的时候提示格式
    {
        printf("Usage: alias <alias_name> <command>\n");
        return 1;
    }

    if (alias_count < ALIAS_BUFFER_SIZE) {
        alias_list[alias_count].alias = strdup(args[1]);
                //strdup() 函数将参数 s 指向的字符串复制到一个字符串指针上去
        alias_list[alias_count].command = strdup(args[2]);
        alias_count++;
    } else {
        printf("Alias buffer is full\n");
    }

    return 1;
}

int mysh_builtin_nums()
{
    return sizeof(builtin_cmd) / sizeof(builtin_cmd[0]);
}

char * mysh_read_line()//定义readline函数
{
    char * line = NULL;
    ssize_t bufsize;//(ssize_t为有符号整型)
    getline(&line, &bufsize, stdin);//getline 函数用于从文件流或标准输入中读取一行文本
    //并将其存储在动态分配的字符数组中//stdin是输入流（用户标准输入的内容）
    return line;
}




char ** mysh_split_line(char * line)//返回值是一个字符串数组
{
    int buffer_size = MYSH_TOK_BUFFER_SIZE, position = 0;//存在第一个
    char **tokens = malloc(buffer_size * sizeof(char *));//分配空间 
    char *token;
    
    token = strtok(line, MYSH_TOK_DELIM);////依据delim分词strtok是分词函数
    while(token != NULL) //进行多次分词直到为空
    {
        tokens[position++] = token;//分给position然后往后走一个
        token = strtok(NULL, MYSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}



int mysh_launch(char **args, int background) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (background) {
            // 忽略 SIGINT 和 SIGTSTP 信号
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
        }
        
        int input_redirect_index = -1;
        int output_redirect_index = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "<") == 0) {
                input_redirect_index = i;
                break;
            } else if (strcmp(args[i], ">") == 0) {
                output_redirect_index = i;
                break;
            }
        }

        if (input_redirect_index != -1) {
            int fd = open(args[input_redirect_index + 1], O_RDONLY);
            if (fd == -1) {
                perror(RED_COLOR"Mysh error at open\n"RESET_COLOR);
                return 1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (output_redirect_index != -1) {
            int fd = open(args[output_redirect_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror(RED_COLOR"Mysh error at open\n"RESET_COLOR);
                return 1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        if (execvp(args[0], args) == -1) {
            perror(RED_COLOR"Mysh error at execvp\n"RESET_COLOR);
            return 1;
        }
    } else if (pid > 0) {
        if (!background) {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    } else {
        perror(RED_COLOR"Mysh error at fork\n"RESET_COLOR);
        return 1;
    }

    return 1;
}


int mysh_execute(char **args)//执行内置命令
{
    if (strcmp(args[0],"kill") == 0)//添加对kill命令的处理，以实现杀死进程的功能
    {
        if (args[1] == NULL)
        {
            printf("Usage:kill<pid>\n");
            return 1;
        }

        int pid = atoi(args[1]);
        if (kill(pid,SIGKILL) == -1)
        {
            perror(RED_COLOR"Mysh error at kill\n"RESET_COLOR);
        }
        return 1;
    }

    if(args[0] == NULL) return 1;

    int background = 0;
    for (int i = 0; args[i] != NULL; i++)//通过&实现后台运行
    {
        if (strcmp(args[i], "&") == 0)
        {
            background = 1;
            args[i] = NULL;
            break;
        }
    }
    
    for (int i = 0; i < mysh_builtin_nums(); i ++)//检查是否是内置命令，如果是就执行
        if(strcmp(args[0], builtin_cmd[i]) == 0)
            return (*builtin_func[i])(args);
    
    for (int i = 0; i < mysh_builtin_nums(); i++) {
        if (strcmp(args[0], builtin_cmd[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    for (int i = 0; i < alias_count; i++) 
    {
        if (strcmp(args[0], alias_list[i].alias) == 0) 
        {
            char *alias_command = strdup(alias_list[i].command);//将alias_list[i].command复制到alias_command
            char *token;
            char **alias_args = malloc(MYSH_TOK_BUFFER_SIZE * sizeof(char *));//动态分配内存
            int position = 0;


            token = strtok(alias_command, MYSH_TOK_DELIM);//和mysh_split_line函数相同进行分词之后在把args传入execute函数中运行
            while (token != NULL) {
                alias_args[position++] = token;
                token = strtok(NULL, MYSH_TOK_DELIM);
            }
            alias_args[position] = NULL;

            int result = mysh_execute(alias_args);

            free(alias_args);
            free(alias_command);

            return result;
        }
    }

    return mysh_launch(args,background);//如果不是内置命令就让系统自己去执行
}

void mysh_loop()//先做框架再逐个完成函数
{
    char *line;//存储用户输入的东西
    
    char **args;//二维数组存储分完词之后的参数（char**是指针的指针）比如将cd \desk\user 分成"cd" "\desk\user"
    int status;//标志命令状态的返回
    
    FILE *history_file = fopen("~/jytshell_history","a");
    
    do
    {   //做出一个终端提示符
        char path[100];//定义一个path数组
        getcwd(path, 100);//调用 getcwd 函数，将获取到的路径存储在 path 中
        char now[200] = "[mysh ";
        strcat(now, path);//字符串拼接函数
        strcat(now, " ]$");
        printf("%s", now);//输出字符串

        line = mysh_read_line();//该函数用于读入终端命令并存在line中
        args = mysh_split_line(line);//把line进行分词，分完词后存入args
        status = mysh_execute(args);//搞一个执行函数 并将结果存入status

        if (history_file != NULL)
        {
            fprintf(history_file,"%s\n",line);
            fclose(history_file);
        }
        
        free(line);
        free(args);
        
        
        
        
    } while (status);//status不为零就要一直等待用户的下一次命令
    
}

int main(int argc, char *argv[])
{
   // 检查历史记录文件是否存在，如果不存在则创建
    FILE *history_file = fopen("~/yourshellname_history", "r");
    if (history_file == NULL)
    {
        history_file = fopen("~/yourshellname_history", "w");
        
    }
    else
    {
        fclose(history_file);
    }

    mysh_loop();//模拟不断在运行的程序
    return 0;
}

