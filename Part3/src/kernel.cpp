
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
    while(1) sysprintf("D");;
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


int binarySearch()
{
    setPriority(getPid(0), 22);
    printf("\n\nGive me the numbers in sorted order and target. The last number is the target: ");
    while(!kbhandler.getEnterPressed())
    {
    }
    printf("\n");

    int ct;
    int array[25];

    kbhandler.getIntegerArray(array, &ct);

    // for(int i=0;i<ct;i++)
    // {
    //     printfHex(array[i]);
    //     printf(" ");
    // }

    // make the the search according to last element of the element
    int n = ct;
    int x = array[ct-1];
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2; // Avoid overflow
        if (array[mid] == x)
        {
            printf("Element found at index: ");
            printfHex(mid);
            printf("\n");
            return mid;
        }
        else if (array[mid] < x)
            low = mid + 1; // Search in the right half
        else
            high = mid - 1; // Search in the left half
        printf("Binary Search Working ");
    }
}

int linearSearch()
{ 
    setPriority(getPid(0), 25);
    printf("\n\nGive me the numbers and target. The last number is the target: ");
    while(!kbhandler.getEnterPressed())
    {
    }
    printf("\n");

    int ct;
    int array[25];

    kbhandler.getIntegerArray(array, &ct);

    // for(int i=0;i<ct;i++)
    // {
    //     printfHex(array[i]);
    //     printf(" ");
    // }

    // make the the search according to last element of the element
    int n = ct;
    int x = array[ct-1];
    for (int i = 0; i < n; i++) {
        if (array[i] == x)
        {
            printf("Element found at index: ");
            printfHex(i);
            printf("\n");
            return i; // Return the index of the element if found
        }
        printf("Linear Search Working ");
    }
    return -1; // Return -1 if the element is not found
}

void taskA()
{
    int test = -1;
    int selam;
    selam = fork(&test);
    
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
    int n = 50;
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

void long_running_program()
{
    setPriority(getPid(0), 20);
    printf("\n\nGive me the number: ");
    while(!kbhandler.getEnterPressed())
    {
    }

    printf("\n");

    int ct;
    int array[25];

    kbhandler.getIntegerArray(array, &ct);

    // for(int i=0;i<ct;i++)
    // {
    //     printfHex(array[i]);
    //     printf(" ");
    // }
    int n = array[0];
    int result = 0; // Use long long for larger results
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
            printf("Long Running Program Working ");;
        }
    }
}

void part3_test()
{
    int pid;
    pid = fork(&pid);

    if(pid == 0)
    {
        taskD();
        exit();
    }

    pid = fork(&pid);

    if(pid == 0)
    {
        long_running_program();
        exit();
    }

    pid = fork(&pid);

    if(pid == 0)
    {
        binarySearch();
        exit();
    }

    pid = fork(&pid);

    if(pid == 0)
    {
        linearSearch();
        exit();
    }

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

    Task task4(&gdt, part3_test, 10);
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
