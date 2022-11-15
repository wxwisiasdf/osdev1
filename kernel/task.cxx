#include <cstddef>
#include "task.hxx"
#include "gdt.hxx"
#include "tty.hxx"
#include "assert.hxx"

static struct Task::TSS tasks[MAX_TASKS] = {};
static size_t n_task = 0;
static bool canSwitch = true;
static Task::StackInfo stacks[MAX_TASKS] = {};

extern "C" void Kernel_EnterV86(uint32_t ss, uint32_t esp, uint32_t cs, uint32_t eip);
extern uint8_t g_KernStackTop;

Task::TSS &Task::Schedule()
{
    assert(n_task < MAX_TASKS);
    // Search for tasks after
    for (size_t i = n_task + 1; i < MAX_TASKS; i++)
    {
        auto &task = tasks[i];
        if (!task.is_active) // Must be active
            continue;
        n_task = i;

        // Make task segment available again
        auto &task_entry = GDT::GetEntry(task.tss_segment);
        if (!task.is_v86)
            task_entry.SetAccess(0x80 | GDT::Entry::TYPE_32TSS_AVAIL);
        else
            task_entry.SetAccess(0x80 | GDT::Entry::TYPE_16TSS_AVAIL);
        return task;
    }

    // Search for tasks before
    for (size_t i = 0; i < n_task; i++)
    {
        auto &task = tasks[i];
        if (!task.is_active) // Must be active
            continue;
        n_task = i;

        // Make task segment available again
        auto &task_entry = GDT::GetEntry(task.tss_segment);
        if (!task.is_v86)
            task_entry.SetAccess(0x80 | GDT::Entry::TYPE_32TSS_AVAIL);
        else
            task_entry.SetAccess(0x80 | GDT::Entry::TYPE_16TSS_AVAIL);
        return task;
    }
    __builtin_unreachable();
}

void Task::EnableSwitch()
{
    canSwitch = true;
}

void Task::DisableSwitch()
{
    canSwitch = false;
}

bool Task::CanSwitch()
{
    return canSwitch;
}

void Task::Finish()
{
    
}

Task::ThreadSummary Task::GetSummary()
{
    Task::ThreadSummary ts{};
    ts.nTotal = MAX_TASKS;
    for (size_t i = 0; i < MAX_TASKS; i++)
        if (tasks[i].is_active)
            ts.nActive++;
    return ts;
}

Task::StackInfo &Task::GetStack()
{
    for (size_t i = 0; i < MAX_TASKS; i++)
        if (!stacks[i].is_used)
            return stacks[i];
    __builtin_unreachable();
}

extern "C" void Kernel_Task16Trampoline();
extern "C" void Kernel_Task32Trampoline();

