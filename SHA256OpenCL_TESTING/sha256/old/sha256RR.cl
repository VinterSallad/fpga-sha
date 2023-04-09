typedef ulong uint64_t;
typedef uchar uint8_t;
typedef uint uint32_t;

#define SHR(x, c) ((x) >> (c))
#define ROTR_32(x, c) (((x) >> (c)) | ((x) << (32 - (c))))
#define ROTR_64(x, c) (((x) >> (c)) | ((x) << (64 - (c))))

#define Ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define Sigma0_32(x) (ROTR_32(x, 2) ^ ROTR_32(x,13) ^ ROTR_32(x,22))
#define Sigma1_32(x) (ROTR_32(x, 6) ^ ROTR_32(x,11) ^ ROTR_32(x,25))
#define sigma0_32(x) (ROTR_32(x, 7) ^ ROTR_32(x,18) ^ SHR(x, 3))
#define sigma1_32(x) (ROTR_32(x,17) ^ ROTR_32(x,19) ^ SHR(x,10))

#define M_32(w0, w14, w9, w1) w0 = sigma1_32(w14) + (w9) + sigma0_32(w1) + (w0);

#define EXPAND_32 \
    words[0] = M_32(words[0], words[14], words[9], words[1]);   \
    words[1] = M_32(words[1], words[15], words[10], words[2]);  \
    words[2] = M_32(words[2], words[0], words[11], words[3]);   \
    words[3] = M_32(words[3], words[1], words[12], words[4]);   \
    words[4] = M_32(words[4], words[2], words[13], words[5]);   \
    words[5] = M_32(words[5], words[3], words[14], words[6]);   \
    words[6] = M_32(words[6], words[4], words[15], words[7]);   \
    words[7] = M_32(words[7], words[5], words[0], words[8]);    \
    words[8] = M_32(words[8], words[6], words[1], words[9]);    \
    words[9] = M_32(words[9], words[7], words[2], words[10]);   \
    words[10] = M_32(words[10], words[8], words[3], words[11]);  \
    words[11] = M_32(words[11], words[9], words[4], words[12]);  \
    words[12] = M_32(words[12], words[10], words[5], words[13]); \
    words[13] = M_32(words[13], words[11], words[6], words[14]); \
    words[14] = M_32(words[14], words[12], words[7], words[15]); \
    words[15] = M_32(words[15], words[13], words[8], words[0]);


#define F32(wV, w, k)                                   \
    wV[8] = wV[7] + Sigma1_32(wV[4]) + Ch(wV[4], wV[5], wV[6]) + (w) + (k); \
    wV[9] = Sigma0_32(wV[0]) + Maj(wV[0], wV[1], wV[2]);                \
    wV[7] = wV[6];                                           \
    wV[6] = wV[5];                                           \
    wV[5] = wV[4];                                           \
    wV[4] = wV[3] + wV[8];                                   \
    wV[3] = wV[2];                                           \
    wV[2] = wV[1];                                           \
    wV[1] = wV[0];                                           \
    wV[0] = wV[8] + wV[9];



__constant uint8_t iv_256[32] = {
    0x6a, 0x09, 0xe6, 0x67, 0xbb, 0x67, 0xae, 0x85,
    0x3c, 0x6e, 0xf3, 0x72, 0xa5, 0x4f, 0xf5, 0x3a,
    0x51, 0x0e, 0x52, 0x7f, 0x9b, 0x05, 0x68, 0x8c,
    0x1f, 0x83, 0xd9, 0xab, 0x5b, 0xe0, 0xcd, 0x19
};

__constant uint32_t roundConstants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,

    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,

    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,

    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};




uint32_t load_bigendian_32(__local uint8_t *x)
{
    return (uint32_t)(x[3]) | (((uint32_t)(x[2])) << 8) |
           (((uint32_t)(x[1])) << 16) | (((uint32_t)(x[0])) << 24);
}


uint32_t load_bigendian_32_g(__global uint8_t *x)
{
    return (uint32_t)(x[3]) | (((uint32_t)(x[2])) << 8) |
           (((uint32_t)(x[1])) << 16) | (((uint32_t)(x[0])) << 24);
}

