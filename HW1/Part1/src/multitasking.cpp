
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
    this->gdt = gdt;
    
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
    
    return tasks[numTasks-1].pid;
}


void TaskManager::PrintAll()
{
    printf("All Tasks:\n");
    for (int i = 0; i < numTasks; i++)
    {
        printf("Task PID: ");
        printfHex(tasks[i].pid);
        printf(" ");
        printf("Task State (0-Blocked, 1-Ready, 3-Terminated): ");
        printfHex(tasks[i].state);
        printf("\n");
    }
    printf("\n\n");
}

bool TaskManager::WaitPID(common::uint32_t pid, CPUState* cpu)
{
    int index = getIndex(pid);
    if(index > -1)
    {
        tasks[currentTask].state = TASK_WAITING;
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

bool TaskManager::Execve(CPUState* cpustate, void entrypoint())
{
    tasks[currentTask].state = TASK_TERMINATED;
    Task task(tasks[currentTask].gdt, entrypoint);
    AddTask(&task);
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

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;


    int searchedTask=(currentTask+1)%numTasks;
    
    while(tasks[searchedTask].state != TASK_READY)
    {
        if(tasks[searchedTask].state == TASK_WAITING && tasks[tasks[searchedTask].waitPid-1].state == TASK_TERMINATED)
        {
            tasks[searchedTask].state = TASK_READY;
            tasks[searchedTask].waitPid = 0;
            break;
        }
        searchedTask=(searchedTask+1)%numTasks;
    }

    PrintAll();
    currentTask = searchedTask;
    return tasks[currentTask].cpustate;
}

    