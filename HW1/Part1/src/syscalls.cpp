
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

int myos::fork(int *pid)
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

void myos::execve(void entrypoint())
{
    asm("int $0x80" :: "a"(11), "b"(entrypoint));
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
        
        case 11:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->Execve(cpu, (void (*)())cpu->ebx);
            asm("int $0x20" : : "a"(cpu));
            break;
            
        default:
            break;
    }

    
    return esp;
}

