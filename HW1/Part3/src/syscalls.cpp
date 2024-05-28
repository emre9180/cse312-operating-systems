
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

void myos::setPriority(int pid, int priority)
{
    asm("int $0x80" :: "a"(5), "b"(pid), "c"(priority));
}

int myos::getInterruptCounter(int *counter)
{
    asm("int $0x80" : "=c"(*counter) : "a"(6));
    return *counter;
}

int myos::getPid(int *pid)
{
    asm("int $0x80" : "=c"(*pid) : "a"(7));
    return *pid;
}
uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    

    switch(cpu->eax)
    {
        case 1:
            // Deactive timer
            asm("cli");
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->ExitTask(cpu);
            asm("sti");
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
        case 5:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->SetPriority(cpu->ebx, cpu->ecx);
            asm("int $0x20" : : "a"(cpu));
            break;
        case 6:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->GetInterruptCounter();
            break;
        case 7:
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->GetPId();
            break;
        case 11:
            asm("cli");
            cpu->ecx = InterruptHandler::interruptManager->GetTaskManager()->Execve(cpu, (void (*)())cpu->ebx);
            asm("sti");
            asm("int $0x20" : : "a"(cpu));
            break;
            
        default:
            break;
    }

    
    return esp;
}

