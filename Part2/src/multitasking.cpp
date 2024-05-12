
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char* str);
void printfHex(uint8_t key);
void printfHex16(uint16_t key);
void printfHex32(uint32_t key);

myos::common::uint32_t myos::Task::pIdCounter = 0;

Task::Task(GlobalDescriptorTable *gdt, void entrypoint(), int priority)
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
    this->priority = priority;
    this->state = TASK_READY;
    this->dynamicTarget = 0;
    this->waitPid = -1;
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
    this->interruptCounter = 0;
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
    tasks[numTasks].priority = tasks[currentTask].priority;
    tasks[numTasks].dynamicTarget = 0;

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
    
    // printfHex(tasks[numTasks-1].pid);
    return tasks[numTasks-1].pid;
}


int TaskManager::getIndex(common::uint32_t pid)
{
    printf("numtasks: ");
    printfHex(numTasks);
    printf("\n");
    printf("pid: ");
    printfHex(pid);
    printf("\n\n\n");
    for (int i = 0; i < numTasks; i++)
    {
        printf("pid: ");
        printfHex(tasks[i].pid);
        printf("\n");
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
    tasks[numTasks].priority = task->priority;
    //tasks[numTasks].cpustate -> esp = task->cpustate->esp;
    //tasks[numTasks].cpustate -> ss = task->cpustate->ss;

    numTasks++;
    return true;
}

bool TaskManager::Execve(CPUState* cpustate, void entrypoint())
{
    // PrintAll();
    // printf("pid :");
    // printfHex(tasks[currentTask].pid);

    tasks[currentTask].state = TASK_TERMINATED;

    Task task(tasks[currentTask].gdt, entrypoint, 20);
    
    if(numTasks >= 256)
        return false;

    tasks[numTasks].state = TASK_READY;
    tasks[numTasks].pid = nextpid++;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = task.cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task.cpustate->ebx; 
    tasks[numTasks].cpustate -> ecx = task.cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task.cpustate->edx;
    tasks[numTasks].cpustate -> esi = task.cpustate->esi;
    tasks[numTasks].cpustate -> edi = task.cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task.cpustate->ebp;
    tasks[numTasks].cpustate -> eip = task.cpustate->eip;
    tasks[numTasks].cpustate -> cs = task.cpustate->cs;
    tasks[numTasks].cpustate -> eflags = task.cpustate->eflags;
    tasks[numTasks].priority = 5;
    tasks[numTasks].dynamicTarget = 1;
    //tasks[numTasks].cpustate -> esp = task->cpustate->esp;
    //tasks[numTasks].cpustate -> ss = task->cpustate->ss;

    numTasks++;

    PrintAll();
    printf("pid :");
    printfHex(tasks[currentTask].pid);


    return true;
}

// WAİT READY RUNING FINSI

int TaskManager::getMaxPriority()
{
    int maxPriority = -1;
    for (int i = 0; i < numTasks; i++)
    {
        if(tasks[i].priority >= maxPriority && tasks[i].state != TASK_TERMINATED)
            maxPriority = tasks[i].priority;
    }
    return maxPriority;
}

common::uint32_t TaskManager::GetPId()
{
    return tasks[currentTask].pid;
}

common::uint32_t TaskManager::GetInterruptCounter()
{
    return interruptCounter;
}

void TaskManager::PrintAll()
{
    printf("All Tasks:\n");
    for (int i = 0; i < numTasks; i++)
    {
        printf("Task PID: ");
        printfHex(tasks[i].pid);
        printf(" ");
        printf("Task Priority: ");
        printfHex(tasks[i].priority);
        printf(" ");
        printf("Task State: ");
        printfHex(tasks[i].state);
        printf("\n");
    }
    printf("\n\n");
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    interruptCounter++;
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    int maxPriority = getMaxPriority();

    int findTask=(currentTask+1)%numTasks;
    
    while(tasks[findTask].state != TASK_READY || tasks[findTask].priority != maxPriority  || tasks[findTask].dynamicTarget == 1)
    {
        // printf("Girmeden once maxPriority: ");
        // printfHex(maxPriority);
        // printf(" ");
        // printf("Girmeden once findtask priority: ");
        // printfHex(tasks[findTask].priority);
        // printf("\n");

        if(interruptCounter>50 && tasks[findTask].dynamicTarget == 1 && tasks[findTask].priority != maxPriority)
        {
            tasks[findTask].priority = 25;
            interruptCounter = 0;
    // printf("MAX findTask: ");
    //     printfHex(tasks[findTask].pid);
    //     printf(" priortity");
    //     printfHex(tasks[findTask].priority);
    //     printf(" dynamic target");
    //     printfHex(tasks[findTask].dynamicTarget);
    //     printf(" maxPriority: ");
    //     printfHex(maxPriority);
    //     printf("\n");
           
            break;
        }

        else if(interruptCounter>50 && tasks[findTask].dynamicTarget == 1)
        {
        //      printf("MIN findTask: ");
        // printfHex(tasks[findTask].pid);
        // printf(" priortity");
        // printfHex(tasks[findTask].priority);
        // printf(" dynamic target");
        // printfHex(tasks[findTask].dynamicTarget);
        // printf(" maxPriority: ");
        // printfHex(maxPriority);
        // printf("\n");

            tasks[findTask].priority = 5;
            interruptCounter = 0;
            maxPriority = getMaxPriority();
        }

        if(tasks[findTask].state == TASK_WAITING && tasks[findTask].waitPid!=-1 && tasks[tasks[findTask].waitPid-1].state == TASK_TERMINATED)
        {
            if(tasks[findTask].priority==maxPriority)
            {
                tasks[findTask].state = TASK_READY;
                tasks[findTask].waitPid = -1;
                break;
            }

            else
            {
                tasks[findTask].state = TASK_READY;
                tasks[findTask].waitPid = 0;
                findTask=(findTask+1)%numTasks;
                continue;
            }
        }

        else if(tasks[findTask].priority==maxPriority && tasks[findTask].state==TASK_READY)
            break;

        findTask=(findTask+1)%numTasks;
    }

    PrintAll();
    currentTask = findTask;
    return tasks[currentTask].cpustate;
}

bool TaskManager::SetPriority(common::uint32_t pid, int priority)
{
    int index = getIndex(pid);
    if(index > -1)
    {
        tasks[index].priority = priority;
        return true;
    }
    return false;
}

bool TaskManager::SetDynamicTarget(common::uint32_t pid, int isDynamic)
{
    printf("neeeee gelen pid: ");
    printfHex(pid);
    printf("\n\n");
    common::uint32_t index = getIndex(pid);
    printf("bulunan index: ");
    printfHex(index);
    // if(index > -1)
    // {
        tasks[index].dynamicTarget = isDynamic;
        printf("index and isDynamic: ");
        printfHex(index);
        printf(" ");
        printfHex(isDynamic);
        printfHex(tasks[index].dynamicTarget);
        return true;

    // }
    // else
    // {
    //     printf("bulamadik");
    //     printf(" pid: ");
    //     printfHex(pid);
    // }
}
