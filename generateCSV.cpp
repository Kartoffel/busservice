#include <stdio.h>

const unsigned int tStart = 5;
const unsigned int tEnd = 1805;
const unsigned int steps = 360;

int main(void) {
    char command[100];
    printf("\"t\",\"Waiting time (s)\",\"Seat availability (%%)\""
        ",\"Overtakes\",\"Profit (%%)\"\n");
    for (int t = tStart; t <= tEnd; t += (tEnd - tStart) / steps) {
        printf("%d,", t);
        sprintf(command, "./busservice %d %d %d", t, t * 2, t * 6);
        char buff[100];
        FILE *openFile = popen(command,"r");
        while (fgets(buff, sizeof(buff), openFile) != NULL) {
            printf("%s", buff);
        }
        pclose(openFile);

    }
    return 0;
}
