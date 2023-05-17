#include "myshell.h"
#include<errno.h>
#include<math.h>
#define MAXARGS   128

char HOME[MAXLINE]; //쉘이 시작하는 홈 디렉토리의 정보

void eval(char* cmdline);
int parseline(char* buf, char** argv, int* argc);
int builtin_command(char** argv);
void command_history(); //builtin_command로 
void history_option1(char* string);
int history_option2(char* string, int number);
void command_echo(); //echo에서 따옴표를 제외하고 출력하는 것을 반영하기 위한 함수
void save_command_history(); //입력된 command에 대해서 중복 예외 처리해서 저장하는 함수
int test_history(char** argv, int argc, char* cmdline);
 
int main() {
    char cmdline[MAXLINE]; //command line
    getcwd(HOME, sizeof(HOME)); //쉘이 시작했을 때의 디렉토리 위치를 HOME으로 설정

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
    
    if (test_history(argv, argc, cmdline)) { //argv[0]에 !!이나 !#이 있는지 검사 --> 있으면 해당 라인으로 대체
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
        if (Waitpid(pid, &status, 0) < 0) //여기서 child의 pid를 넣었으니 끝날때까지 parent process는 대기
            unix_error("waitfg: waitpid error");

        save_command_history(cmdline);
        return;
    }

    if (strcmp(argv[0], "history") == 0) {
        command_history();
        save_command_history(cmdline); //history는 저장
        return;
    }

    if (!builtin_command(argv)) { //examine if the command is builtin command
        if ((pid = Fork()) == 0) {
            if (execve(strcat(PATH, argv[0]), argv, environ) < 0) { 
                printf("%s: %s\n", argv[0], strerror(errno)); //해당 부분이 오류 메시지 출력하는 부분
                exit(0);
            }
        }
        int status;
        if (Waitpid(pid, &status, 0) < 0) //여기서 child의 pid를 넣었으니 끝날때까지 parent process는 대기
            unix_error("waitfg: waitpid error");
    }

    save_command_history(cmdline);
    return;
}

int builtin_command(char** argv) { //if built-in command, return 1
    if (strcmp(argv[0], "exit") == 0) exit(0); //need not to return(shell done)

    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL || !strcmp(argv[1], "$HOME")) argv[1] = HOME; //그냥 cd만 있거나 cd $HOME이 들어왔을 때 만든 쉘 기준 홈 디렉토리로 이동
        if (chdir(argv[1]) != 0) //chdir function(=change directory): if success to move, return 0, else return -1
            fprintf(stderr, "%s: No such file or directory\n", argv[1]);
        return 1;
    }

    if (strcmp(argv[0], "history") == 0) return 1;

    return 0; //not built-in command                    
}

int test_history(char** argv, int argc, char* cmdline) {
    char string[MAXLINE]; //긴 커맨드 라인이 그대로 들어올 수도 있기 때문
    char temp[MAXARGS];
    int j = 0;
    char save[MAXARGS] = { 0 }; //! 뒤에 나오는 char형 숫자를 저장하는 변수
    int count = 0; //! 뒤에 나오는 char형 숫자의 갯수
    int number = 0; //! 뒤에 나오는 char형 숫자를 int형으로 저장한 값
    char new_cmdline[MAXLINE]; //새로 만들어진 커맨드를 저장할 변수
    int change = 0;

    int len = strlen(argv[0]);
    for (int i = 0; i < len; i++) {
        if ((i < len - 1) && argv[0][i] == '!') { //!이 처음 나온 경우
            if ((i + 1 < len) && argv[0][i + 1] == '!') { //!!이 모두 나온 경우
                history_option1(string);
                string[strlen(string) - 1] = '\0'; //string으로 가져온 커맨드 라인의 마지막에 있을 개행문자를 \0으로 대체
                argv[0][i] = '\0'; //!가 시작하는 부분을 강제로 문장 끝으로 만듦
                strncpy(temp, &argv[0][i + 2], sizeof(argv[0]) - (i + 2)); //!가 끝나는 부분부터 끝까지를 temp에 복사
                strcat(argv[0], string);
                strcat(argv[0], temp);

                change = 1; //변경되었다는 flag
            }
            else if ((i + 1 < len) && argv[0][i + 1] >= 48 && argv[0][i + 1] <= 57) { //다음칸에 숫자가 나오면
                j = i + 1;
                while ((j < len) && argv[0][j] >= 48 && argv[0][j] <= 57) { //숫자가 나오는게 멈출 때까지
                    save[count++] = argv[0][j++];
                }
                number = atoi(save);
                if (history_option2(string, number) == 1) {
                    string[strlen(string) - 1] = '\0'; //string으로 가져온 커맨드 라인의 마지막에 있을 개행문자를 \0으로 대체
                    argv[0][i] = '\0'; //!가 시작하는 부분을 강제로 문장 끝으로 만듦
                    strncpy(temp, &argv[0][j], sizeof(argv[0]) - (j - 3)); //!#이 끝나는 부분부터 끝까지를 temp에 복사
                    strcat(argv[0], string);
                    strcat(argv[0], temp);

                    change = 1; //변경되었다는 flag
                }
            }
        }
    }

    if (change == 1) { //argv가 !!나 !#에 의해 변경되었다면, new cmdline을 만들어서 기존 cmdline을 갱신
        strcpy(new_cmdline, argv[0]);
        for (int i = 1; i < argc; i++) {
            strcat(new_cmdline, " ");
            strncat(new_cmdline, argv[i], sizeof(argv[i]));
        }
        strcat(new_cmdline, "\n");

        strcpy(cmdline, new_cmdline);

        if (argc == 1) printf("%s", cmdline); //대체된 후, 기존의 argv[0] 1개의 커멘드만 있는 경우에는 실행될 커멘드를 한번 띄워주어야 함

        return 1;
    }
    else return 0;
}

