#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

struct Order {
    int order_id; //arrival sequence(internal use)
    char order_number[10]; //order number [0-9]
    char order_name; //A to I for 9 product types
    int order_quantity; //quantity of product
    time_t due; //due date
};

struct Period {
    time_t start;
    time_t end;
};

struct Schedule {
    int schedule_id; //(internal use)

    struct Period period;
    int schedule_A[2][10000]; //order_id, quantity
    int schedule_B[2][10000]; //order_id, quantity
    int schedule_C[2][10000]; //order_id, quantity
};

struct Scheduler {
    pid_t pid;
    int fdp2c[2]; //pipe from parent to child
    int fdc2p[2]; //pipe from child to parent
};

int isRemaining(int a, int b) {
    if (a == 0) return 0;
    if (a < 0) return b;
    return -1;
}

int MinThree(int a, int b, int c) {
    int min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

int MinTwo(int a, int b) {
    int min = a;
    if (b < min) min = b;
    return min;
}

int MaxThree(int a, int b, int c) {
    int max = a, ind = 0;
    if (b > max) max = b; ind = 1;
    if (c > max) max = c; ind = 2;
    return ind;
}

int MaxTwo(int a, int b) {
    int max = a, ind = 0;
    if (b > max) max = b; ind = 1;
    return ind;
}


int remainingDueTimeCalculation(int Quantity, int RT1, int RT2, int RT3, int usedTime[3]) {
    int i, RX, RY, RZ, usedTimeX = 0, usedTimeY = 0, usedTimeZ = 0, shortestDue, mostRemaining, mark = 0, Mark = -1;
    for (i = 0; i < RT1 + RT2 + RT3; i++) {
        if (RT1 == 0 && RT2 == 0 && RT3 == 0 || Quantity <= 0) break;
        RX = RT1 > 0 ? Quantity % 300: __INT_MAX__;
        RY = RT2 > 0 ? Quantity % 400: __INT_MAX__;
        RZ = RT3 > 0 ? Quantity % 500: __INT_MAX__;
        if (RX < RY && RX < RZ) {
            RT1--;
            Quantity -= 300;
            usedTimeX++;
            mark = isRemaining(Quantity, 1);
        } 
        else if (RY < RX && RY < RZ) {
            RT2--;
            Quantity -= 400;
            usedTimeY++;
            mark = isRemaining(Quantity, 2);
        }
        else if (RZ < RX && RZ < RY) {
            RT3--;
            Quantity -= 500;
            usedTimeZ++;
            mark = isRemaining(Quantity, 3);
        }
        else if (RZ == RX && RX == RY) {
            shortestDue = MinThree(RT1, RT2, RT3);
            if (shortestDue * 1200 > Quantity) {
                switch(MaxThree(RT1, RT2, RT3)) {
                    case 0:
                        RT1--;
                        usedTimeX++;
                        Quantity -= 300;
                        mark = isRemaining(Quantity, 1);
                        break;
                    case 1:
                        RT2--;
                        usedTimeY++;
                        Quantity -= 400;
                        mark = isRemaining(Quantity, 2);
                        break;
                    case 2:
                        RT3--;
                        usedTimeZ++;
                        Quantity -= 500;
                        mark = isRemaining(Quantity, 3);
                        break;
                    default:
                        break;
                }
            }
            else {
                RT1--;
                RT2--;
                RT3--;
                usedTimeX++;
                usedTimeY++;
                usedTimeZ++;
                Quantity -= 1200;
            }
        }
        else if (RX == RY && RX != RZ) {
            shortestDue = MinTwo(RT1, RT2);
            if (shortestDue * 700 > Quantity) {
                switch(MaxTwo(RT1, RT2)) {
                    case 0:
                        RT1--;
                        usedTimeX++;
                        Quantity -= 300;
                        mark = isRemaining(Quantity, 1);
                        break;
                    case 1:
                        RT2--;
                        usedTimeY++;
                        Quantity -= 400;
                        mark = isRemaining(Quantity, 2);
                        break;
                    default:
                        break;
                }
            }
            else {
                RT1--;
                RT2--;
                usedTimeX++;
                usedTimeY++;
                Quantity -= 700;
            }
        }
        else if (RX == RZ && RX != RY) {
            shortestDue = MinTwo(RT1, RT3);
            if (shortestDue * 700 > Quantity) {
                switch(MaxTwo(RT1, RT3)) {
                    case 0:
                        RT1--;
                        usedTimeX++;
                        Quantity -= 300;
                        mark = isRemaining(Quantity, 1);
                        break;
                    case 1:
                        RT3--;
                        usedTimeZ++;
                        Quantity -= 500;
                        mark = isRemaining(Quantity, 3);
                        break;
                    default:
                        break;
                }
            }
            else {
                RT1--;
                RT3--;
                usedTimeX++;
                usedTimeZ++;
                Quantity -= 800;
            }
        }
        else if (RY == RZ && RY != RX) {
            shortestDue = MinTwo(RT2, RT3);
            if (shortestDue * 700 > Quantity) {
                switch(MaxTwo(RT2, RT3)) {
                    case 0:
                        RT2--;
                        usedTimeY++;
                        Quantity -= 400;
                        mark = isRemaining(Quantity, 2);
                        break;
                    case 1:
                        RT3--;
                        usedTimeZ++;
                        Quantity -= 500;
                        mark = isRemaining(Quantity, 3);
                        break;
                    default:
                        break;
                }
            }
            else {
                RT2--;
                RT3--;
                usedTimeY++;
                usedTimeZ++;
                Quantity -= 900;
            }
        }
        if(mark != -1) Mark = mark;
    }
    usedTime[0] = usedTimeX;
    usedTime[1] = usedTimeY;
    usedTime[2] = usedTimeZ;
    return Mark;
}

int main() {
    printf("~~WELCOME TO PLS~~\n");

    struct Order order[10];
    for(int i = 0; i < 10; i++) {
        order[i].order_id = i;
        order[i].order_number[0] = '\0';
        order[i].order_name = 'A';
        order[i].order_quantity = 0;
        order[i].due = 0;
    }
    
    pid_t pid[4];
    struct Scheduler scheduler;

    if (pipe(scheduler.fdp2c) < 0 || pipe(scheduler.fdc2p) < 0) {
        perror("pipe");
        return 1;
    }

    scheduler.pid = fork();
    if (scheduler.pid < 0) {
        perror("fork");
        return 1;
    } else if (scheduler.pid == 0) {
        // child
        close(scheduler.fdp2c[1]);
        close(scheduler.fdc2p[0]);
        // read from parent
        struct Order order[10];
        int n = read(scheduler.fdp2c[0], &order, sizeof(order));
        if (n < 0) {
            perror("read");
            return 1;
        }
        //// schedule
        struct Schedule schedule;
        schedule.schedule_id = 0;
        schedule.period.start = 0;
        schedule.period.end = 0;
        for(int i = 0; i < 10; i++) {
            schedule.schedule_A[0][i] = order[i].order_id;
            schedule.schedule_A[1][i] = order[i].order_quantity;
            schedule.schedule_A[2][i] = order[i].due;
        }
        // write to parent
        n = write(scheduler.fdc2p[1], &schedule, sizeof(schedule));
        if (n < 0) {
            perror("write");
            return 1;
        }
        close(scheduler.fdp2c[0]);
        close(scheduler.fdc2p[1]);
    } else {
        // parent
        close(scheduler.fdp2c[0]);
        close(scheduler.fdc2p[1]);
        // write to child
        int n = write(scheduler.fdp2c[1], &order, sizeof(order));
        if (n < 0) {
            perror("write");
            return 1;
        }
        struct Schedule schedule;
        // read from child
        n = read(scheduler.fdc2p[0], &schedule, sizeof(schedule));
        if (n < 0) {
            perror("read");
            return 1;
        }
        //print the schedule
        printf("Schedule ID: %d\n", schedule.schedule_id);
        printf("Period: %ld to %ld\n", schedule.period.start, schedule.period.end);
        close(scheduler.fdp2c[1]);
        close(scheduler.fdc2p[0]);
    }
    
    // wait for child
    wait(NULL);
    return 0;
}