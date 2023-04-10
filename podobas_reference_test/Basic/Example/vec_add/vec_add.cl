

__kernel void vec_add10(__global float *x)
{
    int index = get_global_id(0);
    x[index] = x[index] + 10.0;
}



