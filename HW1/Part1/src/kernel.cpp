
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
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
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

void sysprintf(char *str)
{
    asm("int $0x80" : : "a"(4), "b"(str));
}

void printA()
{
    printf("A");
    exit();

}


void printCollatz(int n) {
    int temp_n = n;
    printfHex(n);
    for(int i=0;i<50;i++)
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
        for(int i=0;i<100000000;i++);
    }
    printf("  "); // New line after each sequence
}

void long_running_program(int n) {
    int result = 0; // Use long long for larger results
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
        }
        printf("Long running program is running. Please wait...\n");
    }
}


void ForkTest()
{
    int test = -1;
    int pid;
    pid = fork(&test);
    
    if(pid==0)
    {
        printf("Child is started.\n");
        test = 23;
        printfHex(test);
        printf("\n");
        for(int i=0;i<1000000000;i++);
        exit();
    }

    else
    {
        // waitpid(pid);
        printf("Parent is started.\n");
        test = 31;
        printfHex(test);
        printf("\n");
        // execve(taskB);
    }

    printf("Parent is terminating...\n");
    exit();
        
}

void WaitPidTest()
{
    int test = -1;
    int pid;
    pid = fork(&test);
    
    if(pid==0)
    {
        printf("Child is started.\n");
        test = 23;
        printfHex(test);
        printf("\n");
        for(int i=0;i<1000000000;i++);
        exit();
    }

    else
    {
        printf("Wait for child to finish.\n");
        waitpid(pid);
        printf("Child is finished, parent is started.\n");
        test = 31;
        printfHex(test);
        printf("\n");
    }

    printf("Parent is terminating...\n");
    exit();
        
}
// Function to print the Collatz sequence for a given number n

void ExecveTest()
{
    int test = -1;
    int pid;
    pid = fork(&test);
    
    if(pid==0)
    {
        printf("Child is started.\n");
        test = 23;
        printfHex(test);
        printf("\n");
        for(int i=0;i<1000000000;i++);
        exit();
    }

    else
    {
        // waitpid(pid);
        printf("Parent is started.\n");
        test = 31;
        printfHex(test);
        printf("\n");
        execve(printA);
    }

    printf("Parent is terminating...\n");
    exit();
}

void strategy()
{
    int pid, pid2, pid3, pid4, pid5, pid6;
    pid = fork(&pid);

    if (pid == 0)
    {
        long_running_program(3);
        exit();
    }

    pid2 = fork(&pid2);

    if (pid2 == 0)
    {
        long_running_program(3);
        exit();
    }

    pid3 = fork(&pid3);

    if (pid3 == 0)
    {
        long_running_program(3);
        exit();
    }

    pid4 = fork(&pid4);

    if (pid4 == 0)
    {
        printCollatz(10);
        exit();
    }

    pid5 = fork(&pid5);

    if (pid5 == 0)
    {
        printCollatz(10);
        exit();
    }

    pid6 = fork(&pid6);

    if (pid6 == 0)
    {
        printCollatz(10);
        exit();
    }
    
    waitpid(pid6);
    waitpid(pid5);
    waitpid(pid4);
    waitpid(pid3);
    waitpid(pid2);
    waitpid(pid);

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

    Task task1(&gdt, strategy);
    taskManager.AddTask(&task1);

    // Task task1(&gdt, ForkTest);
    // taskManager.AddTask(&task1);

    // Task task1(&gdt, ExecveTest);
    // taskManager.AddTask(&task1);

    // Task task1(&gdt, WaitPidTest);
    // taskManager.AddTask(&task1);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    DriverManager drvManager;

    PrintfKeyboardEventHandler kbhandler;
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
