#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#define MOUSEFILE "/dev/input/mouse0"

int main()
{
    //brightness vars

    FILE *fp;
    int currentBrightness = 1;
    const char *kFileName = "/sys/class/backlight/intel_backlight/brightness";

    //get current brightness

    //mouse vars
    int fd;
    struct input_event ie;
    unsigned char *ptr = (unsigned char *)&ie;
    unsigned char button, bLeft, bMiddle, bRight;
    char x, y;

    if ((fd = open(MOUSEFILE, O_RDONLY | O_NONBLOCK)) == -1)
    {

        printf("NonBlocking %s open ERROR\n", MOUSEFILE);
        exit(EXIT_FAILURE);
    }
    else
    {

        printf("NonBlocking %s open OK\n", MOUSEFILE);
    }

    printf("--------------------------------------------------------\n");
    printf("Left click to decrease brightness\n");
    printf("--------------------------------------------------------\n");

    bool check = 0;
    while (1)
    {
        if (read(fd, &ie, sizeof(struct input_event)) != -1)
        {
            button = ptr[0];
            printf("---------\n");
            printf("%d\n", (int)button);
            bLeft = button & 0x01;
            bRight = button >> 1 & 0x01;
            bMiddle = button >> 2 & 0x01;
            printf("Left : %d\n", bLeft);
            printf("Right : %d\n", bRight);
            printf("Middle: %d\n", bMiddle);
            printf("---------\n");
            x = (char)ptr[1];
            y = (char)ptr[2];
            // printf("Coordinate [x, y]: [%d, %d]\n", x, y);

            //get current brightness
            fp = fopen(kFileName, "w+");
            fscanf(fp, "%d", &currentBrightness);

            /* Handling event here */
            if (bMiddle == 1)
            {
                if (bLeft == 1)
                {
                    fprintf(fp, "%d", currentBrightness - 500);
                }
                if (bRight == 1)
                {
                    fprintf(fp, "%d", currentBrightness + 500);
                }
            }
            /*---------------------*/

            fclose(fp);
            fflush(stdout);
        }
    }

    // //wait for release of middle button
    // int a;
    // int b;
    // while (1)
    // {

    //     if (read(fd, &ie, sizeof(struct input_event)) != -1)
    //     {

    //         a = ptr[0];
    //         b = (a & 0x4) > 0;
    //         if (b == 0)
    //             break;
    //     }
    // }
    close(fd);
    fflush(stdout);

    return 0;
}
