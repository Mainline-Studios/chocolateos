#include <choc/vmm.h>
#include <choc/pmm.h>

static uint64_t *page_table(uint64_t entry) {
    return (uint64_t *)pmm_to_virt(entry & ~0xFFFull);
}

void vmm_map_mmio(uint64_t phys, uint64_t size) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t start = phys & ~0x1FFFFFull;
    uint64_t end = (phys + size + 0x1FFFFF) & ~0x1FFFFFull;
    for (uint64_t p = start; p < end; p += 0x200000) {
        uint64_t v = p + hhdm_offset();
        uint64_t *pml4 = page_table(cr3);
        uint64_t *pml4e = &pml4[(v >> 39) & 0x1FF];
        if (!(*pml4e & 1)) {
            void *n = pmm_alloc(4096, 4096);
            *pml4e = pmm_to_phys(n) | 0x3;
        }
        uint64_t *pdpt = page_table(*pml4e);
        uint64_t *pdpte = &pdpt[(v >> 30) & 0x1FF];
        if (!(*pdpte & 1)) {
            void *n = pmm_alloc(4096, 4096);
            *pdpte = pmm_to_phys(n) | 0x3;
        }
        if (*pdpte & 0x80) {
            continue;
        }
        uint64_t *pd = page_table(*pdpte);
        uint64_t *pde = &pd[(v >> 21) & 0x1FF];
        *pde = p | 0x1B | 0x80; /* present, rw, pwt, pcd, 2MiB */
        __asm__ volatile("invlpg (%0)" : : "r"(v) : "memory");
    }
}
