
#include <multitasking.h>

using namespace myos;
using namespace myos::common;


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
    forked = false;
}

Task::Task(Task *parent)
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));

    // copy stack
    
    
    cpustate -> eax = parent->cpustate->eax;
    cpustate -> ebx = parent->cpustate->ebx;
    cpustate -> ecx = parent->cpustate->ecx;
    cpustate -> edx = parent->cpustate->edx;

    cpustate -> esi = parent->cpustate->esi;
    cpustate -> edi = parent->cpustate->edi;
    cpustate -> ebp = parent->cpustate->ebp;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    cpustate -> eip = parent->cpustate->eip;
    cpustate -> cs = parent->cpustate->cs;
    // cpustate -> ss = ;
    cpustate -> eflags = parent->cpustate->eflags;
    forked = false;
}



Task::~Task()
{
}

int Task::getPid()
{
    if(forked==true)
        return -1;
    else
        return this->pid;
}

void Task::setPid(int pid)
{
    this->pid = pid;
}

bool Task::isForked()
{
    return forked;
}

void Task::setForked(bool forked)
{
    this->forked = forked;
}

void printf(char*);
void printfHex(common::uint8_t key);

void TaskManager::printAll()
{
    printf("Tasks: \n");
    for(int i = 0; i < numTasks; i++)
    {
        printf("Task ");
        printfHex(i);
        printf(" PID: ");
        printfHex(tasks[i]->getPid());
        printf("\n");
    }
}


common::uint32_t TaskManager::Fork() {
    // Assuming `currentTask` points to the currently executing task
    this->printAll();
    Task child = Task(this->tasks[currentTask]);
    printfHex(31);
    AddTask(&child);  // Add new task and get its identifier
    return this->tasks[currentTask]->getPid();  // Return parent's PID
}

        
TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
    nextPid = 0;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks] = task;
    tasks[numTasks]->setPid(nextPid);
    nextPid = nextPid + 1;
    numTasks = numTasks + 1;
    return true;
}



CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask]->cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask]->cpustate;
}