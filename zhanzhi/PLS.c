#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_DATE_LEN 32
struct tm startDate = {0}, endDate = {0};
char startDateStr[MAX_DATE_LEN], endDateStr[MAX_DATE_LEN]; // start date and end date for the period
#define MAX_ALGORITHMNAME_LEN 20
char algorithmName[MAX_ALGORITHMNAME_LEN];// name of the current algorithm for schedule module
#define MAX_FILENAME_LEN 50
char reportFileName[MAX_FILENAME_LEN]; // name of the report file for analysis report

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
#define MAX_ORDER_NUM 200
int orderNum = 0;
struct Order order[MAX_ORDER_NUM];

struct Allocation
{
    // one of this struct shows the allocation of one order
    int order_id; // order id, initialized to -1
    int accepted; // 1 for accepted, 0 for denied, initialized to -1
    // first dimension is plant x at 0, plant y at 1, plant z at 2
    // second demension is days since start date at 0, days to produce in that plant at 1, quantity at 2
    // initialized to -1
    int schedule[3][3]; //plant_id: days since start date, days to produce in that plant, quantity
};

struct Schedule
{
    int schedule_id; //(internal use)
    struct Period period;
    // first dimension is order id at 0 and quantity at 1, initialized to -1
    // second dimension is the day starting from 0 which is the start date
    // if not job that day, order id is -1, quantity is not specified
    int schedule_X[2][MAX_ORDER_NUM]; // order_id, quantity
    int schedule_Y[2][MAX_ORDER_NUM]; // order_id, quantity
    int schedule_Z[2][MAX_ORDER_NUM]; // order_id, quantity
};

struct Report
{
    // this is uesd in the analysis report
    // for each order, there is a allocation struct
    // assume that the order id is the same as the index in the array
    struct Allocation allocation[MAX_ORDER_NUM];
    // number of days in use at 0, number of quantity produced at 1, initialized to -1
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
int addOrder(const char *str);
void addBatch(const char *str);
void runPLS(const char *str);
void exitPLS();
void work(const char *algorithm, const char *filename);
void parseInput(const char *str);
void inputModule();
void invokeScheduler(const char *str);
void invoke(const char *str);
int dateDiff(const struct tm *startDate, const struct tm *endDate);
void writeReport(struct Report *report);

int kernel(int Q, int Rx, int Ry, int Rz, int alloc[3]) {
    
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

    int re = alloc[0] * 300 + alloc[1] * 400 + alloc[2] * 500 - Q;
    //find which plant where the vacancy from
    if (re == 0) {
        return 0;
    } else if (re >= 400 && alloc[2] > 0) {
        return 3;
    } else if (re >=300 && alloc[1] > 0) {
        return 2;
    } else if (re >= 300 && alloc[2] > 0) {
        return 3;
    } else if (alloc[0] > 0) {
        return 1;
    } else if (alloc[1] > 0) {
        return 2;
    } else if (alloc[2] > 0) {
        return 3;
    } else {
        return -1;
    }
}
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

int dateDiff(const struct tm *startDate, const struct tm *endDate)
{
    time_t start = mktime((struct tm *)startDate);
    time_t end = mktime((struct tm *)endDate);
    if (start == -1 || end == -1)
    {
        perror("mktime");
        exit(1);
    }
    double diff = difftime(end, start);
    return (int)diff / (60 * 60 * 24);
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
    // char startDateStr[MAX_DATE_LEN];
    // char endDateStr[MAX_DATE_LEN];
    sscanf(str, "addPERIOD %s %s", startDateStr, endDateStr);
    // printf("start date is %s\n", startDateStr);
    // printf("end date is %s\n", endDateStr);
    startDate = str2Date(startDateStr);
    endDate = str2Date(endDateStr);
    // printf("start date is %d-%d-%d\n", startDate.year, startDate.month, startDate.day);
    // printf("start date is %d-%d-%d\n", endDate.year, endDate.month, endDate.day);
}
const char INVALID_INPUTS[] = "InvalidInputs.txt";
void appendToInvalidFile(const char *str){
    FILE *filePtr;
    // Open the file in append mode
    filePtr = fopen(INVALID_INPUTS, "a");
    if (filePtr == NULL) {
        perror("Error opening file");
        return;
    }

    // Append the line to the file
    if (fprintf(filePtr, "%s\n", str) < 0) {
        perror("Error writing to file");
        // Close the file before returning
        fclose(filePtr);
        return;
    }

    // Close the file
    fclose(filePtr);
}
int CheckDueDate(struct tm orderDueDate){
    // check if that due date is within the period
    // if is, return 1
    // if not, return 0
    if (dateDiff(&startDate, &orderDueDate) < 0 || dateDiff(&endDate, &orderDueDate) > 0)
    {
        return 0;
    }
    return 1;

}
int addOrder(const char *str)
{
    // str is a addORDER command.
    char dueDateStr[MAX_DATE_LEN];
    sscanf(str, "addORDER %s %s %d Product_%c", order[orderNum].order_number, dueDateStr, &order[orderNum].order_quantity, &order[orderNum].product_name);
    struct tm orderDueDate = str2Date(dueDateStr);
    int n = CheckDueDate(orderDueDate);
    if ( n == 0 ){
        appendToInvalidFile(str);
        return 1;
    }
    order[orderNum].order_id = orderNum;
    order[orderNum].dueDate = str2Date(dueDateStr);
    ++orderNum;
    return 0;
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
    int invalid = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        // Remove newline character if present
        buffer[strcspn(buffer, "\n")] = 0;
        //printf("You entered: %s\n", buffer);
        int p = addOrder(buffer);
        if (p == 1) {
            invalid = 1;
        }
    }
    if (invalid == 1) {
       printf("Some due dates are out of period, deemed invalid input, saved these lines to file InvalidInputs.txt.\n");
    }
}