uint64_t load_bigendian_64(__local uint8_t *x)
{
    return (uint64_t)(x[7]) | (((uint64_t)(x[6])) << 8) |
           (((uint64_t)(x[5])) << 16) | (((uint64_t)(x[4])) << 24) |
           (((uint64_t)(x[3])) << 32) | (((uint64_t)(x[2])) << 40) |
           (((uint64_t)(x[1])) << 48) | (((uint64_t)(x[0])) << 56);
}

static void store_bigendian_32(__local uint8_t *x, uint64_t u) {
    x[3] = (uint8_t) u;
    u >>= 8;
    x[2] = (uint8_t) u;
    u >>= 8;
    x[1] = (uint8_t) u;
    u >>= 8;
    x[0] = (uint8_t) u;
}

static void store_bigendian_32_g(__global uint8_t *x, uint64_t u) {
    x[3] = (uint8_t) u;
    u >>= 8;
    x[2] = (uint8_t) u;
    u >>= 8;
    x[1] = (uint8_t) u;
    u >>= 8;
    x[0] = (uint8_t) u;
}

static void store_bigendian_64(__local uint8_t *x, uint64_t u) {
    x[7] = (uint8_t) u;
    u >>= 8;
    x[6] = (uint8_t) u;
    u >>= 8;
    x[5] = (uint8_t) u;
    u >>= 8;
    x[4] = (uint8_t) u;
    u >>= 8;
    x[3] = (uint8_t) u;
    u >>= 8;
    x[2] = (uint8_t) u;
    u >>= 8;
    x[1] = (uint8_t) u;
    u >>= 8;
    x[0] = (uint8_t) u;
}


uint8_t crypto_hashblocks_sha256(__local uint8_t *statebytes,
                                   __local uint8_t *in, size_t inlen) {
                                        
    uint32_t state[8];
    uint32_t workingVars[10];

    for(int i = 0; i < 8; i++)
    {
        workingVars[i] = load_bigendian_32(statebytes + i*4);
        state[i] = workingVars[i];
    }

    //Medan det finns chunks som inte är sista chunksen, processera
    while (inlen >= 64) {
        uint32_t words[16];

        for(int i = 0; i < 16; i++)
        {
            words[i] = load_bigendian_32(in + i*4);
        }

        int round = 0; 
        for(int j = 0; j < 4; j++)
        {
            for(int i = 0; i < 16; i++)
            {
                F32(workingVars, words[i], roundConstants[round]);
                round += 1;
            }
            if(j < 3)
                EXPAND_32
        }

        for(int i = 0; i < 8; i++)
        {
            workingVars[i] += state[i];
            state[i] = workingVars[i];
        }

        in += 64;
        inlen -= 64;
    }

    //Lagra de muterade working state variables till utdata
    store_bigendian_32(statebytes + 0, state[0]);
    store_bigendian_32(statebytes + 4, state[1]);
    store_bigendian_32(statebytes + 8, state[2]);
    store_bigendian_32(statebytes + 12, state[3]);
    store_bigendian_32(statebytes + 16, state[4]);
    store_bigendian_32(statebytes + 20, state[5]);
    store_bigendian_32(statebytes + 24, state[6]);
    store_bigendian_32(statebytes + 28, state[7]);

    //Returnera längden av datan som återstår, programmet är designad så att man paddar indata först när man kommer till chunksen som ska paddas
    return inlen;
}

