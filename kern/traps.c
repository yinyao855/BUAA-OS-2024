#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);

void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

void do_ri(struct Trapframe *tf) {
	curenv->env_ri_cnt++;
    
    u_long va = tf->cp0_epc; //va是异常指令的虚拟地址（用户虚拟地址）
    Pte *pte;
    page_lookup(curenv->env_pgdir, va, &pte);//通过查询curenv页表，获得页表项
    u_long pa = PTE_ADDR(*pte) | (va & 0xfff);//由页表项获得物理地址
    u_long kva = KADDR(pa);
    //将物理地址转化至"kseg0 区间中对应的虚拟地址"（内核虚拟地址）
    
    int *instr = (int *)kva;//内核虚拟地址可以直接访存获得指令
    int code = (*instr)>>26;
    int subcode = (*instr)&(0xf);

    int rs = ((*instr)>>10)&(0x1f);
    int rt;
    int rd
    
    if (code == 0) {
        if (subcode == 0) {
            printk("add ov handled\n");
        } else if (subcode == 2) {
            printk("sub ov handled\n");
        }
        (*instr) = (*instr)|(0x1); //把指令换成addu或subu
    } else {
        tf->cp0_epc += 4;
        printk("addi ov handled\n");
        int reg_s = ((*instr)>>21) & (0x1f);
        int reg_t = ((*instr)>>16) & (0x1f);
        u_int imm = (*instr) & (0xffff);
        tf->regs[reg_t] = tf->regs[reg_s]/(u_int)2 + imm/(u_int)2;
    }
    return;
}