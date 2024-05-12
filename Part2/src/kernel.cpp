
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

// For the printf function
void printf(char *str)
{
    // Write first location on the video memory
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (0x1F << 8) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (0x1F << 8) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    char buffer[1024];
    int ct;
    bool isEnterPressed;
    void OnKeyDown(char c)
    {
        if(c=='\n')
        {
            buffer[ct] = 0;
            if(ct>0)
            {
                isEnterPressed = true;
            }
            else
            {
                isEnterPressed = false;
            }
            return;
        }
        char *foo = " ";
        foo[0] = c;
        printf(foo);
        this->buffer[ct] = c;
        ++ct;
    }

    void printBuffer()
    {
        isEnterPressed = false;
        ct = 0;
        printf(buffer);
        // reset buffer
        for(int i=0;i<1024;i++)
        {
            buffer[i] = 0;
        }
    }

    // find ct, fill array
    void getIntegerArray(int *arr, int* ct)
    {
        int i = 0;
        int j = 0;
        int num = 0;
        while(buffer[i] != 0)
        {
            if(buffer[i] == ' ')
            {
                if(num != 0)
                {
                    arr[j] = num;
                    num = 0;
                    j++;
                }
            }
            else
            {
                num = num*10 + (buffer[i] - '0');
            }
            i++;
        }
        if(num != 0)
        {
            arr[j] = num;
            j++;
        }
        *ct = j;

        // reset buffer
        for(int i=0;i<1024;i++)
        {
            buffer[i] = 0;
        }

        this->ct = 0;
        this->isEnterPressed = false;
    }

    bool getEnterPressed()
    {
        return isEnterPressed;
    }

    PrintfKeyboardEventHandler()
    {
        ct = 0;
        isEnterPressed = false;
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

class PrintfUDPHandler : public UserDatagramProtocolHandler
{
public:
    void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket *socket, common::uint8_t *data, common::uint16_t size)
    {
        char *foo = " ";
        for (int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
    }
};

class PrintfTCPHandler : public TransmissionControlProtocolHandler
{
public:
    bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket *socket, common::uint8_t *data, common::uint16_t size)
    {
        char *foo = " ";
        for (int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }

        if (size > 9 && data[0] == 'G' && data[1] == 'E' && data[2] == 'T' && data[3] == ' ' && data[4] == '/' && data[5] == ' ' && data[6] == 'H' && data[7] == 'T' && data[8] == 'T' && data[9] == 'P')
        {
            socket->Send((uint8_t *)"HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n", 184);
            socket->Disconnect();
        }

        return true;
    }
};
    PrintfKeyboardEventHandler kbhandler;

void sysprintf(char *str)
{
    asm("int $0x80" : : "a"(4), "b"(str));
}

void taskB()
{
        sysprintf("B");
        while(1);
}

void taskD()
{
    while(1) printf("D");;
    for(int i=0;i<1000;i++);
    exit();
    while(1);
}

void taskE()
{
    while(1)sysprintf("E");;;
    for(int i=0;i<1000;i++);
    exit();
    while(1);
}

void taskF()
{
    while(1) sysprintf("F");;
    for(int i=0;i<1000;i++) ;
    exit();
    while(1);
}

void taskDs()
{
    sysprintf("D");;
    for(int i=0;i<1000;i++);
    exit();
}

void taskEs()
{
    sysprintf("E");
    for(int i=0;i<1000;i++);
    exit();
}

void taskFs()
{
    sysprintf("F");
    for(int i=0;i<1000;i++) ;
    exit();
}

int linearSearch(int arr[], int n, int x) {
    for (int i = 0; i < n; i++) {
        if (arr[i] == x)
            return i; // Return the index of the element if found
        printf("Linear Search Working ");
    }
    return -1; // Return -1 if the element is not found
}

int binarySearch(int arr[], int n, int x) {
    printf("A Binary Search Working!\n");
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2; // Avoid overflow
        if (arr[mid] == x)
            return mid; // Return the index of the element if found
        else if (arr[mid] < x)
            low = mid + 1; // Search in the right half
        else
            high = mid - 1; // Search in the left half
    }
    return -1; // Return -1 if the element is not found
}

void taskA()
{
    int test = -1;
    int selam;
    selam = sefa(&test);
    
    if(selam==0)
    {
        printf("Child");
        test = 23;
        printfHex(test);
        for(int i=0;i<1000000000;i++);
        exit();
    }

    else
    {
        waitpid(selam);
        printf("Parent");
        test = 31;
        printfHex(test);
        execve(taskB);
    }

    printf("Terminate A");
    while (true); 
}

