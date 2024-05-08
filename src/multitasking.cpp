
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
    this->gdt = gdt;
}

Task::Task(Task *parent)
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));

    // copy stack
    for(int j = 0; j < 4096; j++)
    {
        stack[j] = parent->stack[j];
    }
    
    
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

    //copy stack
    // for(uint32_t i = 0; i < 4096; i++)
    // {
    //     stack[i] = parent->stack[i];
    // }

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
        printfHex(tasks[i].getPid());
        printf("\n");
    }
}
void copyTask(Task* parent, Task* child, CPUState* cpu) {
    // Assume CPUState and stack are already allocated and initialized for the child

    // Adjust the instruction pointer to skip the 'int $0x80'
    // child->cpustate->eip += 0;  // Assuming 'int $0x80' is at parent->cpustate->eip

    // Copy the stack
    for (int j = 0; j < 4096; j++) {
        child->stack[j] = parent->stack[j];
    }

    *(child->cpustate) = *(parent->cpustate);
    child->cpustate->eip = (uint32_t)cpu->eip + 2;
    // Copy the entire CPU state


    // If the stack pointer (esp) should be adjusted relative to the new stack:
    // child->cpustate->esp = (uint32_t)(child->stack + (parent->cpustate->esp - (uint32_t)parent->stack));
}

void taskA();
void taskB();


common::uint32_t TaskManager::Fork(CPUState* cpu) {
    // Assuming `currentTask` points to the currently executing task
    
    Task task1(gdt, taskA);
    Task task2(gdt, taskB);
    AddTask(task1);
    AddTask(task2);
}

// common::uint32_t TaskManager::Fork(CPUState* cpu) {
//     if (numTasks + 2 >= 256) {  // Ensure there's enough space for two more tasks
//         printf("Task limit reached, cannot create more tasks.\n");
//         return -1;
//     }

//     // Initialize and add taskA
//     tasks[numTasks] = Task(gdt, taskA);
//     tasks[numTasks].cpustate->eax = 0;
//     tasks[numTasks].cpustate->eip = (uint32_t)taskA;
//     // tasks[numTasks].cpustate->esp = (uint32_t)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
//     tasks[numTasks].cpustate->cs = gdt->CodeSegmentSelector();
//     tasks[numTasks].cpustate->ss = gdt->DataSegmentSelector();
//     tasks[numTasks].cpustate->eflags = 0x202;
//     tasks[numTasks].setPid(nextPid++);
//     numTasks++;

//     // Initialize and add taskB
//     tasks[numTasks] = Task(gdt, taskB);
//     tasks[numTasks].cpustate->eax = 0;
//     tasks[numTasks].cpustate->eip = (uint32_t)taskB;
//     // tasks[numTasks].cpustate->esp = (uint32_t)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
//     tasks[numTasks].cpustate->cs = gdt->CodeSegmentSelector();
//     tasks[numTasks].cpustate->ss = gdt->DataSegmentSelector();
//     tasks[numTasks].cpustate->eflags = 0x202;
//     tasks[numTasks].setPid(nextPid++);
//     numTasks++;

//     printAll();
//     return 0;  // Return success
// }


TaskManager::TaskManager(GlobalDescriptorTable *gdt)
{
    this->gdt = gdt;
    numTasks = 0;
    currentTask = -1;
    nextPid = 0;

    // for(int i = 0; i < 256; i++)
    // {
    //     tasks[i] = Task();
    // }
    // AddTask(&tasks[0]);

    // Task task1(gdt, taskA);
    // Task task2(gdt, taskB);
    // AddTask(task1);
    // AddTask(task2);
}



TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
    nextPid = 0;

    // for(int i = 0; i < 256; i++)
    // {
    //     tasks[i] = Task();
    // }
    // AddTask(&tasks[0]);

    //  Task task1(gdt, taskA);
    // Task task2(&gdt, taskB);
    // AddTask(&task1);
    // AddTask(&task2);
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task task)
{   
    
    if(numTasks >= 256)
        return false;
    // tasks[numTasks] = Task();
    tasks[numTasks] = task;
    // tasks[numTasks].setPid(nextPid);
    // nextPid = nextPid + 1;
    numTasks = numTasks + 1;
    // while(numTasks>1) printfHex(numTasks);
    return true;
}

Task::Task()
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
    cpustate -> eip = (myos::common::uint32_t)taskA;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    forked = false;
    this->gdt = gdt;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    printf("Scheduling\n");
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask].cpustate;
}