#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <string>

int arrayBitSize(char* message) {
    int size = 0;

    while(message[size] != '\0') {
        //std::cout<<message[size];
        size++;
    }

    return size*8;    
}

int findSmallestMultiple512(int messageBits) {
    int n = 0;
    //Find multiple n
    while(messageBits > 512) {
        messageBits -= 512;
        n++;
    }
    
    //Account for required padding space
    if(messageBits > (512-64-8)) {
        n += 2;
    } else {
        n++;
    }

    return n;
}

uint32_t[] createPaddedMessage(char *message, int messageBits) {
    int n = findSmallestMultiple512(messageBits);

    int paddedMessageSize = n*(512/32);
    //16 words per 512 bit chunk
    uint32_t paddedMessage[paddedMessageSize];
    
    for(int i = 0; i < paddedMessageSize; i++) {
        uint8_t buf[4];

        for(int j = 0; j < 4; j++) {
            if(message == '\0') {

                //Fill out the rest of the buffer
                if(j < 4) {
                    buf[j] = 0x80;
                    j++;
                }
                while(j < 4) {
                    buf[j] = 0x00;
                    j++;
                }

                break;
            }

            buf[j] = (uint8_t)message;
            message++;
        }

    }

    message[startingIndex] = 'a';
    if(message[startingIndex] == '\0') {
        std::cout<<message;
    }
    
    for(int i = 1; i < paddingBitsArea-(64/8); i++) {

    }
}

int SHA256(char* message) {
    //Init
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    //std::cout<<sizeof(&message[0]);
    //std::cout<<sizeof(message[2]);
    uint32_t messageBits = arrayBitSize(message);
    //std::cout<<messageBits;
    padMessage(message, messageBits);
    
    

    return 0;
}  

int main(int argc, char **argv) {

    char message[] = "1";
    SHA256(message);

    return 0;
}