void command_history() {
    char str[MAXLINE] = { 0 };
    char history_path[MAXLINE] = { 0, }; //홈 디렉토리에 .history 파일이 있는지 체크하기 위한 경로
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
    if (!strcmp(line, "history\n")) { //마지막 줄이 history였으면 마지막 라인 history 출력 안하고
        //do nothing
    }
    else { //마지막 줄이 history가 아니였으면, 마지막 라인 history 출력
        printf("%d  %s\n", count, "history");
    }
    
    fclose(fp);
}

void history_option1(char* string) { // !!인 경우, 마지막 commandline으로 대체
    char history_path[MAXLINE] = { 0, }; // 홈 디렉토리에 .history 파일이 있는지 체크하기 위한 경로
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");

    FILE* fp;
    fp = fopen(history_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: No history\n");
        return;
    }
    int cnt = 1;
    fseek(fp, -2, SEEK_END); // 파일의 끝에서부터 읽어들이기(마지막 개행은 건너뛰고)
    char c = fgetc(fp);
    while (c != '\n' && ftell(fp) > 0) {
        fseek(fp, -2, SEEK_CUR);
        c = fgetc(fp);
    }

    fgets(string, MAXLINE, fp); // 마지막 줄을 str에 저장
    fclose(fp);
}

int history_option2(char* string, int number) { //!#인 경우, 특정 라인의 command로 대체
    char history_path[MAXLINE] = { 0, }; //홈 디렉토리에 .history 파일이 있는지 체크하기 위한 경로
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");

    FILE* fp;
    fp = fopen(history_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: No history\n");
        return -1;
    }

    int line_count = 0;
    while (fgets(string, MAXLINE, fp) != NULL) { //.history 파일에 몇 줄까지 있는지 파악
        line_count++;
    }

    if (line_count < number) { //주어진 number가 history log에 없는 라인 넘버면 오류 출력
        fprintf(stderr, "Not valid line number\n");
        fclose(fp); //파일을 닫고
        return -1; //에러 종료
    }

    line_count = 0;
    fseek(fp, 0, SEEK_SET); //다시 파일 맨 앞으로 이동
    while (fgets(string, MAXLINE, fp) != NULL) {
        line_count++;
        if (line_count == number) {
            break;
        }
    }

    fclose(fp);
    return 1; //변경 후 종료
}

void save_command_history(char* cmdline) { 
    char str[MAXLINE] = { 0 };
    char history_path[MAXLINE] = { 0, }; //홈 디렉토리에 .history 파일이 있는지 체크하기 위한 경로
    strcpy(history_path, HOME);
    strcat(history_path, "/.history");
   
    FILE* fp; char* buf = NULL;

    //1. 파일이 있는지 확인
    fp = fopen(history_path, "r");
    if (fp != NULL) { //파일이 있는 경우
        long int file_size;  int count = 0;

        fseek(fp, 0, SEEK_END); //파일의 크기 구하기
        file_size = ftell(fp);

        buf = (char*)malloc(file_size + 1); //파일 끝에서부터 읽어들이기
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


        int len = strlen(buf); // 순서를 뒤집음
        for (int i = 0; i < len / 2; i++) {
            char temp = buf[i];
            buf[i] = buf[len - i - 1];
            buf[len - i - 1] = temp;
        }

        fclose(fp);

        if (strcmp(buf, cmdline) == 0) { //2-1. 중복이면 그냥 종료
            free(buf);
            return;
        }
    }

    //2-2. 파일이 없거나 중복이 아니면, 명령어의 실행 여부 상관없이 저장
    if(buf != NULL) free(buf);

    fp = fopen(history_path, "a");
    if (fp == NULL) {
        fprintf(stderr, "No file to save history\n");
        exit(0);
    }
    fprintf(fp, "%s", cmdline);
    fclose(fp);
}

//echo에서 따옴표를 제외하고 출력하는 것을 반영하기 위한 함수
void command_echo(char** argv, int argc) {
    char str[MAXLINE] = { 0 }; // str 초기화
    char temp[MAXLINE] = { 0 }; // temp 초기화
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            for (int j = 0; j < strlen(argv[i]); j++) {
                if (argv[i][j] == '\'' || argv[i][j] == '\"') {
                    argv[i][j] = '\0';
                    strcpy(temp, &argv[i][j + 1]);
                    strcat(argv[i], temp);
                }
            }
            if (strlen(argv[i]) + strlen(str) + 1 >= MAXLINE) { // 최대 길이 초과 시 예외 처리
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

    buf[strlen(buf) - 1] = ' ';  //command끝에 있는 개행문자('\n')를 공백으로 대체
    while (*buf && (*buf == ' ')) //공백들은 무시하고
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