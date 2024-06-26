Project Title: Steel-making Production Line Scheduler (PLS)

Group Members:
- LIN Zhanzhi (Student ID: 22097456D)
- ZHANG Qinye (Student ID: 22098835D)
- DAI Yuhang (Student ID: 22097845D)
- CUI Zhaoyu (Student ID: 22102947D)
- KONG Zirui (Student ID: 22103493D)

----------------------------------------------------------

-Overview
The Product Line Scheduler (PLS) is a C program project designed to assist manufacturing plants in scheduling their production lines efficiently. It takes into account various factors such as order deadlines, production capacity, and resource availability to optimize the scheduling process. This tool aims to improve throughput and minimize downtime in production environments, and it contains all the necessary information to run the Steel-making Production Line Scheduler (PLS) program.

-Features
Order Input: Add one or more production orders.
Schedule Optimization: Automatically calculates the optimal production schedule.
Resource Allocation: Manages and allocates plants effectively based on current and projected orders.
Reporting: Generates reports on production efficiency and order status.

-System Requirements
Linux Operating System (Ubuntu 20.04 LTS recommended)
GCC Compiler (version 9.3.0 or higher)

-Prerequisites
Before you begin the installation, make sure you have the GCC compiler installed on your Linux system.
The "invalidInputs" file is generated by the error-handling moduele in the PLS_G20.c.
You can install GCC using the following command:

sudo apt update
sudo apt install build-essential

-Cloning the Repository
Clone the repository to your local machine using the following command:

git clone https://github.com/hiteacherIamhumble/PLS.git
cd PLS

-Building the Project
To build the Product Line Scheduler, ensure that your compiler supports the C99 standard. Run the following commands in the project directory:

gcc PLS.c -o PLS -std=c99

-Usage
To run the Product Line Scheduler, use the following command from the src directory:

./pls

-Adding an Order
To add an order, you should input specifying the product number, required deadline, quantity, and the product name.

addORDER P0XX 20XX-XX-XX XXXX Product_X

or you can input a batch file and PLS can handle this file automatically

addBATCH orderBATCHXX.dat

-Viewing Reports
Reports can be viewed by using "pico" command.

pico report_0X_XXXX.txt

-Use LaTeX Document
Because of our environmental configuration, you should install the following packets:
pdfLaTeX, MakeIndex, BibTeX

-Contact
For any queries, please open an issue on the GitHub repository, or connect us by the E-mail above.