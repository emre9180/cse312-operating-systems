
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

void TaskManager::StartNewTask(Task* child, GlobalDescriptorTable* gdt, void(*entrypoint)()) {
    child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));

    child->cpustate->eax = 0;
    child->cpustate->ebx = 0;
    child->cpustate->ecx = 0;
    child->cpustate->edx = 0;
    child->cpustate->esi = 0;
    child->cpustate->edi = 0;
    child->cpustate->ebp = 0;
    
    child->cpustate->eip = (uint32_t)entrypoint;
    child->cpustate->cs = gdt->CodeSegmentSelector();
    // child->cpustate->ss = gdt->StackSegmentSelector();  // Make sure this is set if needed
    // child->cpustate->ds = gdt->DataSegmentSelector();
    // child->cpustate->es = gdt->DataSegmentSelector();
    // child->cpustate->fs = 0;  // Typically not used, set to zero
    // child->cpustate->gs = 0;  // Typically not used, set to zero
    child->cpustate->eflags = 0x202;  // Set standard flags, ensure IF is set to enable interrupts

    child->gdt = gdt;
    child->forked = false;
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
    child->cpustate->esp = cpu->esp;
    child->cpustate->eip = cpu->eip + 2;  // Set the child's instruction pointer to the same as the parent's

    // Make any necessary adjustments to eip, or other registers if required
    // For example, if the fork system call needs to return 0 in the child:
    child->cpustate->eax = 0;
}




common::uint32_t TaskManager::Fork(CPUState* cpu) {
    if (numTasks >= 256)
        return -1;  // Return an error if too many tasks

    Task* parent = tasks[currentTask];
    Task* child = tasks[numTasks];  // Assuming you have a copy constructor
    StartNewTask(child, parent->gdt, (void (*)())parent->cpustate->eip);
    copyTask(parent, child, cpu);

    child->cpustate->eax = 0;  // Child sees fork() return 0
    parent->cpustate->eax = child->getPid();  // Parent sees child's PID

    AddTask(child);
    printAll();
    return parent->cpustate->eax;
}

// common::uint32_t TaskManager::Fork(CPUState* cpu) {
//     if (numTasks >= 256) {
//         return -1; // Maximum number of tasks reached, cannot create more
//     }

//     Task* parent = tasks[currentTask]; // Current running task
//     Task* child = tasks[numTasks]; // Create a new task as a copy of the parent
//     StartNewTask(child, parent->gdt, (void (*)())cpu->eip); // Initialize child task

//     // Manually copy the stack from the parent to the child, excluding the CPUState area
//     for (int i = 0; i < 4096 - sizeof(CPUState); i++) {
//         child->stack[i] = parent->stack[i];
//     }

//     // Now set up the child's CPUState from the current interrupt state
//     *(child->cpustate) = *cpu; // Copy CPUState structure contents

//     // Adjust stack pointers within the child's stack context
//     child->cpustate->eflags |= 0x200; // Ensure IF is set to enable interrupts

//     // Set the child's instruction pointer to continue from where it left off
//     child->cpustate->eip = cpu->eip + 2; // Skip the 'int $0x80' instruction

//     // Fork should return 0 in the child
//     child->cpustate->eax = 0;

//     // Add the new task to the list of managed tasks
//     child->setPid(nextPid++); // Assign a new PID to the child
//     AddTask(child);
//     printAll();
//     // Parent gets the PID of the newly created child
//     parent->cpustate->eax = child->getPid();

//     return parent->cpustate->eax; // Return child's PID to parent
// }


TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
    nextPid = 0;
}

TaskManager::~TaskManager()
{
}
bool TaskManager::AddTask(Task* task) {
    if (numTasks >= 256)
        return false;
    
    tasks[numTasks] = task;
    task->setPid(nextPid);  // Set unique PID
    nextPid++;  // Increment PID for the next task
    numTasks++;
    return true;
}



CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if (numTasks <= 0) return cpustate;
    printf("Scheduling task ");
    tasks[currentTask]->cpustate = cpustate;  // Save current state
    currentTask = (currentTask + 1) % numTasks;  // Move to next task
    return tasks[currentTask]->cpustate;  // Load next task state
}