/// @brief Adds a new task
/// @param eip EIP to set task to
/// @param esp ESP to set stack to
/// @param v86 Start the task in v86 mode
/// @return Task segment
Task::TSS &Task::Add(void (*eip)(), void *esp, bool v86)
{
    for (size_t i = 0; i < MAX_TASKS; i++)
    {
        auto &task = tasks[i];
        if (!task.is_active)
        {
            if (esp == nullptr)
            {
                auto &stack = Task::GetStack();
                stack.is_used = true;
                esp = &stack.stack[sizeof(stack.stack) - 1];
            }

            task.is_active = true;
            task.is_v86 = v86;

            /// @brief Setup LDT entry for this TSS
            auto &ldt_entry = GDT::AllocateEntry();
            ldt_entry.SetBase(&task.local_entries);
            ldt_entry.SetLimit(sizeof(task.local_entries) - 1);
            ldt_entry.SetAccess(0x80 | GDT::Entry::TYPE_LDT);
            ldt_entry.access.present = 1;

            auto &task_entry = GDT::AllocateEntry();
            task_entry.SetBase(&task);
            task_entry.SetLimit(sizeof(Task::TSS) - 1);
            if (task.is_v86)
                task_entry.SetAccess(0x80 | GDT::Entry::TYPE_16TSS_AVAIL);
            else
                task_entry.SetAccess(0x80 | GDT::Entry::TYPE_32TSS_AVAIL);
            task_entry.access.present = 1;

            task.tss_segment = GDT::GetEntrySegment(task_entry);
            task.ldt_segment = GDT::GetEntrySegment(ldt_entry);

            // Kernel code, 0x08 -- so interrupts can run
            auto *local_entry = &task.local_entries.kern_xcode;
            local_entry->SetBase(nullptr);
            local_entry->SetLimit(0xFFFFFFFF);
            local_entry->flags.size = task.is_v86 ? 0 : 1;
            local_entry->access.present = 1;
            local_entry->access.privilege = 0;
            local_entry->access.always_1 = 1;
            local_entry->access.executable = 1;
            local_entry->access.read_write = 0;

            // Kernel data, 0x10 -- so interrupts can run
            local_entry = &task.local_entries.kern_xdata;
            local_entry->SetBase(nullptr);
            local_entry->SetLimit(0xFFFFFFFF);
            local_entry->flags.size = task.is_v86 ? 0 : 1;
            local_entry->access.present = 1;
            local_entry->access.privilege = 0;
            local_entry->access.always_1 = 1;
            local_entry->access.executable = 0;
            local_entry->access.read_write = 1;

            // User entries in GDT
            task.local_entries.user_xcode = task.local_entries.kern_xcode;
            task.local_entries.user_xcode.access.privilege = 3;
            task.local_entries.user_xdata = task.local_entries.kern_xdata;
            task.local_entries.user_xdata.access.privilege = 3;
            if (!task.is_v86)
            {
                task.prot.eax = 0;
                task.prot.ebx = 0;
                task.prot.ecx = 0;
                task.prot.edx = 0;
                task.prot.esi = 0;
                task.prot.edi = 0;
                task.prot.eip = (uintptr_t)eip;
                task.prot.ldtr = GDT::GetEntrySegment(ldt_entry);
                task.prot.esp = (uintptr_t)esp;
                task.prot.ebp = task.prot.esp;
                task.prot.esp0 = task.prot.esp;
                task.prot.esp1 = task.prot.esp;
                task.prot.esp2 = task.prot.esp;
                task.prot.cs = GDT::KERNEL_XCODE | GDT::LDT_SEGMENT;
                task.prot.ds = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.es = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.ss = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.gs = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.fs = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.ss0 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.ss1 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.ss2 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.prot.iopb = offsetof(Task::TSS, io_bitmap);
                asm volatile("\tmov %%cr3,%%eax\r\n"
                             "\tmov %%eax,%0\r\n"
                             :
                             : "m"(task.prot.cr3)
                             :);
                TTY::Print("task: New PROT EIP=%p,ESP=%p\n", task.prot.eip,
                           task.prot.esp);
                TTY::Print("ES=%p,CS=%p,DS=%p,SS=%p,GS=%p,FS=%p\n",
                           task.prot.es, task.prot.cs, task.prot.ds,
                           task.prot.ss, task.prot.gs, task.prot.fs);
            }
            else
            {
                task.real.ax = 0;
                task.real.bx = 0;
                task.real.cx = 0;
                task.real.dx = 0;
                task.real.si = 0;
                task.real.di = 0;
                task.real.ip = (uintptr_t)eip;
                task.real.ldtr = GDT::GetEntrySegment(ldt_entry);
                task.real.sp = (uintptr_t)esp;
                task.real.bp = task.real.sp;
                task.real.sp0 = task.real.sp;
                task.real.sp1 = task.real.sp;
                task.real.sp2 = task.real.sp;
                task.real.cs = GDT::KERNEL_XCODE | GDT::LDT_SEGMENT;
                task.real.ds = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.real.es = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.real.ss = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.real.ss0 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.real.ss1 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                task.real.ss2 = GDT::KERNEL_XDATA | GDT::LDT_SEGMENT;
                TTY::Print("task: New REAL IP=%p,SP=%p\n", task.real.ip,
                           task.real.sp);
                TTY::Print("ES=%p,CS=%p,DS=%p,SS=%p\n", task.real.es,
                           task.real.cs, task.real.ds, task.real.ss);
            }
            return task;
        }
    }
    __builtin_unreachable();
}

void Task::Sleep(unsigned int usec)
{
    while (usec--)
    {
        IO_Wait();
    }
}
