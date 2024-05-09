 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));

    enum TaskState {
        RUNNING,
        BLOCKED,
        READY,
        TERMINATED
    };

    struct TaskStateInfo {
        TaskState state;
    };
    
    
    class Task
    {
    friend class TaskManager;
    private:
        struct TaskStateInfo stateInfo;
    public:
    common::uint8_t stack[4096]; // 4 KiB
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        Task(Task *parent);
        Task();
        int pid;
        bool forked;
        CPUState* cpustate;
        ~Task();
        common::uint32_t getPid();
        void setPid(common::uint32_t pid);
        bool isForked();
        void setForked(bool forked);
        GlobalDescriptorTable *gdt;
    };
    
    
    class TaskManager
    {
    private:
        Task tasks[256];
        int numTasks;
        int currentTask;
        common::uint32_t nextPid;
    public:
    GlobalDescriptorTable *gdt;
    TaskManager(GlobalDescriptorTable *gdt);
        TaskManager();
        ~TaskManager();
        common::uint32_t getEsp();
        void printAll();
        bool AddTask(Task task);
        common::uint32_t Fork(CPUState* cpu, CPUState* parentCpu);
        int WaitPID();
        CPUState* Schedule(CPUState* cpustate);
    };
    
    
    
}


#endif