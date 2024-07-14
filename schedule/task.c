#include <task.h>
#include <io.h>
#include <gic.h>

tcb_t task_list[MAX_TASKS];
tcb_t *current_task = (tcb_t *)0;
uint32_t task_count = 0;

void create_task(void (*task_func)(), void *stack_top)
{
    if (task_count >= MAX_TASKS)
        return;

    tcb_t *task = &task_list[task_count];
    task->id = task_count;
    task->state = 1; // 就绪状态
    task->ctx.x30 = (uint64_t)task_func;
    task->ctx.x29 = (uint64_t)(stack_top - 2048); // 分配一个4KB的堆栈并初始化堆栈指针
    task->ctx.sp_el1 = (uint64_t)stack_top; // 分配一个4KB的堆栈并初始化堆栈指针

    task_count++;
}

void print_current_task()
{
    for (int i = 0; i < task_count; i++)
    {
        tcb_t *task = &task_list[i];
        printf("id: %x, sp: 0x%x, lr: 0x%x\n", task->id, task->ctx.x29, task->ctx.x30);
    }
}

static char idel_task_stack[1024] = {0};
static uint64_t idel_num = 0;
static void idel_task()
{
    while (1)
        wfi();
}

// 创建一个 idel task, task0
void schedule_init()
{
    create_task(idel_task, idel_task_stack + 1024);
}

void move_to_first_task()
{
    current_task = task_list; // 当前运行的是 idel task

    void *entry = idel_task;
    __asm__ __volatile__("mov x30, %0\n"
                         "ret\n"
                         :              // out put
                         : "r"(entry)   // in  put
                         : "x30");
}

extern void switch_context(tcb_t *, tcb_t *);

void schedule()
{
    if (task_count == 0)
        return;

    // 找到下一个就绪的任务
    uint32_t next_task_id = (current_task->id + 1) % task_count;

    // while (task_list[next_task_id].state != 1)
    // { // 跳过非就绪状态的任务
    //     next_task_id = (next_task_id + 1) % task_count;
    // }

    tcb_t *next_task = &task_list[next_task_id];

    if (current_task != next_task)
    {
        tcb_t *prev_task = current_task;
        current_task = next_task;
        // printf("prev_task %d switch to next_task %d\n", prev_task->id, next_task->id);
        // enable_interrupts();
        switch_context(prev_task, next_task);
    }
}