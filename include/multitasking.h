 
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
    
    enum TaskState
    {
        TASK_WAITING,
        TASK_READY,
        TASK_RUNNING,
        TASK_TERMINATED
    };
    
    class Task
    {
        friend class TaskManager;
        private:
            static common::uint32_t pIdCounter;
            common::uint8_t stack[4096]; // 4 KiB
            common::uint32_t pid = 0;
            common::uint32_t pPid = 0;
            // 0 for waiting, 1 for ready, 2 for running
            TaskState state;
            common::uint32_t waitPid;
            CPUState* cpustate;
            GlobalDescriptorTable* gdt;

        public:
            Task(GlobalDescriptorTable *gdt, void entrypoint());
            ~Task();
            Task();
            int priority;
            GlobalDescriptorTable* getGdt();
            common::uint32_t getId();

            //moveto priv
            bool forked;
    };

    class TaskManager
    {
        private:
            Task tasks[256];
            int numTasks;
            int currentTask;

        public:
            // void PrintProcessTable();
            common::uint32_t ForkTask(CPUState *cpustate);
            common::uint32_t ExecTask(void entrypoint());
            common::uint32_t AddTask(void entrypoint());
            common::uint32_t GetPId();
            bool ExitTask(CPUState *cpustate);
            bool WaitPID(common::uint32_t pid, CPUState* cpustate);
            bool Execve(CPUState* cpustate, void entrypoint());
            
            TaskManager();
            ~TaskManager();
            bool AddTask(Task *task);
            CPUState* Schedule(CPUState* cpustate);

            int getIndex(common::uint32_t pid);


            // priv geçecek
            int nextpid;

    };
}
#endif