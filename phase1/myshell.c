#include "myshell.h"
#include<errno.h>
#include<math.h>
#define MAXARGS   128

char HOME[MAXLINE]; //���� �����ϴ� Ȩ ���丮�� ����

void eval(char* cmdline);
int parseline(char* buf, char** argv, int* argc);
int builtin_command(char** argv);
void command_history(); //builtin_command�� 
void history_option1(char* string);
int history_option2(char* string, int number);
void command_echo(); //echo���� ����ǥ�� �����ϰ� ����ϴ� ���� �ݿ��ϱ� ���� �Լ�
void save_command_history(); //�Էµ� command�� ���ؼ� �ߺ� ���� ó���ؼ� �����ϴ� �Լ�
int test_history(char** argv, int argc, char* cmdline);
 
int main() {
    char cmdline[MAXLINE]; //command line
    getcwd(HOME, sizeof(HOME)); //���� �������� ���� ���丮 ��ġ�� HOME���� ����

    while (1) {
        printf("CSE4100-MP-P1> "); //Shell Prompt: print your prompt

        fgets(cmdline, MAXLINE, stdin); //1. READING

        if (feof(stdin))  exit(0);
       
        eval(cmdline);
    }
}

void eval(char* cmdline) {
    char* argv[MAXARGS]; //argument list execve()
    char buf[MAXLINE];   //holds modified command line
    int bg;              //should the job run in bg(backgroud) or fg(frontgroud)?
    pid_t pid;           //process id
    int argc = 0;
    
    char PATH[MAXARGS] = "/usr/bin/";

    strcpy(buf, cmdline); //copy command line into buf
    bg = parseline(buf, argv, &argc); //2. PARSING
    if (argv[0] == NULL) return; //ignore empty lines
    
    if (test_history(argv, argc, cmdline)) { //argv[0]�� !!�̳� !#�� �ִ��� �˻� --> ������ �ش� �������� ��ü
        argc = 0;
        strcpy(buf, cmdline); //copy command line into buf
        bg = parseline(buf, argv, &argc); //2-1. PARSING AGAIN
        if (argv[0] == NULL) return; //ignore empty lines
    }; 
    
    if (!strcmp(argv[0], "echo")) {
        if ((pid = Fork()) == 0) {
            command_echo(argv, argc);
            exit(0); //child process terminate
        }
        int status;
        if (Waitpid(pid, &status, 0) < 0) //���⼭ child�� pid�� �־����� ���������� parent process�� ���
            unix_error("waitfg: waitpid error");

        save_command_history(cmdline);
        return;
    }

    if (strcmp(argv[0], "history") == 0) {
        command_history();
        save_command_history(cmdline); //history�� ����
        return;
    }

    if (!builtin_command(argv)) { //examine if the command is builtin command
        if ((pid = Fork()) == 0) {
            if (execve(strcat(PATH, argv[0]), argv, environ) < 0) { 
                printf("%s: %s\n", argv[0], strerror(errno)); //�ش� �κ��� ���� �޽��� ����ϴ� �κ�
                exit(0);
            }
        }
        int status;
        if (Waitpid(pid, &status, 0) < 0) //���⼭ child�� pid�� �־����� ���������� parent process�� ���
            unix_error("waitfg: waitpid error");
    }

    save_command_history(cmdline);
    return;
}

int builtin_command(char** argv) { //if built-in command, return 1
    if (strcmp(argv[0], "exit") == 0) exit(0); //need not to return(shell done)

    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL || !strcmp(argv[1], "$HOME")) argv[1] = HOME; //�׳� cd�� �ְų� cd $HOME�� ������ �� ���� �� ���� Ȩ ���丮�� �̵�
        if (chdir(argv[1]) != 0) //chdir function(=change directory): if success to move, return 0, else return -1
            fprintf(stderr, "%s: No such file or directory\n", argv[1]);
        return 1;
    }

    if (strcmp(argv[0], "history") == 0) return 1;

    return 0; //not built-in command                    
}

