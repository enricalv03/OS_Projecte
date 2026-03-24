#ifndef ARM_PMM_H
#define ARM_PMM_H

void         arm_pmm_init(void);
unsigned int arm_pmm_alloc(void);
void         arm_pmm_free(unsigned int phys_addr);
unsigned int arm_pmm_free_count(void);

#endif
