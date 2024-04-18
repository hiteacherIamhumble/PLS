#include <stdio.h>

#define MAX_ORDER_NUM 100

// Assuming the definition of the structures are given as:
struct Report {
    struct Allocation allocation[MAX_ORDER_NUM];
    int X[2]; // days, quantity for Plant_X
    int Y[2]; // days, quantity for Plant_Y
    int Z[2]; // days, quantity for Plant_Z
};

struct Allocation {
    int order_id; // order id
    int accepted; // 1 for accepted, 0 for denied
    int schedule[3][3]; // plant_id: days since start date, days to produce in that plant, quantity
};

void writeReport(struct Report report) {
    FILE *file = fopen("report.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Write the header of the report
    fprintf(file, "***PLS Schedule Analysis Report***\n\n");
    fprintf(file, "Algorithm used: XXXXXXXXXXXXXXXX\n\n");
    fprintf(file, "There are XX Orders ACCEPTED. Details are as follows:\n");
    fprintf(file, "ORDER NUMBER START              END                DAYS  QUANTITY  PLANT\n");
    fprintf(file, "=====================================================================\n");

    // Write accepted orders
    for (int i = 0; i < MAX_ORDER_NUM; i++) {
        if (report.allocation[i].accepted == 1) {
            // Assuming we have the dates and plant names available,
            // otherwise placeholder values are used
            fprintf(file, "P%04d       2024-06-DD        2024-06-DD      %d     %d       Plant_X\n",
                    report.allocation[i].order_id,
                    report.allocation[i].schedule[0][1], // Days to produce
                    report.allocation[i].schedule[0][2]); // Quantity
        }
    }

    fprintf(file, "- End -\n\n");

    // Write rejected orders section
    fprintf(file, "There are XX Orders REJECTED. Details are as follows:\n");
    fprintf(file, "ORDER NUMBER  PRODUCT NAME  Due Date    QUANTITY\n");
    fprintf(file, "================================================\n");

    // Write rejected orders
    for (int i = 0; i < MAX_ORDER_NUM; i++) {
        if (report.allocation[i].accepted == 0) {
            fprintf(file, "P%04d         Product_?     2024-06-DD   ????\n",
                    report.allocation[i].order_id);
        }
    }

    fprintf(file, "- End -\n\n");

    // Write performance section
    fprintf(file, "***PERFORMANCE***\n\n");
    fprintf(file, "Plant_X:\n");
    fprintf(file, "Number of days in use: %d days\n", report.X[0]);
    fprintf(file, "Number of products produced: %d (in total)\n", report.X[1]);
    fprintf(file, "Utilization of the plant: %.1f %%\n\n", /* Placeholder for utilization */);

    // Repeat for Plant_Y and Plant_Z

    fprintf(file, "Overall of utilization: %.1f %%\n", /* Placeholder for overall utilization */);

    fclose(file);
}

int main() {
    struct Report report;
    // Here you would have the code to fill the report structure with data
    writeReport(report);

    return 0;
}
