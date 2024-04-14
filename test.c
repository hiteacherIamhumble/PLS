#include<stdio.h>
#include <limits.h>

int isRemaining(int a, int b) {
    if (a = 0) return 0;
    if (a < 0) return b;
    return -1;
}

int isDueFinished(int RT1, int RT2, int RT3, int Quantity) { // 1 if due can be finished, 0 if not
    if (RT1 * 300 + RT2 * 400 + RT3 * 500 >= Quantity) {
        return 1;
    } else {
        return 0;
    }
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

//return which plant has internal fragmentation
//0: no internal fragmentation, 1: plant A, 2: plant B, 3: plant C
int remainingDueTimeCalculation(int Quantity, int RT1, int RT2, int RT3, int usedTime[3]) {
    int i, RX, RY, RZ, usedTimeX = 0, usedTimeY = 0, usedTimeZ = 0, shortestDue, mostRemaining, mark = 0;
    for (i = 0; i < RT1 + RT2 + RT3; i++) {
        if (RT1 == 0 && RT2 == 0 && RT3 == 0 || Quantity <= 0) break;
        RX = RT1 > 0 ? Quantity % RT1: __INT_MAX__;
        RY = RT2 > 0 ? Quantity % RT2: __INT_MAX__;
        RZ = RT3 > 0 ? Quantity % RT3: __INT_MAX__;
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
    }
    usedTime[0] = usedTimeX;
    usedTime[1] = usedTimeY;
    usedTime[2] = usedTimeZ;
    return mark;
}

int main() { //这只是一个test文件，debug专用嘿嘿
    int Quantity = 2000, RT1 = 2, RT2 = 3, RT3 = 4, usedTime[3] = {0, 0, 0};
    int mark = remainingDueTimeCalculation(Quantity, RT1, RT2, RT3, usedTime);
    printf("Plant A: %d, Plant B: %d, Plant C: %d\n", usedTime[0], usedTime[1], usedTime[2]);
    printf("Internal Fragmentation: %d\n", mark);
    return 0;
}