void runPLS(const char *str)
{
    sscanf(str, "runPLS %s | printREPORT > %s", algorithmName, reportFileName);
    //printf("use algorithm %s, report file to %s\n", algorithmName, reportFileName);
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
        //puts("This is a addPERIOD command.");
        addPeriod(str);
    }
    else if (startsWith(str, "addORDER"))
    {
        //puts("This is a addORDER command.");
        addOrder(str);
    }
    else if (startsWith(str, "addBATCH"))
    {
        //puts("This is a addBATCH command.");
        addBatch(str);
    }
    else if (startsWith(str, "runPLS"))
    {
        //puts("This is a runPLS command.");
        runPLS(str);
    }
    else if (startsWith(str, "exitPLS"))
    {
        //puts("This is a exitPLS command.");
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
            //("You entered: %s\n", buffer);
            parseInput(buffer);
        }
    }
}
struct tm calcDate(struct tm date, int offset){
    //offset is the number of days to add to date
    //result is the date after adding offset days
    struct tm result = date;
    result.tm_mday += offset;
    mktime(&result);
    return result;
}
void dateToStr(struct tm date, char *str){
    //convert date to string
    strftime(str, MAX_DATE_LEN, "%Y-%m-%d", &date);
}
int diffDate(struct tm date1, struct tm date2){
    //date1 is the earlier date
    //date2 is the later date
    //return the difference in days between date1 and date2
    int diff = dateDiff(&date1, &date2);
    return diff;
}
void printSchedule(struct Schedule schedule){
    //first print plant x
    printf("=====================================================================================================\n");
    printf("Plant_X (300 per day)\n");
    printf("%s to %s\n", startDateStr, endDateStr);
    printf("\n");
    //print name of each column
    //first column has a width of 14 character
    //second column has a width of 16 character
    //third column has a width of 16 character
    //fourth column has a width of 22 character
    //fifth column has a width of 12 character
    printf("%14s%16s%16s%22s%12s\n", "Date", "Product Name", "Order Number", "Quantity (Produced)", "Due Date");
    printf("=====================================================================================================\n");
    int totalDays = diffDate(startDate, endDate)+1;//including the start date
    for(int i = 0; i < totalDays; i++) {// go over each day
        char todayDateStr[MAX_DATE_LEN];
        struct tm todayDate = calcDate(startDate, i);
        dateToStr(todayDate, todayDateStr);
        if(schedule.schedule_X[0][i] == -1) {// no order on this day
            printf("%14s%16s\n", todayDateStr, "NA");
            continue;
        }
        int orderID = schedule.schedule_X[0][i];
        int quantity = schedule.schedule_X[1][i];
        char productName[10] = "Product_";
        char temp[2] = {order[orderID].product_name, '\0'};
        strcat(productName, temp);
        char ordreDueDateStr[MAX_DATE_LEN];
        dateToStr(order[orderID].dueDate, ordreDueDateStr);
        printf("%14s%16s%16s%22d%12s\n", todayDateStr, productName, order[orderID].order_number, quantity, ordreDueDateStr);
    }
    printf("\n");
    printf("=====================================================================================================\n");
    printf("\n");
    //then print plant y
    printf("Plant_Y (400 per day)\n");
    printf("%s to %s\n", startDateStr, endDateStr);
    printf("\n");
    printf("%14s%16s%16s%22s%12s\n", "Date", "Product Name", "Order Number", "Quantity (Produced)", "Due Date");
    printf("=====================================================================================================\n");
    for(int i = 0; i < totalDays; i++) {// go over each day
        char todayDateStr[MAX_DATE_LEN];
        struct tm todayDate = calcDate(startDate, i);
        dateToStr(todayDate, todayDateStr);
        if(schedule.schedule_Y[0][i] == -1) {// no order on this day
            printf("%14s%16s\n", todayDateStr, "NA");
            continue;
        }
        int orderID = schedule.schedule_Y[0][i];
        int quantity = schedule.schedule_Y[1][i];
        char productName[10] = "Product_";
        char temp[2] = {order[orderID].product_name, '\0'};
        strcat(productName, temp);
        char ordreDueDateStr[MAX_DATE_LEN];
        dateToStr(order[orderID].dueDate, ordreDueDateStr);
        printf("%14s%16s%16s%22d%12s\n", todayDateStr, productName, order[orderID].order_number, quantity, ordreDueDateStr);
    }
    printf("\n");
    printf("=====================================================================================================\n");
    printf("\n");
    //then print plant z
    printf("Plant_Z (500 per day)\n");
    printf("%s to %s\n", startDateStr, endDateStr);
    printf("\n");
    printf("%14s%16s%16s%22s%12s\n", "Date", "Product Name", "Order Number", "Quantity (Produced)", "Due Date");
    printf("=====================================================================================================\n");
    for(int i = 0; i < totalDays; i++) {// go over each day
        char todayDateStr[MAX_DATE_LEN];
        struct tm todayDate = calcDate(startDate, i);
        dateToStr(todayDate, todayDateStr);
        if(schedule.schedule_Z[0][i] == -1) {// no order on this day
            printf("%14s%16s\n", todayDateStr, "NA");
            continue;
        }
        int orderID = schedule.schedule_Z[0][i];
        int quantity = schedule.schedule_Z[1][i];
        char productName[10] = "Product_";
        char temp[2] = {order[orderID].product_name, '\0'};
        strcat(productName, temp);
        char ordreDueDateStr[MAX_DATE_LEN];
        dateToStr(order[orderID].dueDate, ordreDueDateStr);
        printf("%14s%16s%16s%22d%12s\n", todayDateStr, productName, order[orderID].order_number, quantity, ordreDueDateStr);
    }
    printf("\n");
    printf("=====================================================================================================\n");
    printf("\n");
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
        close(scheduler.fdp2c[1]); // close write end of parent to child pipe
        close(scheduler.fdc2p[0]); // close read end of child to parent pipe
        // read algorithm name from parent
        int messageLen;
        if (read(scheduler.fdp2c[0], &messageLen, sizeof(messageLen)) < 0)
        {
            perror("error when reading message length from parent");
            exit(EXIT_FAILURE);
        }
        char algorithmName[MAX_ALGORITHMNAME_LEN];
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
        struct Order order[3][orderNum];
        if (read(scheduler.fdp2c[0], order[0], sizeof(struct Order) * orderNum) < 0)
        {
            perror("error when reading orders from parent");
            exit(EXIT_FAILURE);
        }
        // make two order list copys
        for (int i = 0; i < orderNum; i++)
        {
            order[1][i] = order[0][i];
            order[2][i] = order[0][i];
        }
        // schedule & report
        // initialize schedule
        struct Schedule schedule[3];
        for (int m = 0; m < 3; ++ m) {
            schedule[m].schedule_id = 0;
            schedule[m].period = period;
             for (int i = 0; i < MAX_ORDER_NUM; i++)
            {
                schedule[m].schedule_X[0][i] = -1;
                schedule[m].schedule_X[1][i] = -1;
                schedule[m].schedule_Y[0][i] = -1;
                schedule[m].schedule_Y[1][i] = -1;
                schedule[m].schedule_Z[0][i] = -1;
                schedule[m].schedule_Z[1][i] = -1;
            }
        }
        
       
        // initialize report
        struct Report report[3];
        
        for (int m =0; m < 3; m++) {
            for (int i = 0; i < MAX_ORDER_NUM; i++) {
            report[m].allocation[i].order_id = -1;
            report[m].allocation[i].accepted = -1;
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    report[m].allocation[i].schedule[j][k] = -1;
                    report[m].allocation[i].schedule[j][k] = -1;
                    report[m].allocation[i].schedule[j][k] = -1;
                }
            }
            }
        }
        

        // SJF
        for (int i = 0; i < orderNum - 1; i++)
        {
            for (int j = 0; j < orderNum - i - 1; j++)
            {
                if (order[1][j].order_quantity > order[1][j + 1].order_quantity)
                {
                    struct Order temp = order[1][j];
                    order[1][j] = order[1][j + 1];
                    order[1][j + 1] = temp;
                }
            }
        }
        
        // NOVEL
        for (int i = 0; i < orderNum - 1; i++)
        {
            for (int j = 0; j < orderNum - i - 1; j++)
            {
                if (order[2][j].order_quantity < order[2][j + 1].order_quantity)
                {
                    struct Order temp = order[2][j];
                    order[2][j] = order[2][j + 1];
                    order[2][j + 1] = temp;
                }
            }
        }
        
        for (int m = 0; m < 3; m ++) {
            // tranverse the order list to generate schedule
            int currentX = 0, currentY = 0, currentZ = 0; //current production days of X, Y, Z
            int X_remain = 0, Y_remain = 0, Z_remain = 0; //remaining days of X, Y, Z before due
            int X_total = 0, Y_total = 0, Z_total = 0;    //total quantity of X, Y, Z
            for (int i = 0; i < orderNum; i++)
            {
                int id = order[m][i].order_id;
                X_remain = dateDiff(&period.startDate, &order[m][i].dueDate) - currentX;
                Y_remain = dateDiff(&period.startDate, &order[m][i].dueDate) - currentY;
                Z_remain = dateDiff(&period.startDate, &order[m][i].dueDate) - currentZ;
                // Acceptance Judge
                // deney the order if the three plants cannot produce the product before its due date
                if (300 * X_remain + 400 * Y_remain + 500 * Z_remain < order[m][id].order_quantity)
                {
                    // deney the order
                    report[m].allocation[id].order_id = id;
                    report[m].allocation[id].accepted = 0;
                    continue;
                }
                X_remain = 0 > X_remain ? 0 : X_remain;
                Y_remain = 0 > Y_remain ? 0 : Y_remain;
                Z_remain = 0 > Z_remain ? 0 : Z_remain;
                // Allocation Calculation
                int alloc[3] = {0, 0, 0}; // days to assign to X, Y, Z
                //int vacancy = allocate(order[i].order_quantity, X_remain, Y_remain, Z_remain, alloc); // which plant has internal fragmentation
                int vacancy;
                vacancy = kernel(order[m][i].order_quantity, X_remain, Y_remain, Z_remain, alloc); // which plant has internal fragmentation
                
                
                if (vacancy == -1)
                {
                    printf("error: invalid vacancy\n");
                    exit(1);
                }

                // Report Generation
                // record the allocation for current order
                report[m].allocation[id].order_id = id;
                report[m].allocation[id].accepted = 1;
                report[m].allocation[id].schedule[0][0] = currentX;
                report[m].allocation[id].schedule[1][0] = currentY;
                report[m].allocation[id].schedule[2][0] = currentZ;
                report[m].allocation[id].schedule[0][1] = alloc[0];
                report[m].allocation[id].schedule[1][1] = alloc[1];
                report[m].allocation[id].schedule[2][1] = alloc[2];
                report[m].allocation[id].schedule[0][2] = 0;
                report[m].allocation[id].schedule[1][2] = 0;
                report[m].allocation[id].schedule[2][2] = 0;

                // Schedule Generation
                for (int j = 0; j < alloc[0]; j++)
                {
                    //Plant X: day[currentX] produce 300 quantity
                    schedule[m].schedule_X[0][currentX] = id;
                    schedule[m].schedule_X[1][currentX] = 300;
                    //current order produced in plant X
                    report[m].allocation[id].schedule[0][2] += 300;
                    ++currentX;
                }
                for (int j = 0; j < alloc[1]; j++)
                {
                    //Plant Y: day[currentY] produce 400 quantity
                    schedule[m].schedule_Y[0][currentY] = id;
                    schedule[m].schedule_Y[1][currentY] = 400;
                    //current order produced in plant Y
                    report[m].allocation[id].schedule[1][2] += 400;
                    ++currentY;
                }
                for (int j = 0; j < alloc[2]; j++)
                {
                    //Plant Z: day[currentZ] produce 500 quantity
                    schedule[m].schedule_Z[0][currentZ] = id;
                    schedule[m].schedule_Z[1][currentZ] = 500;
                    //current order produced in plant Z
                    report[m].allocation[id].schedule[2][2] += 500;
                    ++currentZ;
                }
                // internal fragmentation handling
                int remain = (alloc[0] * 300 + alloc[1] * 400 + alloc[2] * 500) - order[m][i].order_quantity;
                switch(vacancy) {
                    case 0: // no internal fragmentation
                        break;
                    case 1: // internal fragmentation exists in X
                        schedule[m].schedule_X[1][currentX - 1] = 300 - remain;
                        report[m].allocation[id].schedule[0][2] -= remain;
                        break;
                    case 2:
                        schedule[m].schedule_Y[1][currentY - 1] = 400 - remain;
                        report[m].allocation[id].schedule[1][2] -= remain;
                        break;
                    case 3:
                        schedule[m].schedule_Z[1][currentZ - 1] = 500 - remain;
                        report[m].allocation[id].schedule[2][2] -= remain;
                        break;
                    default:
                    printf("error: invalid vacancy\n");
                        break;
                }

                // update the total quantity of X, Y, Z
                X_total += report[m].allocation[id].schedule[0][2];
                Y_total += report[m].allocation[id].schedule[1][2];
                Z_total += report[m].allocation[id].schedule[2][2];

            }

            // report the total quantity of X, Y, Z
            report[m].X[0] = currentX;
            report[m].X[1] = X_total;
            report[m].Y[0] = currentY;
            report[m].Y[1] = Y_total;
            report[m].Z[0] = currentZ;
            report[m].Z[1] = Z_total;

        }

        if (strcmp(algorithmName, "FCFS") == 0) {
            // write the schedule to parent
            if (write(scheduler.fdc2p[1], &schedule[0], sizeof(struct Schedule)) < 0)
            {
                perror("error when writing schedule to parent");
                exit(EXIT_FAILURE);
            }
            // write the report to parent
            if (write(scheduler.fdc2p[1], &report[0], sizeof(struct Report)) < 0)
            {
                perror("error when writing report to parent");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(algorithmName, "SJF") == 0) {
            // write the schedule to parent
            if (write(scheduler.fdc2p[1], &schedule[1], sizeof(struct Schedule)) < 0)
            {
                perror("error when writing schedule to parent");
                exit(EXIT_FAILURE);
            }
            // write the report to parent
            if (write(scheduler.fdc2p[1], &report[1], sizeof(struct Report)) < 0)
            {
                perror("error when writing report to parent");
                exit(EXIT_FAILURE);
            }
        } else {
            int w = 0;
            float s = 0;
            for (int m = 0; m < 3; m++) {
                if(report[m].X[1] + report[m].Y[1] + report[m].Z[1] >= s) {
                    w = m;
                    s = report[m].X[1] + report[m].Y[1] + report[m].Z[1];
                }
            }
            // write the schedule to parent
            if (write(scheduler.fdc2p[1], &schedule[w], sizeof(struct Schedule)) < 0)
            {
                perror("error when writing schedule to parent");
                exit(EXIT_FAILURE);
            }
            // write the report to parent
            if (write(scheduler.fdc2p[1], &report[w], sizeof(struct Report)) < 0)
            {
                perror("error when writing report to parent");
                exit(EXIT_FAILURE);
            }
        }
        // close the pipe
        close(scheduler.fdp2c[0]);
        close(scheduler.fdc2p[1]);
        exit(0);
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
        // read schedule from scheduler
        struct Schedule schedule;
        // initialize schedule
        schedule.schedule_id = 0;
        for (int i = 0; i < MAX_ORDER_NUM; i++)
        {
            schedule.schedule_X[0][i] = -1;
            schedule.schedule_X[1][i] = -1;
            schedule.schedule_Y[0][i] = -1;
            schedule.schedule_Y[1][i] = -1;
            schedule.schedule_Z[0][i] = -1;
            schedule.schedule_Z[1][i] = -1;
        }
        if (read(scheduler.fdc2p[0], &schedule, sizeof(struct Schedule)) < 0)
        {
            perror("error when reading schedule from scheduler");
            exit(EXIT_FAILURE);
        }
        // read report from scheduler
        struct Report report;
        // initlaize report
        for (int i = 0; i < MAX_ORDER_NUM; i++)
        {
            report.allocation[i].order_id = -1;
            report.allocation[i].accepted = -1;
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    report.allocation[i].schedule[j][k] = -1;
                }
            }
        }
        if (read(scheduler.fdc2p[0], &report, sizeof(struct Report)) < 0)
        {
            perror("error when reading report from scheduler");
            exit(EXIT_FAILURE);
        }

        //close the pipe
        close(scheduler.fdp2c[1]);
        close(scheduler.fdc2p[0]);
        // wait for the scheduler process to terminate, then proceed
        int status;

        waitpid(scheduler.pid, &status, 0);
        // print the schedule to console
        printSchedule(schedule);
        // write the report to file
        writeReport(&report);
    }
}
int calcAccepted(struct Report *report)
{
    // calculate the number of accepted orders
    int accepted = 0;
    for (int i = 0; i < orderNum; i++)
    {
        if (report->allocation[i].accepted == 1)
        {
            accepted++;
        }
    }
    return accepted;
}
void writeReport(struct Report *report)
{
    int totalDays = diffDate(startDate, endDate) + 1;   
    FILE *file = fopen(reportFileName, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }
    fprintf(file, "***PLS Schedule Analysis Report***\n\n");
    fprintf(file, "Algorithm used: %s\n\n", algorithmName);
    int accepted = calcAccepted(report);
    int rejected = orderNum - accepted;
    // first print accepted orders
    fprintf(file, "There are %d Orders ACCEPTED. Details are as follows:\n\n", accepted);
    // print column names
    // first column is order number, width 16
    // second column is start date, width 14
    // third column is end date, width 14
    // fourth column is days, width 8
    // fifth column is quantity, width 12
    // sixth column is plant, width 9
    fprintf(file, "%16s%14s%14s%8s%12s%9s\n", "ORDER NUMBER", "START", "END", "DAYS", "QUANTITY", "PLANT");
    fprintf(file, "=====================================================================\n");
    for (int i = 0; i < orderNum; i++)
    {
        if (report->allocation[i].accepted == 0)
        {
            continue;
        }
        if (report->allocation[i].accepted == -1)
        {
            perror("Error: allocation not done.");
            exit(1);
        }
        
        // first print its schedule at plant x
        if (report->allocation[i].schedule[0][1] > 0){
            struct tm orderStartDate = calcDate(startDate, report->allocation[i].schedule[0][0]);
            char orderStartDateStr[MAX_DATE_LEN];
            dateToStr(orderStartDate, orderStartDateStr);
            struct tm orderEndDate = calcDate(orderStartDate, report->allocation[i].schedule[0][1] - 1);
            char orderEndDateStr[MAX_DATE_LEN];
            dateToStr(orderEndDate, orderEndDateStr);
            fprintf(file, "%16s%14s%14s%8d%12d%9s\n", order[i].order_number, orderStartDateStr, orderEndDateStr, report->allocation[i].schedule[0][1], report->allocation[i].schedule[0][2], "Plant_X");
        }
        // then print its schedule at plant y
        if (report->allocation[i].schedule[1][1] > 0){
            struct tm orderStartDate = calcDate(startDate, report->allocation[i].schedule[1][0]);
            char orderStartDateStr[MAX_DATE_LEN];
            dateToStr(orderStartDate, orderStartDateStr);
            struct tm orderEndDate = calcDate(orderStartDate, report->allocation[i].schedule[1][1] - 1);
            char orderEndDateStr[MAX_DATE_LEN];
            dateToStr(orderEndDate, orderEndDateStr);
            fprintf(file, "%16s%14s%14s%8d%12d%9s\n", order[i].order_number, orderStartDateStr, orderEndDateStr, report->allocation[i].schedule[1][1], report->allocation[i].schedule[1][2], "Plant_Y");
        }
        // finally print its schedule at plant z
        if (report->allocation[i].schedule[2][1] > 0){
            struct tm orderStartDate = calcDate(startDate, report->allocation[i].schedule[2][0]);
            char orderStartDateStr[MAX_DATE_LEN];
            dateToStr(orderStartDate, orderStartDateStr);
            struct tm orderEndDate = calcDate(orderStartDate, report->allocation[i].schedule[2][1] - 1);
            char orderEndDateStr[MAX_DATE_LEN];
            dateToStr(orderEndDate, orderEndDateStr);
            fprintf(file, "%16s%14s%14s%8d%12d%9s\n", order[i].order_number, orderStartDateStr, orderEndDateStr, report->allocation[i].schedule[2][1], report->allocation[i].schedule[2][2], "Plant_Z");
        }
    }
    fprintf(file, "- End -\n\n");
    fprintf(file, "=====================================================================\n");
    fprintf(file, "\n\n");
    // then print rejected orders
    fprintf(file, "There are %d Orders REJECTED. Details are as follows:\n\n", rejected);
    // print column names
    // first column is order number, width 16
    // second column is product name, width 11
    // third column is due date, width 14
    // fourth column is quantity, width 12
    fprintf(file, "%16s%11s%14s%12s\n", "ORDER NUMBER", "PRODUCT", "DUE DATE", "QUANTITY");
    fprintf(file, "=====================================================\n");
    for (int i = 0; i < orderNum; i++)
    {
        if (report->allocation[i].accepted == 1)
        {
            continue;
        }
        if (report->allocation[i].accepted == -1)
        {
            perror("Error: allocation not done.");
            exit(1);
        }
        char orderProductName[10] = "Product_";
        char temp[2] = {order[i].product_name, '\0'};
        strcat(orderProductName, temp);
        char orderDueDateStr[MAX_DATE_LEN];
        dateToStr(order[i].dueDate, orderDueDateStr);
        fprintf(file, "%16s%11s%14s%12d\n", order[i].order_number, orderProductName, orderDueDateStr, order[i].order_quantity);
    }
    fprintf(file, "- End -\n\n");
    fprintf(file, "=====================================================\n");
    fprintf(file, "\n\n");
    // now write the performance analysis
    fprintf(file, "***PERFORMANCE\n\n");
    // first print the performance of plant x
    fprintf(file, "Plant_X\n");
    // the bench marks are indented by 4 spaces
    // the data are right aligned with width 10
    fprintf(file, "       %-35s%10d days\n", "Number of days in use:", report->X[0]);
    fprintf(file, "       %-35s%10d (in total)\n", "Number of products produced:", report->X[1]);
    fprintf(file, "       %-35s%10.2f %%\n", "Utilization of the plant:", (double)report->X[1] / (totalDays * 300.0) * 100);
    fprintf(file, "\n");
    // then print the performance of plant y
    fprintf(file, "Plant_Y\n");
    fprintf(file, "       %-35s%10d days\n", "Number of days in use:", report->Y[0]);
    fprintf(file, "       %-35s%10d (in total)\n", "Number of products produced:", report->Y[1]);
    fprintf(file, "       %-35s%10.2f %%\n", "Utilization of the plant:", (double)report->Y[1] / (totalDays * 400.0) * 100);
    fprintf(file, "\n");
    // finally print the performance of plant z
    fprintf(file, "Plant_Z\n");
    fprintf(file, "       %-35s%10d days\n", "Number of days in use:", report->Z[0]);
    fprintf(file, "       %-35s%10d (in total)\n", "Number of products produced:", report->Z[1]);
    fprintf(file, "       %-35s%10.2f %%\n", "Utilization of the plant:", (double)report->Z[1] / (totalDays * 500.0) * 100);
    fprintf(file, "\n");
    // overall performance
    fprintf(file, "%-42s%10.2f %%\n", "Overall of utilization:", (double)(report->X[1] + report->Y[1] + report->Z[1]) / (totalDays * 1200.0) * 100);
    // fprintf(file, "Overall of utilization: %.2f %%\n", (double)(report->X[1] + report->Y[1] + report->Z[1]) / (totalDays * 1200) * 100);
    fclose(file);
}


void init(){
    //clear the content of the file for invalid inputs
    FILE *filePtr;
    // Open the file in append mode
    filePtr = fopen(INVALID_INPUTS, "w");
    if (filePtr == NULL) {
        perror("Error opening file");
        return;
    }
    // Close the file
    fclose(filePtr);
}
int main()
{
    init();
    inputModule();
    return 0;
}