// Function to print the Collatz sequence for a given number n
void printCollatz() {
    int n = 500;
    int temp_n = n;
    printfHex(n);
    for(int i=0;i<100;i++)
    {
        n = temp_n;
        while (n != 1) {
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
            printfHex(n);
            if (n != 1) {
                printf(", ");
            }
        }
        // for(int i=0;i<100000000;i++);
    }
    printf("  "); // New line after each sequence
    exit();
}


void long_running_program(int n) {
    int result = 0; // Use long long for larger results
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
            printf("Long Running Program Working ");;
        }
    }
}



void TaskV21()
{
    int pid1, pid2, pid3, pid4, pid5, pid6, pid7, pid8, pid9, pid10;
    int result;
    pid1 = sefa(&pid1);

    if (pid1 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid2 = sefa(&pid2);
    if (pid2 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid3 = sefa(&pid3);
    if (pid3 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid4 = sefa(&pid4);
    if (pid4 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid5 = sefa(&pid5);
    if (pid5 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid6 = sefa(&pid6);
    if (pid6 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid7 = sefa(&pid7);
    if (pid7 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid8 = sefa(&pid8);
    if (pid8 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid9 = sefa(&pid9);
    if (pid9 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid10 = sefa(&pid10);
    if (pid10 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = binarySearch(test_array_bs, 10, 5);
        printf("One of the Binary Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    waitpid(pid1);
    waitpid(pid2);
    waitpid(pid3);
    waitpid(pid4);
    waitpid(pid5);
    waitpid(pid6);
    waitpid(pid7);
    waitpid(pid8);
    waitpid(pid9);
    waitpid(pid10);
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n");
    printf("STRATEGY-1 TERMINATED!\n\n\n");
    exit();
}

void TaskV22()
{
    int pid1, pid2, pid3, pid4, pid5, pid6;
    int result;
    pid1 = sefa(&pid1);

    if (pid1 == 0)
    {
        // call binary search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = linearSearch(test_array_bs, 10, 5);
        printf("One of the linear Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid2 = sefa(&pid2);
    if (pid2 == 0)
    {
        // call linear search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = linearSearch(test_array_bs, 10, 5);
        printf("One of the linear Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid3 = sefa(&pid3);
    if (pid3 == 0)
    {
        // call linear search
        int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        result = linearSearch(test_array_bs, 10, 5);
        printf("One of the linear Searches Result: ");
        printfHex(result);
        printf("\n");
        exit();
    }

    pid4 = sefa(&pid4);
    if (pid4 == 0)
    {
        long_running_program(3);
        printf("\n");
        exit();
    }

    pid5 = sefa(&pid5);
    if (pid5 == 0)
    {
        long_running_program(3);
        printf("\n");
        exit();
    }

    pid6 = sefa(&pid6);
    if (pid6 == 0)
    {
        long_running_program(3);
        printf("\n");
        exit();
    }
    waitpid(pid1);
    waitpid(pid2);
    waitpid(pid3);
    waitpid(pid4);
    waitpid(pid5);
    waitpid(pid6);
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    printf("STRATEGY-2 TERMINATED!\n");
    exit();
}

void TaskV23()
{
    int pid;
    pid = sefa(&pid);

    if (pid == 0) {
        execve(printCollatz);
        exit();
    }

    else
    {
        printf("\nChildren:\n");
        int clockCounter;
        while(getInterruptCounter(&clockCounter) < 30);

        pid = sefa(&pid);
        
        if (pid == 0) {
            int c_pid;
            setPriority(getPid(&c_pid), 20);
            int  child;
            long_running_program(50);
            exit();
        }
        printf("Parent pid: ");
        printfHex(getPid(&pid));
        printf("\n");

        pid = sefa(&pid);

        if (pid == 0) {
            int c_pid;
            setPriority(getPid(&c_pid), 20);
            int  child;
            int test_array_bs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            binarySearch(test_array_bs, 10, 5);
            exit();
        }

        pid = sefa(&pid);

        if (pid == 0) {
            int c_pid;
            setPriority(getPid(&c_pid), 20);
            int  child;
            int test_array_ls[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            linearSearch(test_array_ls, 10, 5);
            exit();
        }
    }
    exit();
}

void TaskV24()
{
    int pid;
    pid = sefa(&pid);

    if (pid == 0) {
        execveLow(taskD);
    }

    else
    {
        pid = sefa(&pid);
        
        if (pid == 0) {
            while(1) printf("B");
            exit();
        }

        pid = sefa(&pid);

        if (pid == 0) {
            while(1) printf("A");
            exit();
        }

        pid = sefa(&pid);

        if (pid == 0) {
            while(1) printf("C");
            exit();
        }
    }
    exit();
}

void init()
{
    printf("Init process started. Microkernels will be started respectively...\n");

    int pid;

    pid = sefa(&pid);

    if (pid == 0) {
        // Child process
        TaskV21();
        exit();
    } 

    // Parent process
    waitpid(pid);

    setPriority(getPid(0), 22);
    printf("\nFirst strategy is completed sucessfuly! If you want to continue to Strategy-2, type something and press enter...\n");
    while(!kbhandler.getEnterPressed())
    {
    }
    setPriority(getPid(0), 10);
    printf("\n");

    
    pid = sefa(&pid);

    if (pid == 0) {
        // Child process
        TaskV22();
        exit();
    }

    // Parent process
    waitpid(pid);

    setPriority(getPid(0), 22);
    printf("\nFirst strategy is completed sucessfuly! If you want to continue to Strategy-2, type something and press enter...\n");
    while(!kbhandler.getEnterPressed())
    {
    }
    setPriority(getPid(0), 10);
    printf("\n");

    pid = sefa(&pid);

    if (pid == 0) {
        // Child process
        TaskV23();
        exit();
    }

    // Parent process
    waitpid(pid);

    setPriority(getPid(0), 22);
    printf("\nFirst strategy is completed sucessfuly! If you want to continue to Strategy-3, type something and press enter...\n");
    while(!kbhandler.getEnterPressed())
    {
    }
    setPriority(getPid(0), 10);
    printf("\n");

    pid = sefa(&pid);

    if (pid == 0) {
        // Child process
        TaskV24();
    }

    // Parent process
    waitpid(pid);

    setPriority(getPid(0), 22);
    printf("\nFirst strategy is completed sucessfuly! If you want to continue to Strategy-4, type something and press enter...\n");
    while(!kbhandler.getEnterPressed())
    {
    }
    setPriority(getPid(0), 10);
    printf("\n");

    printf("All microkernels are completed. System is shutting down...\n");
    exit();
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    
    printf("Hello World!\n");

    GlobalDescriptorTable gdt;

    uint32_t *memupper = (uint32_t *)(((size_t)multiboot_structure) + 8);
    size_t heap = 10 * 1024 * 1024;
    MemoryManager memoryManager(heap, (*memupper) * 1024 - heap - 10 * 1024);

    // printf("heap: 0x");
    // printfHex((heap >> 24) & 0xFF);
    // printfHex((heap >> 16) & 0xFF);
    // printfHex((heap >> 8) & 0xFF);
    // printfHex((heap) & 0xFF);

    void *allocated = memoryManager.malloc(1024);
    // printf("\nallocated: 0x");
    // printfHex(((size_t)allocated >> 24) & 0xFF);
    // printfHex(((size_t)allocated >> 16) & 0xFF);
    // printfHex(((size_t)allocated >> 8) & 0xFF);
    // printfHex(((size_t)allocated) & 0xFF);
    // printf("\n");

    TaskManager taskManager;

    // Task task1(&gdt, init);
    // taskManager.AddTask(&task1);
    // Task task2(&gdt, taskB);
    // taskManager.AddTask(&task2);

    // Task task1(&gdt, taskD, 0);
    // taskManager.AddTask(&task1);

    // Task task2(&gdt, taskE, 1);
    // taskManager.AddTask(&task2);

    // Task task3(&gdt, taskF, 2);
    // taskManager.AddTask(&task3);

    Task task4(&gdt, init, 10);
    taskManager.AddTask(&task4);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    DriverManager drvManager;

    KeyboardDriver keyboard(&interrupts, &kbhandler);

    drvManager.AddDriver(&keyboard);

    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);

    drvManager.AddDriver(&mouse);

    PeripheralComponentInterconnectController PCIController;
    PCIController.SelectDrivers(&drvManager, &interrupts);

    drvManager.ActivateAll();

    interrupts.Activate();

    while (1)
    {
    }
}
