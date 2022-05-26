typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#include <limits.h>
#define NULL ((void*)0)

//declaring a struct for the idt
struct idt_entry 
{
    uint16_t base_low16;
    uint16_t selector;
    uint8_t always0;
    uint8_t access;
    uint16_t base_hi16;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

//struct for idtr and lidtr
struct idtr 
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idtr idtr_t;

struct pcb
{
    uint32_t esp;
    uint32_t pid;
    uint32_t priority;
    struct pcb *next;
} __attribute__((packed));
typedef struct pcb pcb_t;

#define QCAPACITY 7
pcb_t PCBs[QCAPACITY];



//function prototypes
//program 1 functions
void k_clearscr();
void print_border(int start_row, int start_col, int end_row, int end_col);
void print_long_line(int start_, int start_col, int length);

//asm function
void k_print(char *string, int string_length, int row, int col);

//program 2 functions
void defaultHandler();
void lidtr(idtr_t* idtr);
void init_idt_entry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access);
void init_idt();
int createProcess(uint32_t processEntry, uint32_t priority);
void enqueue(pcb_t *pcb);
pcb_t *dequeue();
pcb_t *pcbAllocate();
uint32_t *stackAllocate();
void idleProcess();
void p1();
void p2();
void p3();
//void p4();
//void p5();
void goToNext(uint32_t process, uint32_t priority, char* processName, int nameSize);

//asm functions for program 2
void dispatch();
void go();

//functions for program 3
void setupPIC();
void outportb(uint16_t port, uint8_t value);
void init_timer_dev(int);

//functions for program 4
void enqueue_priority(pcb_t *pcb);
void idle();

//variables
idt_entry_t idt[256];
idtr_t idtr;
int stackSpace[1024*10];
int stackSize = 1024*10;
int userStackSize = 1024;
int currentStack = 0;
int nextToRun = 0;
unsigned int retval = 0;
int numProcesses = 0;
int gloRow;
pcb_t* queue;

pcb_t* running;

//our main
int main()
{
    gloRow = 1;
    k_clearscr();
    print_border(0,0,24,79);
    k_print("Running processes", 17, 1, gloRow);
    gloRow++;
    init_idt();
    setup_PIC();
    init_timer_dev(50);

    //round robin structures
    queue = NULL;
    
    goToNext(p1, 10, "p1", 2);
    goToNext(p2, 10, "p2", 2);
    goToNext(p3, 15, "p3", 2);
    goToNext(idle, 5, "idle", 4);
    //now begin running the first process
    
    go();
    k_print("check", 5, 5, 5);

    while(1)
    {

    }
    return 0;
}

int convertNumHeader(uint32_t number, char buffer[])
{
    if(number == 0)
    {
        return 0;
    }

    int index = convertNumHeader(number / 10, buffer);
    buffer[index] = number % 10 + '0';
    buffer[index+1] = '\0';
    return index + 1;
}

void convertNum(unsigned int number, char buffer[])
{
    if(number == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
    }

    else
    {
        convertNumHeader(number, buffer);
    }
}



//program 1 functions
//clear the screen
void k_clearscr()
{
    for(int i = 0; i < 25; i++)
    {
        k_print("                                                                                ", 80, i, 0);
    }
}


//print a really long line across the top and bottom of screen
void print_long_line(int start_row, int start_col, int length)
{
    int temp_col = start_col;
    k_print("+", 1, start_row, temp_col);
    temp_col++;
    for(int i = 0; i < (length -2); i++)
    {
        k_print("-", 1, start_row, temp_col);
        temp_col++;
    }
    k_print("+", 1, start_row, temp_col);
}


//print the border around the entire screen
void print_border(int start_row, int start_col, int end_row, int end_col)
{
    int width = end_col - start_col +1;
    int height = end_row - start_row +1;

    print_long_line(start_row, start_col, width);

    int temp_row = start_row + 1;
    for(int i = 0; i < (height-2); i++)
    {
        k_print("|", 1, temp_row, start_col);
        k_print("|", 1, temp_row, end_col);
        temp_row++;
    }
    print_long_line(end_row, start_col, width);
}


