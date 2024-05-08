
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

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp) {
    CPUState* cpu = (CPUState*)esp;

    switch(cpu->eax) {
        case 1: // FORK
            asm("cli");
            cpu->eax = this->interruptManager->getTaskManager()->Fork(cpu);  // Capture fork result
            asm("sti");
            // this->interruptManager->getTaskManager()->Fork(); 
            break;
        case 2: // FORK
            // cpu->eax = this->interruptManager->getTaskManager()->ForkHelper();  // Capture fork result
            // this->interruptManager->getTaskManager()->Fork(); 
            break;

        case 4: // Print a string
            printf((char*)cpu->ebx);
            cpu->eax = 0; // Success
            break;

        default:
            cpu->eax = -1; // Unknown system call
            break;
    }

    return esp;
}

