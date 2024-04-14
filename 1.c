#include <sys/types.h>
#include <sys/stat.h>
// #include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILENAME_LEN 50
#define MAX_DATE_LEN 32
// struct Date{
//     int year;
//     int month;
//     int day;
// };
// struct Date startDate = {0, 0, 0}, endDate = {0, 0, 0};
struct tm startDate = {0}, endDate = {0};

// when invoking a scheduler, pass the period
struct Period
{
    struct tm startDate;
    struct tm endDate;
};
// struct Period currentPeriod = {{0, 0, 0}, {0, 0, 0}};

#define MAX_ORDERNUMBER_LEN 10
struct Order
{
    int order_id;                           // arrival sequence(internal use)
    char order_number[MAX_ORDERNUMBER_LEN]; // order numbeR
    char product_name;                      // 1 of 9 letters: A, B, C, D, E, F, G, H, I
    int order_quantity;                     // quantity of product
    struct tm dueDate;                      // due date
};

struct Allocation
{
    int order_id; // order id
    int accepted; // 1 for accepted, 0 for denied
    int schedule[3][3]; //plant_id: days since start date, days to produce in that plant, quantity
};

#define MAX_ORDER_NUM 10000
int orderNum = 0;
struct Order order[MAX_ORDER_NUM];

struct Schedule
{
    int schedule_id; //(internal use)

    struct Period period;
    int schedule_X[2][10000]; // order_id, quantity
    int schedule_Y[2][10000]; // order_id, quantity
    int schedule_Z[2][10000]; // order_id, quantity
};

struct Report
{
    struct Allocation allocation[MAX_ORDER_NUM];
    int X[2]; // days, quantity
    int Y[2]; // days, quantity
    int Z[2]; // days, quantity
};

// this is to contain the infos about communication between parent and child process
struct Scheduler
{
    pid_t pid;
    int fdp2c[2]; // pipe from parent to child
    int fdc2p[2]; // pipe from child to parent
};

void promptEnter();
int startsWith(const char *str, const char *prefix);
const struct tm str2Date(const char *str);
void addPeriod(const char *str);
void addOrder(const char *str);
void addBatch(const char *str);
void runPLS(const char *str);
void exitPLS();
void parseInput(const char *str);
void inputModule();
void invokeScheduler(const char *str);
void invoke(const char *str);
int dateDiff(const struct tm *startDate, const struct tm *endDate);
int allocate(int, int, int, int, int[3]);

void promptEnter()
{
    printf("Please enter:\n> ");
}
int startsWith(const char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL)
        return 0; // Handle NULL pointers
    size_t len_prefix = strlen(prefix);
    size_t len_str = strlen(str);

    if (len_prefix > len_str)
        return 0; // Prefix longer than string cannot be a prefix

    return (strncmp(str, prefix, len_prefix) == 0);
}
// const struct Date str2Date(const char *str){
//     struct Date res;
//     sscanf(str, "%d-%d-%d", &res.year, &res.month, &res.day);
//     return res;
// }
const struct tm str2Date(const char *str)
{
    int year, month, day;
    sscanf(str, "%d-%d-%d", &year, &month, &day);
    struct tm res = {0};
    res.tm_year = year - 1900; // year 1900 being 0
    res.tm_mon = month - 1;    // January being 0
    res.tm_mday = day;         // day of the month, day 1 being 1
    return res;
}

/*@para: 2 date; @return: their differnce*/
int dateDiff(const struct tm *startDate, const struct tm *endDate)
{
    time_t start = mktime(startDate);
    time_t end = mktime(endDate);
    if (start == -1 || end == -1)
    {
        perror("mktime");
        exit(1);
    }
    double diff = difftime(end, start);
    return (int)diff / (60 * 60 * 24);
}