//program 2 functions

//default handler for entries 0-31
void defaultHandler()
{
    k_print("ERROR", 5, 0, 0);
    while(1);
}

void idleProcess()
{
    while(1)
    {

    }
}

pcb_t *pcbAllocate()
{
    return &PCBs[nextToRun++];
}

uint32_t* stackAllocate()
{
    int* temp = stackSpace + currentStack;
    currentStack += 1024;
    return temp;
}

//defining the idt 
void init_idt()
{
    
    //for entries 0 to 31, set these to point to the default handler
    for(int i = 0; i < 32; i++)
    {
        init_idt_entry(idt+i, defaultHandler, 16, 0x8e);
    }

    //for entry 32, set to point to dispatcher
    init_idt_entry(idt+32, dispatch, 16, 0x8e);

    //for entry 33 to 255, set to point to 0
    for(int i = 33; i < 256; i++)
    {
        init_idt_entry(idt+i, 0, 0, 0);
    }
    
    //calling lidtr(idtr)
    idtr.limit = (sizeof(idt_entry_t) *256) -1;
    idtr.base = (uint32_t)idt;
    lidtr(&idtr);
}

//the entries in the idt table
void init_idt_entry(idt_entry_t *entry, uint32_t base, uint16_t selector, uint8_t access)
{
    entry -> base_low16 = base & 0xFFFF;
    entry -> base_hi16 = (base >> 16) & 0xFFFF;
    entry -> selector = selector;
    entry -> always0 = 0;
    entry -> access = access;
}

// void enqueue(pcb_t *pcb)
// {
//     queues.PCBs[queues.end++] = *pcb;
//     queues.end = queues.end % QCAPACITY;
// }

pcb_t *dequeue()
{
    //are there any left to dequeue?
    if(queue == NULL)
    {
        return NULL; // error
    }
    else
    {
        pcb_t* temp = queue;
        queue = queue->next;
        //temp->next = NULL;
        return temp;
    }

}

void enqueue_priority(pcb_t *pcb)
{
    //pcb->next = NULL;
    pcb_t* prev = NULL;
    pcb_t* temp = queue;
    if(!temp) 
    {
        pcb->next = NULL;
        queue = pcb;
        return;
    }
    while(temp)
    {
        if(temp->priority < pcb->priority && (prev != NULL)) 
        {
            prev->next = pcb;
            pcb->next = temp;
            return;
        }
        else if(temp->priority < pcb->priority && (prev == NULL)) 
        {
            queue = pcb;
            pcb->next = temp;
            return;
        }
        prev = temp;
        temp = temp->next;
    }

    if(prev)
    {
        prev->next = pcb;
        pcb->next = NULL;
    }
    else
    {
        // error, this shouldn't be reached
    }
}

int createProcess(uint32_t processEntry, uint32_t priority)
{
    
    if(currentStack == stackSize)//if no stack available
    {
        return -1;
    }

    int *stackPointer = stackAllocate(); //stackPointer set to return stackAllocate
    
    int codeSelector = 16;

    stackPointer = stackPointer + userStackSize;
    stackPointer--;

    *stackPointer = ((uint32_t)go);
    stackPointer--;
    
    *stackPointer = 0x200; //EFLAGSs
    stackPointer--;

    *stackPointer = codeSelector; //setting codeSelector to 16
    stackPointer--;
    
    *stackPointer = processEntry;
    stackPointer--;

	*stackPointer = 0;    //ebp 
	stackPointer--;
	*stackPointer = 0;    //now esp
	stackPointer--;
	*stackPointer = 0;    //edi
	stackPointer--;
	*stackPointer = 0;    //esi
	stackPointer--;
	*stackPointer = 0;    //edx
	stackPointer--;
	*stackPointer = 0;    //ecx
	stackPointer--;
	*stackPointer = 0;    //ebx
	stackPointer--;
	*stackPointer = 0;   //finally, eax

    stackPointer--;
	*stackPointer = 8;   //ds, set to 8
	stackPointer--;
	*stackPointer = 8;   //now es
    stackPointer--;
	*stackPointer = 8;   //then fs
	stackPointer--;
	*stackPointer = 8;   //finally, gs
    //stackPointer--;

    pcb_t *pcb = pcbAllocate();

	pcb->esp = stackPointer;
	numProcesses++;

    pcb->priority = priority;

	pcb->pid = numProcesses;
	enqueue_priority(pcb);	//add pointer to queue


	return 0;
}