uint8_t crypto_hashblocks_sha256_g(__local uint8_t *statebytes,
                                   __global uint8_t *in, size_t inlen) {
                                        
    uint32_t state[8];
    uint32_t workingVars[10];

    for(int i = 0; i < 8; i++)
    {
        workingVars[i] = load_bigendian_32(statebytes + i*4);
        state[i] = workingVars[i];
    }

    //Medan det finns chunks som inte är sista chunksen, processera
    while (inlen >= 64) {
        uint32_t words[16];

        for(int i = 0; i < 16; i++)
        {
            words[i] = load_bigendian_32_g(in + i*4);
        }

        int round = 0; 
        for(int j = 0; j < 4; j++)
        {
            for(int i = 0; i < 16; i++)
            {
                F32(workingVars, words[i], roundConstants[round]);
                round += 1;
            }
            if(j < 3)
                EXPANDARRAY
        }
        for(int i = 0; i < 8; i++)
        {
            workingVars[i] += state[i];
            state[i] = workingVars[i];
        }

        in += 64;
        inlen -= 64;
    }

    //Lagra de muterade working state variables till utdata
    store_bigendian_32(statebytes + 0, state[0]);
    store_bigendian_32(statebytes + 4, state[1]);
    store_bigendian_32(statebytes + 8, state[2]);
    store_bigendian_32(statebytes + 12, state[3]);
    store_bigendian_32(statebytes + 16, state[4]);
    store_bigendian_32(statebytes + 20, state[5]);
    store_bigendian_32(statebytes + 24, state[6]);
    store_bigendian_32(statebytes + 28, state[7]);

    for(int i = 0; i < 7; i++)
    {
        printf("%d \n", state[i]);
    }
    //Returnera längden av datan som återstår, programmet är designad så att man paddar indata först när man kommer till chunksen som ska paddas
    return inlen;
}




void sha_inc_init(__local uint8_t *state)
{
    #pragma unroll
    for(int i = 0; i < 32; i++)
    {
        state[i] = iv_256[i];
    }   

    #pragma unroll
    for(int i = 32; i < 40; i++)
    {
        state[i] = 0;
    }
}


void sha_inc_finalize(__global uint8_t *out, __local uint8_t *state, __global uint8_t *in, size_t inlen, __local uint8_t *padded)
{

    uint64_t bytes = load_bigendian_64(state + 32) + inlen;  
    crypto_hashblocks_sha256_g(state, in, inlen);
    
    printf("%d \n", state[0]);

    in += inlen;
    inlen &= 63;
    in -= inlen;
   
    #pragma unroll
    for (size_t i = 0; i < inlen; ++i) {
        padded[i] = in[i];
    }
    
    padded[inlen] = 0x80;


    if (inlen < 56) {
        //Fyll med 0:or
        #pragma unroll
        for (size_t i = inlen + 1; i < 56; ++i) {
            padded[i] = 0;
        }

        //Lägg till längden av indata
        padded[56] = (uint8_t) (bytes >> 53);
        padded[57] = (uint8_t) (bytes >> 45);
        padded[58] = (uint8_t) (bytes >> 37);
        padded[59] = (uint8_t) (bytes >> 29);
        padded[60] = (uint8_t) (bytes >> 21);
        padded[61] = (uint8_t) (bytes >> 13);
        padded[62] = (uint8_t) (bytes >> 5);
        padded[63] = (uint8_t) (bytes << 3);

        //Hasha resten av indata
        crypto_hashblocks_sha256(state, padded, 64);
    }else {
        #pragma unroll
        for (size_t i = inlen + 1; i < 120; ++i) {
            padded[i] = 0;
        }
        padded[120] = (uint8_t) (bytes >> 53);
        padded[121] = (uint8_t) (bytes >> 45);
        padded[122] = (uint8_t) (bytes >> 37);
        padded[123] = (uint8_t) (bytes >> 29);
        padded[124] = (uint8_t) (bytes >> 21);
        padded[125] = (uint8_t) (bytes >> 13);
        padded[126] = (uint8_t) (bytes >> 5);
        padded[127] = (uint8_t) (bytes << 3);
        crypto_hashblocks_sha256(state, padded, 128);
    }
    #pragma unroll
    for (size_t i = 0; i < 32; ++i) {
        out[i] = state[i]; 
    }
}

__kernel void sha256(__global uint8_t *out, __global uint8_t *in, __global size_t *inlen, __local uint8_t *state, __local uint8_t *padded)
{
    sha_inc_init(state);
    sha_inc_finalize(out, state, in, *inlen, padded);	
}





