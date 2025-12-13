#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>

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
    printf("Commands for duckhunt go here!\n");
}

int main(int argc, char *argv[]) {

    int opt;
    char *subopts;
    char *value;
    int errfnd = 0;

    printf("Given arguments? %d\n", argc);
    for(int i=0;i<argc;i++){
        printf("%d: %s \n", i, argv[i]);
    }

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

    seropts.c_cflag = CS8 | CREAD | CLOCAL;
    seropts.c_cc[VMIN] = 1;
    seropts.c_cc[VTIME] = 5;
    cfsetospeed(&seropts, DUCKHUNT_BAUDRATE);
    cfsetispeed(&seropts, DUCKHUNT_BAUDRATE);

    tcsetattr(fd, TCSANOW, &seropts);

    struct termios canon; //STDIN handles input, and it requires end of line in basic settings. We have to turn that off
    tcgetattr(STDIN_FILENO,&canon);

    canon.c_lflag &= ~(ICANON | ECHO);  // no end of line buffering, echo had to be disabled to not use getche()
    canon.c_cc[VMIN]  = 1;
    canon.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &canon);

    int c;
    while(1){
        c = getchar();
        if(c != EOF){
            write(fd, &c, 1);
            printf("%d sent", c);
            c = EOF;
        }
        
    }
    close(fd);

    exit(EXIT_SUCCESS);
}

