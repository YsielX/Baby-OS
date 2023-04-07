#include "stdint.h"


void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm);
int has_mapping(uint64_t* pgtbl,uint64_t va);