int tt(int Rx, int Ry, int Rz, int Q, int alloc[3]) {
    int delta = Q;

    for (int i = 0; i <= Rx; ++i) {
        for (int j = 0; j <= Ry; ++j) {
            for (int k = 0; k <= Rz; ++k) {
                int remain = i * 300 + j * 400 + k * 500 - Q;
                if (remain >= 0 && remain <= delta) {
                    alloc[0] = i;
                    alloc[1] = j;
                    alloc[2] = k;
                    delta = remain;
                }
            }
        }
    }
    printf("alloc is %d %d %d\n", alloc[0], alloc[1], alloc[2]);

    int re = alloc[0] * 300 + alloc[1] * 400 + alloc[2] * 500 - Q;
    printf("re is %d\n", re);
    //find which plant where the vacancy from
    if (re == 0) {
        return 0;
    } else if (re >= 400) {
        return 3;
    } else if (re >=300) {
        return 2;
    } else {
        return 1;
    }
}

int main()
{
    int t = tt(2, 2, 2, 1701, (int[3]){0, 0, 0});
    printf("t is %d\n", t);
    return 0;
}

