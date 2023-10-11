#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>

#define MYSH_TOK_DELIM "\t\r\n"//声明分词的依据 回车换行制表符
#define MYSH_TOK_BUFFER_SIZE 64 //规定分完词后的最大size
#define HISTORY_FILE "./mysh_history"
//编辑内置命令
int mysh_cd(char **args);
int mysh_help(char **args);
int mysh_exit(char **args);
int mysh_history(char **args);
int mysh_which(char **args);
int mysh_builtin_nums();


char * builtin_cmd[] =//内置命令数组
{
    "cd",
    "help",
    "exit",
    "history",
    "which",
    
};

int (*builtin_func[])(char ** )=//定义函数指针数组把内置命令存储起来
{
    &mysh_cd,
    &mysh_help,
    &mysh_exit,
    &mysh_history,
    &mysh_which,
    
};

void saveHistory(char *command) { //添加了 saveHistory 函数
//用于将用户输入的命令保存到历史记录文件中
    FILE *file = fopen(HISTORY_FILE, "a");// fopen 函数用于打开名为 HISTORY_FILE 的文件
    //并返回一个指向该文件的 FILE 类型指针
    /*  FILE *file 声明了一个指向 FILE 类型的指针变量，
    名为 file。
    这个指针将用于引用打开的文件，以便后续对文件进行操作*/
    if (file) {//当使用 fopen 函数打开文件时，如果文件打开成功，fopen 函数将返回一个非空的文件指针。
    //如果文件打开失败，fopen 函数将返回 NULL，表示文件指针为空。
    //if (file) 的目的是检查文件指针是否为非空    
        fprintf(file, "%s\n", command);//把命令写入history中
        fclose(file);// fclose 函数关闭文件，以释放文件资源和确保文件的正确操作
    } else {
        perror("Error saving command to history");
    }
}

void printHistory() //用户输入history时输出history内容
{
    FILE *file = fopen(HISTORY_FILE, "r");//read only
    if (file) {
        char line[100];
        int count = 1;
        while (fgets(line, sizeof(line), file)) //读取history中的内容
        {
            printf("%d. %s", count, line);
            count++;
        }
        fclose(file);
    } else {
        perror("Error reading command history");
    }
}


int mysh_cd(char **args)
{
    if(args[1] == NULL)//即cd后面是空的不知道打开啥
    {
        perror("Mysh error at cd, lack of args\n");
    }
    else
    {
        if(chdir(args[1]) != 0)
        {
            perror("Mysh error at chdir\n");
        }

    }
    return 1;
}

int mysh_help(char **args)
{
    puts("This is JYT's shell");
    puts("Here are some builtin cmd:");
    for (int i = 0; i<mysh_builtin_nums(); i ++)//展示我的内置函数
       printf("%s\n",builtin_cmd[i]);
       return 1;
    
}

int mysh_exit(char **args)
{
    return 0;
}

int mysh_history(char **args) 
{
    printHistory();
    return 1;
}

int mysh_which(char **args)
{
if (args[1] == NULL)//表明只有which后面是空的
{
printf("Usage: which <command>\n");
return 1;
}

for (int i = 0; i < mysh_builtin_nums(); i++)
{
    if (strcmp(args[1], builtin_cmd[i]) == 0)//比较内置函数和输入的内容
    //检查是否是内置命令
    {
        printf("%s is a builtin command\n", args[1]);
        return 1;
    }
}

char *path = getenv("PATH");//getenv大概是获取环境变量 具体的内容暂时不太清楚
char *token = strtok(path, ":");//分词
while (token != NULL)
{
    char command[100];
    sprintf(command, "%s/%s", token, args[1]);//将token 和args[1]的内容写入command中
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





int mysh_builtin_nums()
{
    return sizeof(builtin_cmd) / sizeof(builtin_cmd[0]);
}

char * mysh_read_line()//定义readline函数
{
    char * line =NULL;
    ssize_t bufsize;//(ssize_t为有符号整型)
    getline(&line,&bufsize,stdin);//getline 函数用于从文件流或标准输入中读取一行文本
    //并将其存储在动态分配的字符数组中//stdin是输入流（用户标准输入的内容）
    return line;

}

//分词
char ** mysh_split_line(char * line)//返回值是一个字符串数组
{
    int buffer_size = MYSH_TOK_BUFFER_SIZE,position = 0;//存在第一个
    char **tokens = malloc(buffer_size * sizeof(char *));//分配空间 
    char *token;
    
    token = strtok(line,MYSH_TOK_DELIM);//依据delim分词strtok是分词函数
    while(token != NULL) //进行多次分词直到为空
    {
        tokens[position++] = token;//分给position然后往后走一个
        token = strtok(NULL,MYSH_TOK_DELIM);

    }
    tokens[position] = NULL;
    return tokens;

}

int mysh_launch(char **args)//加载函数
{
    pid_t pid,wpid;//创建进程
    int status;

    pid = fork();//fork函数用于创建一个新的进程 实现多进程并发执行的功能
    if (pid==0)//子进程进行的代码
    {
        if(execvp(args[0],args) == -1)//execvp 函数是 Linux 系统中的一个系统调用函数
        //用于在当前进程中执行一个新的程序。参数包括file指针和命令行
        //args[0]是指令名，args是参数
          perror("Mysh error at execvp");
        exit(EXIT_FAILURE);//execvp执行后不会再执行接下来的代码
    }
    else
    {
        do
        {
            wpid = waitpid(pid,&status,WUNTRACED);

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        /*这段代码使用 waitpid 函数等待子进程的结束，
        并在子进程结束后获取其退出状态。
        通过循环调用 waitpid 函数，
        可以确保在子进程结束之前，父进程一直等待并处理子进程的退出状态。
        （这段关于进程的描述其实我不太懂只是简单照抄了一下）*/  
    }
    return 1;
}

int mysh_execute(char **args)//执行内置命令
{   
    saveHistory(args[0]);
    
    if (args[0] == NULL) return 1;
    
    for (int i = 0; i < mysh_builtin_nums(); i ++)//检查是否是内置命令，如果是就执行
        if (strcmp(args[0],builtin_cmd[i]) == 0)
              return (*builtin_func[i])(args);


        
    return mysh_launch(args);//如果不是内置命令就让系统自己去执行
        


}


void mysh_loop()//先做框架再逐个完成函数
{
    char *line;//存储用户输入的东西
    char **args;//二维数组存储分完词之后的参数（char**是指针的指针）比如将cd \desk\user 分成"cd" "\desk\user"
    int status;//标志命令状态的返回
    do
    {   //做出一个终端提示符
        char path[100];//定义一个path数组
        getcwd(path,100);//调用 getcwd 函数，将获取到的路径存储在 path 中
        char now[200] = "[mysh ";
        strcat(now,path);//字符串拼接函数
        strcat(now,"]$");
        printf("%s",now);//输出字符串

        line = mysh_read_line();//该函数用于读入终端命令并存在line中
        args = mysh_split_line(line);//把line进行分词，分完词后存入args
        status = mysh_execute(args);//搞一个执行函数 并将结果存入status

        free(line);
        free(args);

    } while (status);//status不为零就要一直等待用户的下一次命令
    
}

int main(int argc,char *argv[])
{
    
    FILE *file = fopen(HISTORY_FILE, "w");
    if (file) {
        fclose(file);
    } else {
        perror("Error creating history file");
    }
    mysh_loop(); //模拟不断在运行的程序
   
    return 0;  
}