void p1()
{
    
    for(int i = 0; i < 1000000; i++)
    {
        char* message = "this is process 1 with a value of: ";
        char* temp = "000";

        convertNumHeader(i%100, temp);

        int numToPrint = 36;

        k_print(message, numToPrint, 5, 1);
        k_print(temp, 3, 5, numToPrint+1);
        //i = ((i+1)%500);
    }
}

void p2()
{
    
    for(int i = 0; i < 1000000; i++)
    {
        char* message = "this is process 2 with a value of: ";
        char* temp = "000";

        convertNumHeader(i%100, temp);

        int numToPrint = 36;

        k_print(message, numToPrint, 6, 1);
        k_print(temp, 3, 6, numToPrint+1);
        //i = ((i+1)%500);
    }
}

void p3()
{
    
    for(int i = 0; i < 1000000; i++)
    {
        char* message = "this is process 3 with a value of: ";
        char* temp = "000";

        convertNumHeader(i%100, temp);

        int numToPrint = 36;

        k_print(message, numToPrint, 7, 1);
        k_print(temp, 3, 7, numToPrint+1);
        //i = ((i+1)%500);
    }
}

void idle()
{
    k_print("Process idle running...", 23, 23, 1);

    while(1)
    {
        k_print("/", 1, 23, 24);
        k_print("\\", 1, 23, 24);
    }
}



// void p4()
//  {
//     int i = 0;
//     while(1)
//     {
//         char* message = "this is process 4: ";
//         char* temp = "0000";

//         convertNumHeader(i, temp);

//         int numToPrint = 19;

//         k_print(message, numToPrint, 8, 0);
//         k_print(temp, 4, 8, numToPrint+1);
//         i = ((i+1)%500);
//     }
// }

// void p5()
// {
//     int i = 0;
//     while(1)
//     {
//         char* message = "this is process 5: ";
//         char* temp = "0000";

//         convertNumHeader(i, temp);

//         int numToPrint = 19;

//         k_print(message, numToPrint, 9, 0);
//         k_print(temp, 4, 9, numToPrint+1);
//         i = ((i+1)%500);
//     }
// }

void goToNext(uint32_t process, uint32_t priority, char* processName, int nameSize)
{
    retval = createProcess((uint32_t) process, priority);
    if(retval < 0)
    {
        k_print("Error: return value less than zero for process ", 47, 0, gloRow); 
        k_print(processName, nameSize, 48, gloRow);

        gloRow++;
    }
}

void setRunning(uint32_t esp)
{
    running->esp = esp;
}

//program 3 functions
void setup_PIC() {
    // set up cascading mode:
    outportb(0x20, 0x11); // start 8259 master initialization
    outportb(0xA0, 0x11); // start 8259 slave initialization
    outportb(0x21, 0x20); // set master base interrupt vector (idt 32-38)
    outportb(0xA1, 0x28); // set slave base interrupt vector (idt 39-45)
    // Tell the master that he has a slave:
    outportb(0x21, 0x04); // set cascade ...
    outportb(0xA1, 0x02); // on IRQ2
    // Enabled 8086 mode:
    outportb(0x21, 0x01); // finish 8259 initialization
    outportb(0xA1, 0x01);
    // Reset the IRQ masks
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
    // Now, enable the clock IRQ only 
    outportb(0x21, 0xfe); // Turn on the clock IRQ
    outportb(0xA1, 0xff); // Turn off all others
}