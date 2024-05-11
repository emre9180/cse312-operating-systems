
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char* str);
void printfHex(uint8_t key);
void printfHex16(uint16_t key);
void printfHex32(uint32_t key);

myos::common::uint32_t myos::Task::pIdCounter = 0;

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    
}

Task::~Task()
{
}

Task::Task()
{
}

common::uint32_t Task::getId()
{
    return pid;
}


TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
    nextpid = 1;
}

TaskManager::~TaskManager()
{
}

common::uint32_t TaskManager::ForkTask(CPUState *cpustate)
{
    if(numTasks >= 256)
        return -1;

    if(tasks[currentTask].forked)
    {
        tasks[currentTask].forked = false;
        return 0;
    }

    tasks[numTasks].state = TASK_READY;
    tasks[numTasks].pPid = tasks[currentTask].pid;
    tasks[numTasks].pid = nextpid++;

    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate) - ((common::uint32_t)tasks[currentTask].stack));
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + currentTaskOffset);

    // tasks[numTasks].cpustate -> ecx = 0;
    tasks[numTasks].forked = true;
    tasks[numTasks].cpustate->eip -= 2;
    numTasks++;
    
    printfHex(tasks[numTasks-1].pid);
    return tasks[numTasks-1].pid;
}


int TaskManager::getIndex(common::uint32_t pid)
{
    for (int i = 0; i < numTasks; i++)
    {
        if(tasks[i].pid == pid)
            return i;
    }
    return -1;
}

bool TaskManager::WaitPID(common::uint32_t pid, CPUState* cpu)
{
    int index = getIndex(pid);
    if(index > -1)
    {
        tasks[currentTask].state = TASK_WAITING;
        printf("waiting for: ");
        printfHex(pid);
        tasks[currentTask].waitPid = pid;
        return true;
    }

    return false;
}

bool TaskManager::ExitTask(CPUState* cpu)
{
    if(numTasks <= 0)
        return false;

    tasks[currentTask].state = TASK_TERMINATED;
    return true;
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;

    tasks[numTasks].state = TASK_READY;
    tasks[numTasks].pid = nextpid++;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = task->cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task->cpustate->ebx; 
    tasks[numTasks].cpustate -> ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task->cpustate->edx;
    tasks[numTasks].cpustate -> esi = task->cpustate->esi;
    tasks[numTasks].cpustate -> edi = task->cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task->cpustate->ebp;
    tasks[numTasks].cpustate -> eip = task->cpustate->eip;
    tasks[numTasks].cpustate -> cs = task->cpustate->cs;
    tasks[numTasks].cpustate -> eflags = task->cpustate->eflags;
    //tasks[numTasks].cpustate -> esp = task->cpustate->esp;
    //tasks[numTasks].cpustate -> ss = task->cpustate->ss;

    numTasks++;
    return true;
}

// WAİT READY RUNING FINSI

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    // PrintProcessTable();

    int findTask=(currentTask+1)%numTasks;
    while(tasks[findTask].state != TASK_READY)
    {
        if(tasks[findTask].state == TASK_WAITING && tasks[tasks[findTask].waitPid-1].state == TASK_TERMINATED)
        {
            tasks[findTask].state = TASK_READY;
            tasks[findTask].waitPid = 0;
            break;
        }
        {
            int waitTaskIndex = 0;
            //waitTaskIndex=getIndex(tasks[findTask].waitPid);
            if(waitTaskIndex > -1 && tasks[waitTaskIndex].state != TASK_WAITING)
            {
                if(tasks[waitTaskIndex].state == TASK_RUNNING)
                {
                    tasks[findTask].state = TASK_WAITING;
                    tasks[findTask].waitPid = 0;
                }
                else if (tasks[waitTaskIndex].state == TASK_READY)
                {
                    findTask = waitTaskIndex;
                    continue;
                }
            }
        }
        findTask=(findTask+1)%numTasks;
    }

    currentTask = findTask;    
    return tasks[currentTask].cpustate;
}

    