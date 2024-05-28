 
#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace myos
{
    
    class SyscallHandler : public hardwarecommunication::InterruptHandler
    {
        
    public:
        SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber);
        ~SyscallHandler();
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);

    };

    int fork(int *pid);
    void waitpid(int pid);
    void exit();
    void execve(void entrypoint());
    void execveHigh(void entrypoint());
    void execveLow(void entrypoint());
    void setPriority(int pid, int priority);
    int getInterruptCounter(int *counter);
    int getPid(int *pid);
    void setDynamicPriority(int pid, int isDynamic);


}

#endif