int test_history(char** argv, int argc, char* cmdline) {
    char string[MAXLINE]; //�� Ŀ�ǵ� ������ �״�� ���� ���� �ֱ� ����
    char temp[MAXARGS];
    int j = 0;
    char save[MAXARGS] = { 0 }; //! �ڿ� ������ char�� ���ڸ� �����ϴ� ����
    int count = 0; //! �ڿ� ������ char�� ������ ����
    int number = 0; //! �ڿ� ������ char�� ���ڸ� int������ ������ ��
    char new_cmdline[MAXLINE]; //���� ������� Ŀ�ǵ带 ������ ����
    int change = 0;

    int len = strlen(argv[0]);
    for (int i = 0; i < len; i++) {
        if ((i < len - 1) && argv[0][i] == '!') { //!�� ó�� ���� ���
            if ((i + 1 < len) && argv[0][i + 1] == '!') { //!!�� ��� ���� ���
                history_option1(string);
                string[strlen(string) - 1] = '\0'; //string���� ������ Ŀ�ǵ� ������ �������� ���� ���๮�ڸ� \0���� ��ü
                argv[0][i] = '\0'; //!�� �����ϴ� �κ��� ������ ���� ������ ����
                strncpy(temp, &argv[0][i + 2], sizeof(argv[0]) - (i + 2)); //!�� ������ �κк��� �������� temp�� ����
                strcat(argv[0], string);
                strcat(argv[0], temp);

                change = 1; //����Ǿ��ٴ� flag
            }
            else if ((i + 1 < len) && argv[0][i + 1] >= 48 && argv[0][i + 1] <= 57) { //����ĭ�� ���ڰ� ������
                j = i + 1;
                while ((j < len) && argv[0][j] >= 48 && argv[0][j] <= 57) { //���ڰ� �����°� ���� ������
                    save[count++] = argv[0][j++];
                }
                number = atoi(save);
                if (history_option2(string, number) == 1) {
                    string[strlen(string) - 1] = '\0'; //string���� ������ Ŀ�ǵ� ������ �������� ���� ���๮�ڸ� \0���� ��ü
                    argv[0][i] = '\0'; //!�� �����ϴ� �κ��� ������ ���� ������ ����
                    strncpy(temp, &argv[0][j], sizeof(argv[0]) - (j - 3)); //!#�� ������ �κк��� �������� temp�� ����
                    strcat(argv[0], string);
                    strcat(argv[0], temp);

                    change = 1; //����Ǿ��ٴ� flag
                }
            }
        }
    }

    if (change == 1) { //argv�� !!�� !#�� ���� ����Ǿ��ٸ�, new cmdline�� ���� ���� cmdline�� ����
        strcpy(new_cmdline, argv[0]);
        for (int i = 1; i < argc; i++) {
            strcat(new_cmdline, " ");
            strncat(new_cmdline, argv[i], sizeof(argv[i]));
        }
        strcat(new_cmdline, "\n");

        strcpy(cmdline, new_cmdline);

        if (argc == 1) printf("%s", cmdline); //��ü�� ��, ������ argv[0] 1���� Ŀ��常 �ִ� ��쿡�� ����� Ŀ��带 �ѹ� ����־�� ��

        return 1;
    }
    else return 0;
}

void command_history() {
    char str[MAXLINE] = { 0 };
    char history_path[MAXLINE] = { 0, }; //Ȩ ���丮�� .history ������ �ִ��� üũ�ϱ� ���� ���
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");
    
    FILE* fp = fopen(history_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: No history\n");
        return;
    }

    char line[MAXLINE];
    int count = 1;
    while (fgets(line, MAXLINE, fp) != NULL) {
        printf("%d  %s", count++, line);
    }
    if (!strcmp(line, "history\n")) { //������ ���� history������ ������ ���� history ��� ���ϰ�
        //do nothing
    }
    else { //������ ���� history�� �ƴϿ�����, ������ ���� history ���
        printf("%d  %s\n", count, "history");
    }
    
    fclose(fp);
}

void history_option1(char* string) { // !!�� ���, ������ commandline���� ��ü
    char history_path[MAXLINE] = { 0, }; // Ȩ ���丮�� .history ������ �ִ��� üũ�ϱ� ���� ���
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");

    FILE* fp;
    fp = fopen(history_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: No history\n");
        return;
    }
    int cnt = 1;
    fseek(fp, -2, SEEK_END); // ������ ���������� �о���̱�(������ ������ �ǳʶٰ�)
    char c = fgetc(fp);
    while (c != '\n' && ftell(fp) > 0) {
        fseek(fp, -2, SEEK_CUR);
        c = fgetc(fp);
    }

    fgets(string, MAXLINE, fp); // ������ ���� str�� ����
    fclose(fp);
}

