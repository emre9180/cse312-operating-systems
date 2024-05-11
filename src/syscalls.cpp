
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


void printf(char*);
void myos::fork()
{
    asm("int $0x80" :: "a"(2));
}
void myos::fork(int *pid)
{
    asm("int $0x80" : "=c"(*pid) : "a"(2));
}

int myos::sefa(int *pid)
{
    asm("int $0x80" : "=c"(*pid) : "a"(2));
    return *pid;
}

void myos::waitpid(int pid)
{
    asm("int $0x80" :: "a"(3), "b"(pid));
}

void myos::exit()
{
    asm("int $0x80" :: "a"(1));
}

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    

    switch(cpu->eax)
    {
        case 1:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->ExitTask(cpu);
            asm("int $0x20" : : "a"(cpu));
            break;
        case 2:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->ForkTask(cpu);
            break;
        case 3:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->WaitPID(cpu->ebx, cpu);
            asm("int $0x20" : : "a"(cpu));
            break;
        case 4:
            printf((char*)cpu->ebx);
            break;
            
        default:
            break;
    }

    
    return esp;
}

