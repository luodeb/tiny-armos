#ifndef __ECCEPTION_H__
#define __ECCEPTION_H__

#define NUM_REGS 31

#include <aj_types.h>

typedef struct
{
    uint64_t r[NUM_REGS]; // General-purpose registers x0..x30
    uint64_t usp;         // User Stack Pointer (SP_EL0)
    uint64_t elr;         // Exception Link Register (ELR_EL3)
    uint64_t spsr;        // Saved Process Status Register (SPSR_EL3)
} trap_frame_t;

union hsr {
    uint32_t bits;
    
    struct {
        unsigned long iss:25;  /* Instruction Specific Syndrome */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    };

    /* Common to all conditional exception classes (0x0N, except 0x00). */
    struct hsr_cond {
        unsigned long iss:20;  /* Instruction Specific Syndrome */
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } cond;

    struct hsr_wfi_wfe {
        unsigned long ti:1;    /* Trapped instruction */
        unsigned long sbzp:19;
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } wfi_wfe;

    /* reg, reg0, reg1 are 4 bits on AArch32, the fifth bit is sbzp. */
    struct hsr_cp32 {
        unsigned long read:1;  /* Direction */
        unsigned long crm:4;   /* CRm */
        unsigned long reg:5;   /* Rt */
        unsigned long crn:4;   /* CRn */
        unsigned long op1:3;   /* Op1 */
        unsigned long op2:3;   /* Op2 */
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } cp32; /* HSR_EC_CP15_32, CP14_32, CP10 */

    struct hsr_cp64 {
        unsigned long read:1;   /* Direction */
        unsigned long crm:4;    /* CRm */
        unsigned long reg1:5;   /* Rt1 */
        unsigned long reg2:5;   /* Rt2 */
        unsigned long sbzp:1;
        unsigned long op1:4;    /* Op1 */
        unsigned long cc:4;     /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;    /* Instruction length */
        unsigned long ec:6;     /* Exception Class */
    } cp64; /* HSR_EC_CP15_64, HSR_EC_CP14_64 */

     struct hsr_cp {
        unsigned long coproc:4; /* Number of coproc accessed */
        unsigned long sbz0p:1;
        unsigned long tas:1;    /* Trapped Advanced SIMD */
        unsigned long res0:14;
        unsigned long cc:4;     /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;    /* Instruction length */
        unsigned long ec:6;     /* Exception Class */
    } cp; /* HSR_EC_CP */
    
    struct hsr_dabt {
        unsigned long dfsc:6;  /* Data Fault Status Code */
        unsigned long write:1; /* Write / not Read */
        unsigned long s1ptw:1; /* */
        unsigned long cache:1; /* Cache Maintenance */
        unsigned long eat:1;   /* External Abort Type */
        unsigned long sbzp0:6;
        unsigned long reg:5;   /* Register */
        unsigned long sign:1;  /* Sign extend */
        unsigned long size:2;  /* Access Size */
        unsigned long valid:1; /* Syndrome Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } dabt; /* HSR_EC_DATA_ABORT_* */
};

static inline uint32_t read_esr_el1(void)
{
    uint32_t esr;

    // 使用内联汇编读取 ESR_EL1 寄存器
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));

    return esr;
}

static inline uint32_t read_esr_el2(void)
{
    uint32_t esr;

    // 使用内联汇编读取 ESR_EL1 寄存器
    __asm__ volatile("mrs %0, esr_el2" : "=r"(esr));

    return esr;
}

static inline uint32_t read_esr_el3(void)
{
    uint32_t esr;

    // 使用内联汇编读取 ESR_EL1 寄存器
    __asm__ volatile("mrs %0, esr_el3" : "=r"(esr));

    return esr;
}

typedef void ( * irq_handler_t)(int) ;


void irq_install(int vector, void (*h)(int));
irq_handler_t *get_g_handler_vec();

#endif // __ECCEPTION_FRAME_H__