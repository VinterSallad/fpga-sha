typedef ulong uint64_t;

__kernel void ROL(__global uint64_t *x, int offset)
{
    *x = (((*x) << (offset)) ^ ((*x) >> (64 - (offset))));
}
