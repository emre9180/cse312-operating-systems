
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

common::uint8_t Task::getPid()
{
    this->pid;
}

void Task::setPid(common::uint8_t pid)
{
    this->pid = pid;
}

bool Task::isForked()
{
    return forked;
}

bool Task::getForked()
{
    return forked;
}

void Task::setForked(bool forked)
{
    this->forked = forked;
}

void printf(char*);
void printfHex(common::uint8_t key);
void printfHex32(common::uint32_t key);
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
void taskA();

// void copyTask(Task* parent, Task* child, CPUState* cpu) {
//     // Assume CPUState and stack are already allocated and initialized for the child

//     // Adjust the instruction pointer to skip the 'int $0x80'
//     // child->cpustate->eip += 0;  // Assuming 'int $0x80' is at parent->cpustate->eip

//     // Copy the stack
//     cpu->esp += 8;  // Move ESP up by 8 bytes to skip over certain items on the stack

//     // Example of copying stack memory
//     uint8_t* stackPointer = (uint8_t*)cpu->esp;  // Assuming esp points to the actual stack memory
//     for (int j = 0; j < 4096; j++) {
//         child->stack[j] = stackPointer[j];  // Copy directly from the stack memory
//     }

//         child->cpustate->eip += 2;  // Assuming 'int $0x80' is 2 bytes


// }


Task::Task()
{
}

void TaskManager::StartNewTask(Task* child, GlobalDescriptorTable *gdt, void entrypoint())
{
    child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));
    
    child->cpustate -> eax = 0;
    child->cpustate -> ebx = 0;
    child->cpustate -> ecx = 0;
    child->cpustate -> edx = 0;

    child->cpustate -> esi = 0;
    child->cpustate -> edi = 0;
    child->cpustate -> ebp = 0;
    
    /*
    child->cpustate -> gs = 0;
    child->cpustate -> fs = 0;
    child->cpustate -> es = 0;
    child->cpustate -> ds = 0;
    */
    
    // child->cpustate -> error = 0;    
   
    // child->cpustate -> esp = ;
    child->cpustate -> eip = (uint32_t)entrypoint;
    child->cpustate -> cs = gdt->CodeSegmentSelector();
    // child->cpustate -> ss = ;
    child->cpustate -> eflags = 0x202;
    child->gdt = gdt;
}

void copyTask(Task* parent, Task* child, CPUState* cpu) {
// Ensure the child's stack is clean
    for (int i = 0; i < 4096; i++) {
        child->stack[i] = 0;
    }

    // Copy the stack data from parent to child
    for (int i = 0; i < 4096; i++) {
        child->stack[i] = parent->stack[i];
    }

    // Position child's CPUState at the top of its stack
    child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));

    // Explicitly copy the CPUState data
    *(child->cpustate) = *(cpu);

    // Adjust the stack pointer (esp) to the new task's stack context
    child->cpustate->esp = (common::uint32_t)(child->stack + 4096 - sizeof(CPUState));
    child->cpustate->eip = cpu->eip + 2;  // Set the child's instruction pointer to the same as the parent's

    // Make any necessary adjustments to eip, or other registers if required
    // For example, if the fork system call needs to return 0 in the child:
    child->cpustate->eax = 0;
}

common::uint32_t TaskManager::Fork(CPUState* cpu) {
    Task* child = tasks[numTasks];
    StartNewTask(child, this->tasks[currentTask]->gdt, taskA); // Initialize the new task
    copyTask(this->tasks[currentTask], child, cpu);  // Copy parent to child
    child->setPid(nextPid++);  // Set unique PID
    AddTask(child);  // Add the new task
    printAll();
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
    // tasks[numTasks]->setPid(nextPid);
    // nextPid = nextPid + 1;
    numTasks = numTasks + 1;
    // while(numTasks>1) printfHex(numTasks);
    // this->printAll();
    return true;
}


// 
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