int allocate(int quantity, int X_remain, int Y_remain, int Z_remain, int alloc[3]) {
    alloc[0] = 1;
    alloc[1] = 1;
    alloc[2] = 1;
    return 0;
}
// const char* date2Str(const struct Date date){
//     char res[MAX_DATE_LEN];
//     sprintf(res, "%d-%d-%d", date.year, date.month, date.day);
//     // printf("result is %s\n", res);
//     return res;
// }
void addPeriod(const char *str)
{
    // str is a addPERIOD command.
    char startDateStr[MAX_DATE_LEN];
    char endDateStr[MAX_DATE_LEN];
    sscanf(str, "addPERIOD %s %s", startDateStr, endDateStr);
    // printf("start date is %s\n", startDateStr);
    // printf("end date is %s\n", endDateStr);
    startDate = str2Date(startDateStr);
    endDate = str2Date(endDateStr);
    // printf("start date is %d-%d-%d\n", startDate.year, startDate.month, startDate.day);
    // printf("start date is %d-%d-%d\n", endDate.year, endDate.month, endDate.day);
}
void addOrder(const char *str)
{
    // str is a addORDER command.
    char dueDateStr[MAX_DATE_LEN];
    sscanf(str, "addORDER %s %s %d Product_%c", order[orderNum].order_number, dueDateStr, &order[orderNum].order_quantity, &order[orderNum].product_name);
    order[orderNum].order_id = orderNum;
    order[orderNum].dueDate = str2Date(dueDateStr);
    // printf("order number is %s, due date is %s, quantity is %d, product name is %c\n", order[orderNum].order_number, date2Str(order[orderNum].dueDate), order[orderNum].order_quantity, order[orderNum].product_name);
    // printf("order number is %s, due date is %s, quantity is %d, product name is %c\n", order[orderNum].order_number, dueDateStr, order[orderNum].order_quantity, order[orderNum].product_name);
    ++orderNum;
}
void addBatch(const char *str)
{
    char filename[MAX_FILENAME_LEN];
    sscanf(str, "addBATCH %s", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(1);
    }
    const int MAX_INPUT_LEN = 1024;
    char buffer[MAX_INPUT_LEN];
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        // Remove newline character if present
        buffer[strcspn(buffer, "\n")] = 0;
        printf("You entered: %s\n", buffer);
        addOrder(buffer);
    }
}
#define MAX_METHOD_LEN 20
void runPLS(const char *str)
{
    char algorithmName[MAX_METHOD_LEN];
    char reportFileName[MAX_METHOD_LEN];
    sscanf(str, "runPLS %s | printREPORT > %s", algorithmName, reportFileName);
    printf("use algorithm %s, report file to %s\n", algorithmName, reportFileName);
    work(algorithmName, reportFileName);
}
void exitPLS()
{
    printf("Bye-bye!");
    exit(0);
}
void parseInput(const char *str)
{
    if (startsWith(str, "addPERIOD"))
    {
        puts("This is a addPERIOD command.");
        addPeriod(str);
    }
    else if (startsWith(str, "addORDER"))
    {
        puts("This is a addORDER command.");
        addOrder(str);
    }
    else if (startsWith(str, "addBATCH"))
    {
        puts("This is a addBATCH command.");
        addBatch(str);
    }
    else if (startsWith(str, "runPLS"))
    {
        puts("This is a runPLS command.");
        runPLS(str);
    }
    else if (startsWith(str, "exitPLS"))
    {
        puts("This is a exitPLS command.");
        exitPLS();
    }
    else
    {
        fprintf(stderr, "Error: Input command invalid format.\n");
        exit(1);
    }
}
void inputModule()
{
    printf("\n\t~~WELCOME TO PLS~~\n\n");
    while (1)
    {
        promptEnter();
        const int MAX_INPUT_LEN = 1024;
        char buffer[MAX_INPUT_LEN];
        if (fgets(buffer, sizeof(buffer), stdin))
        {
            // Remove newline character if present
            buffer[strcspn(buffer, "\n")] = 0;
            printf("You entered: %s\n", buffer);
            parseInput(buffer);
        }
    }
}
void work(const char *algorithm, const char *filename)
{
    struct Scheduler scheduler;
    if (pipe(scheduler.fdp2c) < 0 || pipe(scheduler.fdc2p) < 0)
    {
        perror("pipe");
        exit(1);
    }
    scheduler.pid = fork();
    if (scheduler.pid < 0)
    {
        perror("error forking.");
        exit(1);
    }
    if (scheduler.pid == 0) //child process
    {
        printf("child process\n");
        close(scheduler.fdp2c[1]); // close write end of parent to child pipe
        close(scheduler.fdc2p[0]); // close read end of child to parent pipe
        // read algorithm name from parent
        int messageLen;
        if (read(scheduler.fdp2c[0], &messageLen, sizeof(messageLen)) < 0)
        {
            perror("error when reading message length from parent");
            exit(EXIT_FAILURE);
        }
        char algorithmName[MAX_METHOD_LEN];
        int n = read(scheduler.fdp2c[0], algorithmName, messageLen);
        if (n < 0)
        {
            perror("error when reading algorithm name from parent");
            exit(EXIT_FAILURE);
        }
        // read period from parent
        struct Period period;
        n = read(scheduler.fdp2c[0], &period, sizeof(period));
        if (n < 0)
        {
            perror("error when reading period from parent");
            exit(EXIT_FAILURE);
        }
        // read orders from parent
        // number of orders to come
        int orderNum;
        if (read(scheduler.fdp2c[0], &orderNum, sizeof(orderNum)) < 0)
        {
            perror("error when reading orderNum from parent");
            exit(EXIT_FAILURE);
        }
        // orders
        struct Order order[orderNum];
        if (read(scheduler.fdp2c[0], order, sizeof(struct Order) * orderNum) < 0)
        {
            perror("error when reading orders from parent");
            exit(EXIT_FAILURE);
        }
        //print orders
        for (int i = 0; i < orderNum; i++)
        {
            printf("Order ID: %d\n", order[i].order_id);
            printf("Order Number: %s\n", order[i].order_number);
            printf("Product Name: %c\n", order[i].product_name);
            printf("Order Quantity: %d\n", order[i].order_quantity);
            printf("Due Date: %d-%d-%d\n", order[i].dueDate.tm_year + 1900, order[i].dueDate.tm_mon + 1, order[i].dueDate.tm_mday);
        }
        // schedule & report
        struct Schedule schedule;
        schedule.schedule_id = 0;
        schedule.period = period;
        struct Report report;

        // reorder the order list according to the algorithm
        if (strcmp(algorithmName, "FCFS") == 0)
        {
            // FCFS
            // sort by order_id, already done
        }
        else if (strcmp(algorithmName, "SJF") == 0)
        {
            // SJF
            // sort by order_quantity
            // bubble sort
            for (int i = 0; i < orderNum - 1; i++)
            {
                for (int j = 0; j < orderNum - i - 1; j++)
                {
                    if (order[j].order_quantity > order[j + 1].order_quantity)
                    {
                        struct Order temp = order[j];
                        order[j] = order[j + 1];
                        order[j + 1] = temp;
                    }
                }
            }
        }
        else if (strcmp(algorithmName, "NOVEL") == 0)
        {
            // NOVEL
            // sort by order_quantity in descending order
            // bubble sort
            for (int i = 0; i < orderNum - 1; i++)
            {
                for (int j = 0; j < orderNum - i - 1; j++)
                {
                    if (order[j].order_quantity < order[j + 1].order_quantity)
                    {
                        struct Order temp = order[j];
                        order[j] = order[j + 1];
                        order[j + 1] = temp;
                    }
                }
            }
        } else {
            fprintf(stderr, "Error: Invalid algorithm name.\n");
            exit(1);
        }

        // tranverse the order list to generate schedule
        int currentX = 0, currentY = 0, currentZ = 0; //current production days of X, Y, Z
        int X_remain = 0, Y_remain = 0, Z_remain = 0; //remaining days of X, Y, Z before due
        int X_total = 0, Y_total = 0, Z_total = 0;    //total quantity of X, Y, Z
        for (int i = 0; i < orderNum; i++)
        {
            X_remain = dateDiff(&period.startDate, &order[i].dueDate) - currentX;
            Y_remain = dateDiff(&period.startDate, &order[i].dueDate) - currentY;
            Z_remain = dateDiff(&period.startDate, &order[i].dueDate) - currentZ;
            // Acceptance Judge
            // deney the order if the three plants cannot produce the product before its due date
            if (300 * X_remain + 400 * Y_remain + 500 * Z_remain < order[i].order_quantity)
            {
                // deney the order
                report.allocation[i].order_id = order[i].order_id;
                report.allocation[i].accepted = 0;
                continue;
            }

            // Allocation Calculation
            int alloc[3] = {0, 0, 0}; // days to assign to X, Y, Z
            int vacancy = allocate(order[i].order_quantity, X_remain, Y_remain, Z_remain, alloc); // which plant has internal fragmentation
            alloc[0] = 2;
            alloc[1] = 2;
            alloc[2] = 2;
            // Report Generation
            // record the allocation for current order
            report.allocation[i].order_id = order[i].order_id;
            report.allocation[i].accepted = 1;
            report.allocation[i].schedule[0][0] = currentX + 1;
            report.allocation[i].schedule[1][0] = currentY + 1;
            report.allocation[i].schedule[2][0] = currentZ + 1;
            report.allocation[i].schedule[0][1] = alloc[0];
            report.allocation[i].schedule[1][1] = alloc[1];
            report.allocation[i].schedule[2][1] = alloc[2];
            report.allocation[i].schedule[0][2] = 0;
            report.allocation[i].schedule[1][2] = 0;
            report.allocation[i].schedule[2][2] = 0;

            // Schedule Generation
            for (int j = 0; j < alloc[0]; j++)
            {
                //Plant X: day[currentX] produce 300 quantity
                schedule.schedule_X[0][++currentX] = order[i].order_id;
                schedule.schedule_X[1][currentX] = 300;
                //current order produced in plant X
                report.allocation[i].schedule[0][2] += 300;
            }
            for (int j = 0; j < alloc[1]; j++)
            {
                //Plant Y: day[currentY] produce 400 quantity
                schedule.schedule_Y[0][++currentY] = order[i].order_id;
                schedule.schedule_Y[1][currentY] = 400;
                //current order produced in plant Y
                report.allocation[i].schedule[1][2] += 400;
            }
            for (int j = 0; j < alloc[2]; j++)
            {
                //Plant Z: day[currentZ] produce 500 quantity
                schedule.schedule_Z[0][++currentZ] = order[i].order_id;
                schedule.schedule_Z[1][currentZ] = 500;
                //current order produced in plant Z
                report.allocation[i].schedule[2][2] += 500;
            }


            // internal fragmentation handling
            int remain = (alloc[0] * 300 + alloc[1] * 400 + alloc[2] * 500) - order[i].order_quantity;
            switch(vacancy) {
                case 0: // no internal fragmentation
                    break;
                case 1: // internal fragmentation exists in X
                    schedule.schedule_X[1][currentX] = 300 - remain;
                    report.allocation[i].schedule[0][2] -= remain;
                    break;
                case 2:
                    schedule.schedule_Y[1][currentY] = 400 - remain;
                    report.allocation[i].schedule[1][2] -= remain;
                    break;
                case 3:
                    schedule.schedule_Z[1][currentZ] = 500 - remain;
                    report.allocation[i].schedule[2][2] -= remain;
                    break;
                default:
                printf("error: invalid vacancy\n");
                    break;
            }

            // update the total quantity of X, Y, Z
            X_total += report.allocation[i].schedule[0][2];
            Y_total += report.allocation[i].schedule[1][2];
            Z_total += report.allocation[i].schedule[2][2];

        }

        // report the total quantity of X, Y, Z
        report.X[0] = currentX;
        report.X[1] = X_total;
        report.Y[0] = currentY;
        report.Y[1] = Y_total;
        report.Z[0] = currentZ;
        report.Z[1] = Z_total;

        //print the schedule and report
        printf("Schedule ID: %d\n", schedule.schedule_id);
        printf("Period: %ld to %ld\n", dateDiff(&period.startDate, &period.endDate));
        for (int i = 0; i < orderNum; i++)
        {
            printf("Order ID: %d\n", report.allocation[i].order_id);
            printf("Accepted: %d\n", report.allocation[i].accepted);
            printf("Plant X: %d days, %d quantity\n", report.allocation[i].schedule[0][1], report.allocation[i].schedule[0][2]);
            printf("Plant Y: %d days, %d quantity\n", report.allocation[i].schedule[1][1], report.allocation[i].schedule[1][2]);
            printf("Plant Z: %d days, %d quantity\n", report.allocation[i].schedule[2][1], report.allocation[i].schedule[2][2]);
        }
        printf("Plant X: %d days, %d quantity\n", report.X[0], report.X[1]);
        printf("Plant Y: %d days, %d quantity\n", report.Y[0], report.Y[1]);
        printf("Plant Z: %d days, %d quantity\n", report.Z[0], report.Z[1]);


        // write the schedule to parent
        if (write(scheduler.fdc2p[1], &schedule, sizeof(schedule)) < 0)
        {
            perror("error when writing schedule to parent");
            exit(EXIT_FAILURE);
        }
        // write the report to parent
        if (write(scheduler.fdc2p[1], &report, sizeof(report)) < 0)
        {
            perror("error when writing report to parent");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // input module now
        close(scheduler.fdp2c[0]);
        close(scheduler.fdc2p[1]);
        // write algorithm name to scheduler
        int messageLength = strlen(algorithm) + 1; // null terminator is also passed
        if (write(scheduler.fdp2c[1], &messageLength, sizeof(messageLength)) < 0)
        {
            perror("error when writing messageLength to scheduler");
            exit(EXIT_FAILURE);
        }
        if (write(scheduler.fdp2c[1], algorithm, messageLength) != messageLength)
        {
            perror("error when writing algorithm name to scheduler");
            exit(EXIT_FAILURE);
        }
        // write period to scheduler
        struct Period period;
        period.startDate = startDate;
        period.endDate = endDate;
        write(scheduler.fdp2c[1], &period, sizeof(period));
        // write orders to scheduler
        // number of orders to come
        if (write(scheduler.fdp2c[1], &orderNum, sizeof(orderNum)) < 0)
        {
            perror("error when writing orderNum to scheduler");
            exit(EXIT_FAILURE);
        }
        // orders
        if (write(scheduler.fdp2c[1], order, sizeof(struct Order) * orderNum) < 0)
        {
            perror("error when writing orders to scheduler");
            exit(EXIT_FAILURE);
        }
    }
    // } else if (scheduler.pid == 0) {
    //     // child
    //     close(scheduler.fdp2c[1]);
    //     close(scheduler.fdc2p[0]);
    //     // read from parent
    //     struct Order order[10];
    //     int n = read(scheduler.fdp2c[0], &order, sizeof(order));
    //     if (n < 0) {
    //         perror("read");
    //         return 1;
    //     }
    //     //// schedule
    //     struct Schedule schedule;
    //     schedule.schedule_id = 0;
    //     schedule.period.start = 0;
    //     schedule.period.end = 0;
    //     for(int i = 0; i < 10; i++) {
    //         schedule.schedule_A[0][i] = order[i].order_id;
    //         schedule.schedule_A[1][i] = order[i].order_quantity;
    //         schedule.schedule_A[2][i] = order[i].due;
    //     }
    //     // write to parent
    //     n = write(scheduler.fdc2p[1], &schedule, sizeof(schedule));
    //     if (n < 0) {
    //         perror("write");
    //         return 1;
    //     }
    //     close(scheduler.fdp2c[0]);
    //     close(scheduler.fdc2p[1]);
    // } else {
    //     // read from child
    //     n = read(scheduler.fdc2p[0], &schedule, sizeof(schedule));
    //     if (n < 0) {
    //         perror("read");
    //         return 1;
    //     }
    //     //print the schedule
    //     printf("Schedule ID: %d\n", schedule.schedule_id);
    //     printf("Period: %ld to %ld\n", schedule.period.start, schedule.period.end);
    //     close(scheduler.fdp2c[1]);
    //     close(scheduler.fdc2p[0]);
    // }

    // // wait for child
    // wait(NULL);
}
void invokeScheduler(const char *str)
{
    if (startsWith(str, "FCFS"))
    {
        puts("run with FCFS algorithm");
        invoke(str);
    }
    else if (startsWith(str, "SJF"))
    {
        puts("run with SJF algorithm");
        invoke(str);
    }
    else if (startsWith(str, "PR"))
    {
        puts("run with PR algorithm");
        invoke(str);
    }
    else if (startsWith(str, "NOVEL"))
    {
        puts("run with NOVEL algorithm");
        invoke(str);
    }
    else
    {
        fprintf(stderr, "Error: Invalid algorithm name.\n");
        exit(1);
    }
}
void invoke(const char *str)
{
}
int main()
{
    inputModule();
    return 0;
}
