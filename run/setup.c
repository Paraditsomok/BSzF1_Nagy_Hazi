#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/poll.h>   
#include <signal.h>     

#define TERMINAL_STR 64

#define DUCKHUNT_BAUDRATE B115200 //duckhunt only work on 115200 Baud rate, if needed you can replace this with a list
int32_t g_serspeed = 0;

char g_serfile[TERMINAL_STR]="";

enum{
    DEVICE_OPT = 0,
    SPEED_OPT
};

char *const token[] = {
    [DEVICE_OPT] = "dev",
    [SPEED_OPT] = "speed"
};

void print_info() {
    printf("Settings for the app!\n");
    printf("Use -h or -? to ask for the help menu\n");
    printf("Use -s to give your device settings, in the format dev=/dev/tty*,baudrate=*. Currently my program uses 115200. There could be a global case if it was initiated in some other way.\n");
}



int main(int argc, char *argv[]) {

    int opt;
    char *subopts;
    char *value;
    int errfnd = 0;

    while((opt = getopt(argc,argv, "h?s:")) != -1){
        switch(opt){
            case 'h':
                print_info();
                break;
            case '?':
                print_info();
                break;
            case 's':
                printf("You've given info for UART\n");
                subopts = optarg;
                while((*subopts != '\0') && !errfnd){
                    switch(getsubopt(&subopts, token, &value)){
                        case DEVICE_OPT:
                            printf("%s",value);
                            if(strlen(value) < TERMINAL_STR){
                                strcpy(g_serfile, value);
                                printf("%s",g_serfile);
                            }
                            else{
                                printf("Serial file name is too long!\n");
                                exit(EXIT_FAILURE);
                            }
                            break;
                        case SPEED_OPT:
                            printf("%s",value);
                            g_serspeed = atoi(value);
                            if(g_serspeed!=115200){
                                printf("Invalid Baudrate, duckhunt works with 115200 only!\n");
                                exit(EXIT_FAILURE);
                            }
                            break;
                    }
                }
        break;
        default:
                printf("Unknown cmd option, see -h or -?\n");
                break;
        }
    }

    int fd = open(g_serfile, O_RDWR);
    if(fd<0){
        printf("This device doesn't exist!");
        exit(EXIT_FAILURE);
    }

    struct termios seropts;
    tcgetattr(fd, &seropts);

    cfmakeraw(&seropts);

    seropts.c_cflag |= (CS8 | CREAD | CLOCAL);
    seropts.c_cc[VMIN] = 1;
    seropts.c_cc[VTIME] = 0;
    cfsetospeed(&seropts, DUCKHUNT_BAUDRATE);
    cfsetispeed(&seropts, DUCKHUNT_BAUDRATE);

    tcsetattr(fd, TCSANOW, &seropts);

    //I'm not sure about this one
    struct termios kb;
    tcgetattr(STDIN_FILENO, &kb);

    /* cbreak-style input */
    kb.c_lflag &= ~(ICANON | ECHO); // Disable enter from lines
    kb.c_lflag |= ISIG; //keeps the option for special keys

    kb.c_cc[VMIN]  = 1;
    kb.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &kb);

    struct termios old_kb;
    tcgetattr(STDIN_FILENO, &old_kb); // save



    /* non-blocking */
    int kflags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, kflags | O_NONBLOCK); //Adding non blocking settings

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); //Adding non blocking settings


    printf("Welcome to duckhunt! \nPress s to start\nPress h for controls\nPress e to exit\n");

    int diff=0;

    
    struct pollfd controls[2];
    controls[0].fd = STDIN_FILENO;
    controls[0].events = POLLIN;
    controls[1].fd = fd;
    controls[1].events = POLLIN;
    int running = 1;
    int gamerunning = 0;
    char c;
    char r[4];
    int score = 0;
    while(running){
        int success = poll(controls, 2, 100); //sleep for 100ms
        if(success < 0){
            perror("poll");
            break;
        }
        if(controls[0].revents & POLLIN){
            read(STDIN_FILENO, &c, 1);
            switch (c) {
                case 'e':
                    running = 0;
                    break;
                case 's':
                    if (!gamerunning) gamerunning = 1;
                    break;
                case 'h':
                    if (!gamerunning) {
                        printf("to move the hunter left use b, to move it right press j\n"
                            "To shoot the ducks that randomly appear on the other side press a\n"
                            "To change the difficulty, press either '+' or '-'\n"
                            "At the end of the game, you can input your name and store your data\n");
                    }
                    break;
                default:
                    break;
            }
            write(fd, &c, 1);
        }
        if (controls[1].revents & POLLIN) {
            char buf[4];
            read(fd, buf, 4);
            if(buf[0]=='A' && gamerunning){ //Game over
                for(int i=0;i<3;i++){
                    if (buf[i] >= '0' && buf[i] <= '9'){
                        score = score * 10 + (buf[i] - '0');
                    }
                }
                kb.c_lflag |= ICANON | ECHO;
                int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
                flags &= ~O_NONBLOCK;
                fcntl(STDIN_FILENO, F_SETFL, flags);
                tcsetattr(STDIN_FILENO, TCSANOW, &kb);
                char playername[10];
                printf("Enter your name (3 letters): ");
                scanf("%s", playername);
                tcsetattr(STDIN_FILENO, TCSANOW, &old_kb);
                FILE *f = fopen("leaderboard.txt", "a");
                if(f) {
                    fprintf(f, "%d\t%d\t%s\n", score, diff, playername);
                    fclose(f);
                } else {
                    perror("fopen");
                }
                gamerunning = 0;
                score = 0;
            }
            else if(buf[0]=='B'){
                read(fd, &r, 1);
                diff=(int)buf[1]-'0';
                printf("Difficulty set to %d\n", diff);
            }
                
            }
    }
    
    
    close(fd);

    exit(EXIT_SUCCESS);
}

