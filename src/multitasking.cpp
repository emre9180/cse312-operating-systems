
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
    // Copy stack

    child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));

    for (int j = 0; j < sizeof(child->stack); j++) {
        child->stack[j] = parent->stack[j];
    }

    common::int32_t offset_cpu =   common::int32_t(parent->stack) - common::int32_t(cpu);
    common::int32_t offset_ebp =   common::int32_t(cpu->ebp) - common::int32_t(cpu);

    child->cpustate = (CPUState*)(((common::int32_t)child->stack) + offset_cpu);
    // child->cpustate->esp = (common::int32_t) child->cpustate;
    child->cpustate->ebp = (common::int32_t)child->cpustate + offset_ebp ;
    child->cpustate->esp = child->cpustate->ebp + 982;
    // child->cpustate->esp = 0;
    // child->cpustate->eip = cpu->eip - 2;



    // // Set up child's CPU state at the end of the new stack
    // child->cpustate = (CPUState*)(child->stack + 4096 - sizeof(CPUState));

    // // Copy general registers
    child->cpustate->eax = cpu->eax;
    child->cpustate->ebx = cpu->ebx;
    child->cpustate->ecx = cpu->ecx;
    child->cpustate->edx = cpu->edx;

    child->cpustate->esi = cpu->esi;
    child->cpustate->edi = cpu->edi;
    child->cpustate->ss  = cpu->ss;

    // myos::common::uint32_t offset_ebp =  (uint32_t)parent->stack - cpu->ebp;
    //     myos::common::uint32_t offset_esp = parentCpu->ebp - parentCpu->esp ;


    // child->cpustate->ebp = (uint32_t)child->stack - offset_ebp;
    // child->cpustate->esp = child->cpustate->ebp - offset_esp;
    //     child->cpustate->edi = child->cpustate->ebp - 20;
    //     child->cpustate->esi = parent->cpustate->esi;



    // printf("offset_esp: ");
    // printfHex32(offset_esp);
    // printf("offset_ebp: ");
    // printfHex32(offset_ebp);
    // printf("child->cpustate->esp: ");
    // printfHex32(child->cpustate->esp);
    // printf("child->cpustate->ebp: ");
    // printfHex32(child->cpustate->ebp);

    // Assuming you want to continue execution right after the fork call, not decrement EIP
    child->cpustate->eip = cpu->eip;

    // Update the code segment selector and EFLAGS
    child->cpustate->cs = gdt->CodeSegmentSelector();
    child->cpustate->eflags = cpu->eflags;  // Typically, you would want to preserve the parent's EFLAGS unless specific changes are needed

    // Set GDT pointer
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

    copyTask(&tasks[currentTask], &tasks[numTasks], cpu, gdt, parentCpu);
    tasks[numTasks].setForked(true);
    // AddTask(tasks[numTasks]);
    common::uint32_t nextpid = tasks[currentTask].getPid() + 1;
    printf("next pid: ");
    printfHex32(nextpid);
    tasks[numTasks].pid = nextpid;
    
    printf("ebp - esp: ");
    printfHex32(parentCpu->ebp - parentCpu->esp);
    printf("ebp - esp in child: ");
    printfHex32(tasks[numTasks].cpustate->ebp - tasks[numTasks].cpustate->esp);

    printAll();

    //Print parent registers
    printf("Parent registers: \n");
    printf("size of stack: ");
    printfHex32(sizeof(tasks[currentTask].stack));
    printf("size of CPUState: ");
    printfHex32(sizeof(CPUState));
    printf("stack address: ");
    printfHex32((uint32_t)tasks[currentTask].stack);
    printf("cpustate addrtess: ");
    printfHex32((uint32_t)tasks[currentTask].cpustate);
    printf("eax: ");
    printfHex32(parentCpu->eax);
    printf("ebx: ");
    printfHex32(parentCpu->ebx);
    printf("ecx: ");
    printfHex32(parentCpu->ecx);
    printf("edx: ");
    printfHex32(parentCpu->edx);
    printf("esi: ");
    printfHex32(parentCpu->esi);
    printf("edi: ");
    printfHex32(parentCpu->edi);
    printf("ebp: ");
    printfHex32(parentCpu->ebp);
    printf("eip: ");
    printfHex32(parentCpu->eip);
    printf("esp: ");
    printfHex32(parentCpu->esp);
    printf("cs: ");
    printfHex32(parentCpu->cs);
    printf("ss: ");
    printfHex32(parentCpu->ss);
    printf("eflags: ");
    printfHex32(parentCpu->eflags);
    printf("\n");

    //Print parentCpu rgisters
    printf("cpu registers: \n");
    printf("cpu: ");
    printfHex32((uint32_t)cpu);
    printf("eax: ");
    printfHex32(cpu->eax);
    printf("ebx: ");
    printfHex32(cpu->ebx);
    printf("ecx: ");
    printfHex32(cpu->ecx);
    printf("edx: ");
    printfHex32(cpu->edx);
    printf("esi: ");
    printfHex32(cpu->esi);
    printf("edi: ");
    printfHex32(cpu->edi);
    printf("ebp: ");
    printfHex32(cpu->ebp);
    printf("eip: ");
    printfHex32(cpu->eip);
    printf("esp: ");
    printfHex32(cpu->esp);
    printf("cs: ");
    printfHex32(cpu->cs);
    printf("ss: ");
    printfHex32(cpu->ss);
    printf("eflags: ");
    printfHex32(cpu->eflags);
    printf("\n");

    //Print current task rgisters
    printf("current task registers: \n");
    printf("eax: ");
    printfHex32(tasks[currentTask].cpustate->eax);
    printf("ebx: ");
    printfHex32(tasks[currentTask].cpustate->ebx);
    printf("ecx: ");
    printfHex32(tasks[currentTask].cpustate->ecx);
    printf("edx: ");
    printfHex32(tasks[currentTask].cpustate->edx);
    printf("esi: ");
    printfHex32(tasks[currentTask].cpustate->esi);
    printf("edi: ");
    printfHex32(tasks[currentTask].cpustate->edi);
    printf("ebp: ");
    printfHex32(tasks[currentTask].cpustate->ebp);
    printf("eip: ");
    printfHex32(tasks[currentTask].cpustate->eip);
    printf("esp: ");
    printfHex32(tasks[currentTask].cpustate->esp);
    printf("cs: ");
    printfHex32(tasks[currentTask].cpustate->cs);
    printf("ss: ");
    printfHex32(tasks[currentTask].cpustate->ss);
    printf("eflags: ");
    printfHex32(tasks[currentTask].cpustate->eflags);
    printf("\n");
    
    //Print current task rgisters
    printf("ADDED task registers: \n");
    printf("stack address: ");
    printfHex32((uint32_t)tasks[numTasks].stack);
    printf("cpustate addrtess: ");
    printfHex32((uint32_t)tasks[numTasks].cpustate);
    printf("eax: ");
    printfHex32(tasks[numTasks].cpustate->eax);
    printf("ebx: ");
    printfHex32(tasks[numTasks].cpustate->ebx);
    printf("ecx: ");
    printfHex32(tasks[numTasks].cpustate->ecx);
    printf("edx: ");
    printfHex32(tasks[numTasks].cpustate->edx);
    printf("esi: ");
    printfHex32(tasks[numTasks].cpustate->esi);
    printf("edi: ");
    printfHex32(tasks[numTasks].cpustate->edi);
    printf("ebp: ");
    printfHex32(tasks[numTasks].cpustate->ebp);
    printf("eip: ");
    printfHex32(tasks[numTasks].cpustate->eip);
    printf("esp: ");
    printfHex32(tasks[numTasks].cpustate->esp);
    printf("cs: ");
    printfHex32(tasks[numTasks].cpustate->cs);
    printf("ss: ");
    printfHex32(tasks[numTasks].cpustate->ss);
    printf("eflags: ");
    printfHex32(tasks[numTasks].cpustate->eflags);
    printf("\n");
numTasks++;
    return tasks[currentTask].getPid();  // Return success
}



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