int history_option2(char* string, int number) { //!#�� ���, Ư�� ������ command�� ��ü
    char history_path[MAXLINE] = { 0, }; //Ȩ ���丮�� .history ������ �ִ��� üũ�ϱ� ���� ���
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");

    FILE* fp;
    fp = fopen(history_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: No history\n");
        return -1;
    }

    int line_count = 0;
    while (fgets(string, MAXLINE, fp) != NULL) { //.history ���Ͽ� �� �ٱ��� �ִ��� �ľ�
        line_count++;
    }

    if (line_count < number) { //�־��� number�� history log�� ���� ���� �ѹ��� ���� ���
        fprintf(stderr, "Not valid line number\n");
        fclose(fp); //������ �ݰ�
        return -1; //���� ����
    }

    line_count = 0;
    fseek(fp, 0, SEEK_SET); //�ٽ� ���� �� ������ �̵�
    while (fgets(string, MAXLINE, fp) != NULL) {
        line_count++;
        if (line_count == number) {
            break;
        }
    }

    fclose(fp);
    return 1; //���� �� ����
}

void save_command_history(char* cmdline) { 
    char str[MAXLINE] = { 0 };
    char history_path[MAXLINE] = { 0, }; //Ȩ ���丮�� .history ������ �ִ��� üũ�ϱ� ���� ���
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");
   
    FILE* fp; char* buf = NULL;

    //1. ������ �ִ��� Ȯ��
    fp = fopen(history_path, "r");
    if (fp != NULL) { //������ �ִ� ���
        long int file_size;  int count = 0;

        fseek(fp, 0, SEEK_END); //������ ũ�� ���ϱ�
        file_size = ftell(fp);

        buf = (char*)malloc(file_size + 1); //���� ���������� �о���̱�
        fseek(fp, -1, SEEK_END);
        int i = 0;

        while (ftell(fp) > 0) {
            char c = fgetc(fp);
            if (c == '\n') {
                count++;
                if (count == 1) buf[i++] = c;
                else break;
            }
            else buf[i++] = c;
            fseek(fp, -2, SEEK_CUR);
        }
        buf[i] = '\0';


        int len = strlen(buf); // ������ ������
        for (int i = 0; i < len / 2; i++) {
            char temp = buf[i];
            buf[i] = buf[len - i - 1];
            buf[len - i - 1] = temp;
        }

        fclose(fp);

        if (strcmp(buf, cmdline) == 0) { //2-1. �ߺ��̸� �׳� ����
            free(buf);
            return;
        }
    }

    //2-2. ������ ���ų� �ߺ��� �ƴϸ�, ��ɾ��� ���� ���� ������� ����
    if(buf != NULL) free(buf);

    fp = fopen(history_path, "a");
    if (fp == NULL) {
        fprintf(stderr, "No file to save history\n");
        exit(0);
    }
    fprintf(fp, "%s", cmdline);
    fclose(fp);
}

//echo���� ����ǥ�� �����ϰ� ����ϴ� ���� �ݿ��ϱ� ���� �Լ�
void command_echo(char** argv, int argc) {
    char str[MAXLINE] = { 0 }; // str �ʱ�ȭ
    char temp[MAXLINE] = { 0 }; // temp �ʱ�ȭ
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            for (int j = 0; j < strlen(argv[i]); j++) {
                if (argv[i][j] == '\'' || argv[i][j] == '\"') {
                    argv[i][j] = '\0';
                    strcpy(temp, &argv[i][j + 1]);
                    strcat(argv[i], temp);
                }
            }
            if (strlen(argv[i]) + strlen(str) + 1 >= MAXLINE) { // �ִ� ���� �ʰ� �� ���� ó��
                fprintf(stderr, "command_echo: argument too long\n");
                return;
            }
            strcat(str, argv[i]);
            strcat(str, " ");
        }
        strcat(str, "\n");
        printf("%s", str);
    }
    else printf("\n");
}

int parseline(char* buf, char** argv, int* argc) {
    char* delim;        /* Points to first space delimiter */
    int bg;             /* Background job? */

    buf[strlen(buf) - 1] = ' ';  //command���� �ִ� ���๮��('\n')�� �������� ��ü
    while (*buf && (*buf == ' ')) //������� �����ϰ�
        buf++;

    while ((delim = strchr(buf, ' '))) {
        argv[(*argc)++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[(*argc)] = NULL;

    if ((*argc) == 0)  /* Ignore blank line */
        return 1; 

    /* Should the job run in the background? */
    if ((bg = (*argv[(*argc) - 1] == '&')) != 0) //if last character of command is '&', it means
        argv[--(*argc)] = NULL;

    return bg;
}