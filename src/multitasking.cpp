
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

common::uint32_t Task::getPid()
{
    return pid;
}

void Task::setPid(common::uint32_t pid)
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
void printfHex32(common::uint32_t key);
void TaskManager::printAll()
{
    printf("Tasks: \n");
    for(int i = 0; i < numTasks; i++)
    {
        printf("Task ");
        printfHex(i);
        printf(" PID: ");
        printfHex32(tasks[i].getPid());
        printf("\n");
    }
}

common::uint32_t TaskManager::getEsp()
{
    return tasks[currentTask].cpustate->esp;
}
void copyTask(Task* parent, Task* child, CPUState* cpu, GlobalDescriptorTable *gdt, CPUState* parentCpu) {

    // // copy stack
    for(int j = 0; j < 4096; j++)
    {
        child->stack[j] = parent->stack[j];
    }
    child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));

    child->cpustate -> eax = cpu->eax;
    child->cpustate -> ebx = cpu->ebx;
    child->cpustate -> ecx = cpu->ecx;
    child->cpustate -> edx = cpu->edx;

    child->cpustate -> esi = cpu->esi;
    child->cpustate -> edi = cpu->edi;
    child->cpustate -> ebp = cpu->ebp;
    
    // Increase 2 byte parent's eip
    child->cpustate -> eip = cpu->eip - 2;

    // UPdate stack pointer
    child->cpustate -> esp = (uint32_t)(&child->stack[0] + 4096 - sizeof(CPUState));

    child->cpustate -> cs = gdt->CodeSegmentSelector();
    child->cpustate -> eflags = 0x202;
    child->gdt = gdt;
}

void taskA();
void taskB();
void sysprintf(char* str);

void fillTask(GlobalDescriptorTable *gdt, Task* task, void entrypoint()) {
    task->cpustate = (CPUState*)(task->stack + 4096 - sizeof(CPUState));
    
    task->cpustate -> eax = 0;
    task->cpustate -> ebx = 0;
    task->cpustate -> ecx = 0;
    task->cpustate -> edx = 0;

    task->cpustate -> esi = 0;
    task->cpustate -> edi = 0;
    task->cpustate -> ebp = 0;
    
    task->cpustate -> eip = (uint32_t)entrypoint;
    task->cpustate -> cs = gdt->CodeSegmentSelector();
    task->cpustate -> eflags = 0x202;
    task->gdt = gdt;
}


common::uint32_t TaskManager::Fork(CPUState* cpu, CPUState* parentCpu) {
    // Assuming `currentTask` points to the currently executing task
    
    // fillTask(gdt, &tasks[numTasks], taskA);
    // numTasks++;
    // fillTask(gdt, &tasks[numTasks], taskB);
    // numTasks++;
    if(tasks[currentTask].isForked() == true) {
        printf("Cannot fork a forked task\n");
        tasks[currentTask].setForked(false); // Reset the forked flag
        return 0;
    }

    //Print parent registers
    printf("Parent registers: \n");
    printf("eax: ");
    printfHex(parentCpu->eax);
    printf("ebx: ");
    printfHex(parentCpu->ebx);
    printf("ecx: ");
    printfHex(parentCpu->ecx);
    printf("edx: ");
    printfHex(parentCpu->edx);
    printf("esi: ");
    printfHex(parentCpu->esi);
    printf("edi: ");
    printfHex(parentCpu->edi);
    printf("ebp: ");
    printfHex(parentCpu->ebp);
    printf("eip: ");
    printfHex(parentCpu->eip);
    printf("esp: ");
    printfHex(parentCpu->esp);
    printf("cs: ");
    printfHex(parentCpu->cs);
    printf("ss: ");
    printfHex(parentCpu->ss);
    printf("eflags: ");
    printfHex(parentCpu->eflags);
    printf("\n");

    //Print parentCpu rgisters
    printf("cpu registers: \n");
    printf("eax: ");
    printfHex(cpu->eax);
    printf("ebx: ");
    printfHex(cpu->ebx);
    printf("ecx: ");
    printfHex(cpu->ecx);
    printf("edx: ");
    printfHex(cpu->edx);
    printf("esi: ");
    printfHex(cpu->esi);
    printf("edi: ");
    printfHex(cpu->edi);
    printf("ebp: ");
    printfHex(cpu->ebp);
    printf("eip: ");
    printfHex(cpu->eip);
    printf("esp: ");
    printfHex(cpu->esp);
    printf("cs: ");
    printfHex(cpu->cs);
    printf("ss: ");
    printfHex(cpu->ss);
    printf("eflags: ");
    printfHex(cpu->eflags);
    printf("\n");

    //Print current task rgisters
    printf("current task registers: \n");
    printf("eax: ");
    printfHex(tasks[currentTask].cpustate->eax);
    printf("ebx: ");
    printfHex(tasks[currentTask].cpustate->ebx);
    printf("ecx: ");
    printfHex(tasks[currentTask].cpustate->ecx);
    printf("edx: ");
    printfHex(tasks[currentTask].cpustate->edx);
    printf("esi: ");
    printfHex(tasks[currentTask].cpustate->esi);
    printf("edi: ");
    printfHex(tasks[currentTask].cpustate->edi);
    printf("ebp: ");
    printfHex(tasks[currentTask].cpustate->ebp);
    printf("eip: ");
    printfHex(tasks[currentTask].cpustate->eip);
    printf("esp: ");
    printfHex(tasks[currentTask].cpustate->esp);
    printf("cs: ");
    printfHex(tasks[currentTask].cpustate->cs);
    printf("ss: ");
    printfHex(tasks[currentTask].cpustate->ss);
    printf("eflags: ");
    printfHex(tasks[currentTask].cpustate->eflags);
    printf("\n");


    copyTask(&tasks[currentTask], &tasks[numTasks], cpu, gdt, parentCpu);
    tasks[numTasks].setForked(true);
    // AddTask(tasks[numTasks]);
    common::uint32_t nextpid = tasks[currentTask].getPid() + 1;
    printf("next pid: ");
    printfHex32(nextpid);
    tasks[numTasks].pid = nextpid;
    numTasks++;
    printAll();
    return tasks[currentTask].getPid();  // Return success
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
    nextPid = 1;

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
    tasks[numTasks].setForked(false);
    tasks[numTasks].setPid(nextPid);
    nextPid = nextPid + 1;
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
    this->pid = 0;
    this->forked = false;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    // printf("Scheduling task\n");

    if(numTasks <= 0)
        return cpustate;
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask].cpustate;
}