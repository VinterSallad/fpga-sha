/* Based on the public domain implementation in
 * crypto_hash/keccakc512/simple/ from http://bench.cr.yp.to/supercop.html
 * by Ronny Van Keer
 * and the public domain "TweetFips202" implementation
 * from https://twitter.com/tweetfips202
 * by Gilles Van Assche, Daniel J. Bernstein, and Peter Schwabe */

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <iostream>

//Antal rundor som sker vid varje f där 5 grekiska funktioner exekveras per runda
#define NROUNDS 24

//Rotate Left a med offset
#define ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

#define SHAKE256_RATE 136

//Skapa en 64 bitars integer från 8 bitars integer
/*************************************************
 * Name:        load64
 *
 * Description: Load 8 bytes into uint64_t in little-endian order
 *
 * Arguments:   - const uint8_t *x: pointer to input byte array
 *
 * Returns the loaded 64-bit unsigned integer
 **************************************************/
static uint64_t load64(const uint8_t *x) {
    uint64_t r = 0;
    for (size_t i = 0; i < 8; ++i) {
        r |= (uint64_t)x[i] << 8 * i;
    }

    return r;
}

//Skapa 8 bitars integer från 64 bitars integer
/*************************************************
 * Name:        store64
 *
 * Description: Store a 64-bit integer to a byte array in little-endian order
 *
 * Arguments:   - uint8_t *x: pointer to the output byte array
 *              - uint64_t u: input 64-bit unsigned integer
 **************************************************/
static void store64(uint8_t *x, uint64_t u) {
    for (size_t i = 0; i < 8; ++i) {
        x[i] = (uint8_t) (u >> 8 * i);
    }
}

//24 konstanter för de 24 rundor
/* Keccak round constants */
static const uint64_t KeccakF_RoundConstants[NROUNDS] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

//Permuterar/Mutera rektangelns state (1600-bitar), dvs. exekvera de 24 rundor på rektangeln
/*************************************************
 * Name:        KeccakF1600_StatePermute
 *
 * Description: The Keccak F1600 Permutation
 *
 * Arguments:   - uint64_t *state: pointer to input/output Keccak state
 **************************************************/
static void KeccakF1600_StatePermute(uint64_t *state) {
    int round;

    //Naming convention, a,e,i,o,u är vänster till höger, b,g,k,m,s är uppåt till neråt, 
    //var varsam att Aba är mitten av rektangeln på grund av hur indexering av rektangeln ser ut. Tänk på tabellen i rapporten och hur det korresponderar till state[i]
    //Bra att veta, designen allmänt nedan är att exekvera alla t.ex. XOR samtidigt, så var beredd att tänka på den stora bilden snarare än beräkningen för en singulär bit på rektangeln

    //64 bitars lanes i rektangeln
    uint64_t Aba, Abe, Abi, Abo, Abu;
    uint64_t Aga, Age, Agi, Ago, Agu;
    uint64_t Aka, Ake, Aki, Ako, Aku;
    uint64_t Ama, Ame, Ami, Amo, Amu;
    uint64_t Asa, Ase, Asi, Aso, Asu;

    //Temporära variabler

    //Temporära variabler för raden man arbetar på
    uint64_t BCa, BCe, BCi, BCo, BCu;
    //Theta, där man sparar 2 kolumners XOR med varandra
    uint64_t Da, De, Di, Do, Du;

    uint64_t Eba, Ebe, Ebi, Ebo, Ebu;
    uint64_t Ega, Ege, Egi, Ego, Egu;
    uint64_t Eka, Eke, Eki, Eko, Eku;
    uint64_t Ema, Eme, Emi, Emo, Emu;
    uint64_t Esa, Ese, Esi, Eso, Esu;

    //Ladda in rektangeln
    // copyFromState(A, state)
    Aba = state[0];
    Abe = state[1];
    Abi = state[2];
    Abo = state[3];
    Abu = state[4];
    Aga = state[5];
    Age = state[6];
    Agi = state[7];
    Ago = state[8];
    Agu = state[9];
    Aka = state[10];
    Ake = state[11];
    Aki = state[12];
    Ako = state[13];
    Aku = state[14];
    Ama = state[15];
    Ame = state[16];
    Ami = state[17];
    Amo = state[18];
    Amu = state[19];
    Asa = state[20];
    Ase = state[21];
    Asi = state[22];
    Aso = state[23];
    Asu = state[24];

    //Exekvera alla 24 rundor, dvs de grekiska funktionerna, loop unfoldad så att 2 rundor exekveras per iteratiion
    for (round = 0; round < NROUNDS; round += 2) {
        //    prepareTheta
        //Räkna ut alla vertikala kolumners XOR, BCa första biten kan tänkas vara paritet av kolumnen till vänster längst fram i rektangeln osv.
        BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
        BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
        BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
        BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
        BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

        // thetaRhoPiChiIotaPrepareTheta(round  , A, E)

        //theta funktionen, "Di" andra biten är då kolumnen till vänster XOR med kolumnen till höger 1 steg inåt, (tänk illustrationen för theta)
        Da = BCu ^ ROL(BCe, 1);
        De = BCa ^ ROL(BCi, 1);
        Di = BCe ^ ROL(BCo, 1);
        Do = BCi ^ ROL(BCu, 1);
        Du = BCo ^ ROL(BCa, 1);

        //Samtliga funktioner exekveras samtidigt
        //Designen är att man beräknar det som krävs för att ta fram hela raden för nästa state innan man går till nästa rad.

        //Mittersta raden

        //Applicera theta (gäller samtliga ^= Dx)
        Aba ^= Da;

        //Spara till temporär variabel (special fall då Aba är mittersta lanen)
        BCa = Aba;

        //Theta, pi (lanes flyttas) sker implicit också här då man väljer att arbeta på lanen som kommer flyttas till raden man arbetar på, tänk figuren för pi
        Age ^= De;

        //rho (Gäller samtliga ROL), andra steg där man roterar lanen, lägg märke till att om en lane inte använder sig av ROL så betyder det att den roteras med 0.
        BCe = ROL(Age, 44);

        Aki ^= Di;
        BCi = ROL(Aki, 43);
        Amo ^= Do;
        BCo = ROL(Amo, 21);
        Asu ^= Du;
        BCu = ROL(Asu, 14);

        //Börja assigna nästa states rad, "Eyx" är nästa state på rad y.
        //Chi, näst sista steget där man tar och XOR:ar med 2 bitar till höger (mod 5)
        Eba = BCa ^ ((~BCe) & BCi);
        //iota, sista steget där man XOR centrala lanen med round constant, lägg märke till att övriga rader inte har denna beräkning
        Eba ^= KeccakF_RoundConstants[round];
        Ebe = BCe ^ ((~BCi) & BCo);
        Ebi = BCi ^ ((~BCo) & BCu);
        Ebo = BCo ^ ((~BCu) & BCa);
        Ebu = BCu ^ ((~BCa) & BCe);

        //Nästa rad samma sak
        Abo ^= Do;
        BCa = ROL(Abo, 28);
        Agu ^= Du;
        BCe = ROL(Agu, 20);
        Aka ^= Da;
        BCi = ROL(Aka, 3);
        Ame ^= De;
        BCo = ROL(Ame, 45);
        Asi ^= Di;
        BCu = ROL(Asi, 61);
        Ega = BCa ^ ((~BCe) & BCi);
        Ege = BCe ^ ((~BCi) & BCo);
        Egi = BCi ^ ((~BCo) & BCu);
        Ego = BCo ^ ((~BCu) & BCa);
        Egu = BCu ^ ((~BCa) & BCe);

        //Skrivet om så att det logiskt makear mer sense
        //För varje rad på rektangeln
        /*
            //Applicera theta som förberäknats ovan samtidigt som pi sker
            Abo ^= Do;
            Agu ^= Du;
            Aka ^= Da;
            Ame ^= De;
            Asi ^= Di;

            //Applicera rho
            BCa = ROL(Abo, 28);
            BCe = ROL(Agu, 20);
            BCi = ROL(Aka, 3);
            BCo = ROL(Ame, 45);
            BCu = ROL(Asi, 61);

            //Applicera chi
            Ega = BCa ^ ((~BCe) & BCi);
            Ege = BCe ^ ((~BCi) & BCo);
            Egi = BCi ^ ((~BCo) & BCu);
            Ego = BCo ^ ((~BCu) & BCa);
            Egu = BCu ^ ((~BCa) & BCe);

            //Applicera iota bara om det är mittersta raden
        */

        Abe ^= De;
        BCa = ROL(Abe, 1);
        Agi ^= Di;
        BCe = ROL(Agi, 6);
        Ako ^= Do;
        BCi = ROL(Ako, 25);
        Amu ^= Du;
        BCo = ROL(Amu, 8);
        Asa ^= Da;
        BCu = ROL(Asa, 18);
        Eka = BCa ^ ((~BCe) & BCi);
        Eke = BCe ^ ((~BCi) & BCo);
        Eki = BCi ^ ((~BCo) & BCu);
        Eko = BCo ^ ((~BCu) & BCa);
        Eku = BCu ^ ((~BCa) & BCe);

        Abu ^= Du;
        BCa = ROL(Abu, 27);
        Aga ^= Da;
        BCe = ROL(Aga, 36);
        Ake ^= De;
        BCi = ROL(Ake, 10);
        Ami ^= Di;
        BCo = ROL(Ami, 15);
        Aso ^= Do;
        BCu = ROL(Aso, 56);
        Ema = BCa ^ ((~BCe) & BCi);
        Eme = BCe ^ ((~BCi) & BCo);
        Emi = BCi ^ ((~BCo) & BCu);
        Emo = BCo ^ ((~BCu) & BCa);
        Emu = BCu ^ ((~BCa) & BCe);

        Abi ^= Di;
        BCa = ROL(Abi, 62);
        Ago ^= Do;
        BCe = ROL(Ago, 55);
        Aku ^= Du;
        BCi = ROL(Aku, 39);
        Ama ^= Da;
        BCo = ROL(Ama, 41);
        Ase ^= De;
        BCu = ROL(Ase, 2);
        Esa = BCa ^ ((~BCe) & BCi);
        Ese = BCe ^ ((~BCi) & BCo);
        Esi = BCi ^ ((~BCo) & BCu);
        Eso = BCo ^ ((~BCu) & BCa);
        Esu = BCu ^ ((~BCa) & BCe);


        //Samma som ovan men nästa iteration nu när "Eyx" är en muterad state av "Ayx", kommer mutera "Ayx" baserad på "Eyx" vilket förs över till nästa iteration...

        //    prepareTheta
        BCa = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
        BCe = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
        BCi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
        BCo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
        BCu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

        // thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
        Da = BCu ^ ROL(BCe, 1);
        De = BCa ^ ROL(BCi, 1);
        Di = BCe ^ ROL(BCo, 1);
        Do = BCi ^ ROL(BCu, 1);
        Du = BCo ^ ROL(BCa, 1);

        Eba ^= Da;
        BCa = Eba;
        Ege ^= De;
        BCe = ROL(Ege, 44);
        Eki ^= Di;
        BCi = ROL(Eki, 43);
        Emo ^= Do;
        BCo = ROL(Emo, 21);
        Esu ^= Du;
        BCu = ROL(Esu, 14);
        Aba = BCa ^ ((~BCe) & BCi);
        Aba ^= KeccakF_RoundConstants[round + 1];
        Abe = BCe ^ ((~BCi) & BCo);
        Abi = BCi ^ ((~BCo) & BCu);
        Abo = BCo ^ ((~BCu) & BCa);
        Abu = BCu ^ ((~BCa) & BCe);

        Ebo ^= Do;
        BCa = ROL(Ebo, 28);
        Egu ^= Du;
        BCe = ROL(Egu, 20);
        Eka ^= Da;
        BCi = ROL(Eka, 3);
        Eme ^= De;
        BCo = ROL(Eme, 45);
        Esi ^= Di;
        BCu = ROL(Esi, 61);
        Aga = BCa ^ ((~BCe) & BCi);
        Age = BCe ^ ((~BCi) & BCo);
        Agi = BCi ^ ((~BCo) & BCu);
        Ago = BCo ^ ((~BCu) & BCa);
        Agu = BCu ^ ((~BCa) & BCe);

        Ebe ^= De;
        BCa = ROL(Ebe, 1);
        Egi ^= Di;
        BCe = ROL(Egi, 6);
        Eko ^= Do;
        BCi = ROL(Eko, 25);
        Emu ^= Du;
        BCo = ROL(Emu, 8);
        Esa ^= Da;
        BCu = ROL(Esa, 18);
        Aka = BCa ^ ((~BCe) & BCi);
        Ake = BCe ^ ((~BCi) & BCo);
        Aki = BCi ^ ((~BCo) & BCu);
        Ako = BCo ^ ((~BCu) & BCa);
        Aku = BCu ^ ((~BCa) & BCe);

        Ebu ^= Du;
        BCa = ROL(Ebu, 27);
        Ega ^= Da;
        BCe = ROL(Ega, 36);
        Eke ^= De;
        BCi = ROL(Eke, 10);
        Emi ^= Di;
        BCo = ROL(Emi, 15);
        Eso ^= Do;
        BCu = ROL(Eso, 56);
        Ama = BCa ^ ((~BCe) & BCi);
        Ame = BCe ^ ((~BCi) & BCo);
        Ami = BCi ^ ((~BCo) & BCu);
        Amo = BCo ^ ((~BCu) & BCa);
        Amu = BCu ^ ((~BCa) & BCe);

        Ebi ^= Di;
        BCa = ROL(Ebi, 62);
        Ego ^= Do;
        BCe = ROL(Ego, 55);
        Eku ^= Du;
        BCi = ROL(Eku, 39);
        Ema ^= Da;
        BCo = ROL(Ema, 41);
        Ese ^= De;
        BCu = ROL(Ese, 2);
        Asa = BCa ^ ((~BCe) & BCi);
        Ase = BCe ^ ((~BCi) & BCo);
        Asi = BCi ^ ((~BCo) & BCu);
        Aso = BCo ^ ((~BCu) & BCa);
        Asu = BCu ^ ((~BCa) & BCe);
    }

    //Spara nu permuterade rektangeln
    // copyToState(state, A)
    state[0] = Aba;
    state[1] = Abe;
    state[2] = Abi;
    state[3] = Abo;
    state[4] = Abu;
    state[5] = Aga;
    state[6] = Age;
    state[7] = Agi;
    state[8] = Ago;
    state[9] = Agu;
    state[10] = Aka;
    state[11] = Ake;
    state[12] = Aki;
    state[13] = Ako;
    state[14] = Aku;
    state[15] = Ama;
    state[16] = Ame;
    state[17] = Ami;
    state[18] = Amo;
    state[19] = Amu;
    state[20] = Asa;
    state[21] = Ase;
    state[22] = Asi;
    state[23] = Aso;
    state[24] = Asu;
}


/*************************************************
 * Name:        keccak_absorb
 *
 * Description: Absorb step of Keccak;
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - uint64_t *s: pointer to (uninitialized) output Keccak state
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *m: pointer to input to be absorbed into s
 *              - size_t mlen: length of input in bytes
 *              - uint8_t p: domain-separation byte for different
 *                                 Keccak-derived functions
 **************************************************/
static void keccak_absorb(uint64_t *s, uint32_t r, const uint8_t *m,
                          size_t mlen, uint8_t p) {
    size_t i;
    uint8_t t[200];

    //Fyll rektangeln med 0:or
    /* Zero state */
    for (i = 0; i < 25; ++i) {
        s[i] = 0;
    }

    //Medan storleken av input fortfarande går att mata in i muterings algoritm f (r = rate/bitrate)
    while (mlen >= r) {
        //Ladda in en chunk av bitrate i state med XOR som i figuren, resten av rektangeln är 0:or (capacity) 
        for (i = 0; i < r / 8; ++i) {
            s[i] ^= load64(m + 8 * i);
        }

        //Permutera rektangeln
        KeccakF1600_StatePermute(s);

        //Iterera framåt
        mlen -= r;
        m += r;
    }

    //Förbered sista chunken
    for (i = 0; i < r; ++i) {
        t[i] = 0;
    }
    //Lägg in resten av indatan
    for (i = 0; i < mlen; ++i) {
        t[i] = m[i];
    }
    //Padda (format:    p ... 0 ... 1   oftast  1 ... 0 ... 1)
    t[i] = p; //Special padding som identifierar vilken keccakvariant
    //Padding i SHA3 slutar med en 1:a (hela algoritmen är little endian, därför 128)
    t[r - 1] |= 128;
    //Ladda/XOR:a in sista paddade chunken, algoritmen är nu redo att börja squeeza!
    for (i = 0; i < r / 8; ++i) {
        s[i] ^= load64(t + 8 * i);
    }
}

/*************************************************
 * Name:        keccak_squeezeblocks
 *
 * Description: Squeeze step of Keccak. Squeezes full blocks of r bytes each.
 *              Modifies the state. Can be called multiple times to keep
 *              squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *h: pointer to output blocks
 *              - size_t nblocks: number of blocks to be
 *                                                squeezed (written to h)
 *              - uint64_t *s: pointer to input/output Keccak state
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 **************************************************/
static void keccak_squeezeblocks(uint8_t *h, size_t nblocks,
                                 uint64_t *s, uint32_t r) {
    //Medans antalet squeeze inte är uppfyllt
    while (nblocks > 0) {
        //Permutera rektangeln
        KeccakF1600_StatePermute(s);

        //Extrahera bitrate som ska bli hash till h (r bitshiftad till höger då det är lika med r/8)
        for (size_t i = 0; i < (r >> 3); i++) {
            store64(h + 8 * i, s[i]);
        }
        
        //Iterera framåt
        h += r;
        nblocks--;
    }
}

        // samtliga under är "public funktioner", 
        //hjälper att gömma konstanter som Bitrate, 
        //jag har tagit bort varianterna vi inte bryr oss om som SHAKE128 och SHA3, 
        //programmet är designad så att funktionerna ovan kan användas till många varianter, därför är det så här.
        //Tänk API för programmet

/*************************************************
 * Name:        shake256_absorb
 *
 * Description: Absorb step of the SHAKE256 XOF.
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - uint64_t *s: pointer to (uninitialized) output Keccak state
 *              - const uint8_t *input: pointer to input to be absorbed
 *                                            into s
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256_absorb(uint64_t *s, const uint8_t *input, size_t inlen) {
    keccak_absorb(s, SHAKE256_RATE, input, inlen, 0x1F);
}

/*************************************************
 * Name:        shake256_squeezeblocks
 *
 * Description: Squeeze step of SHAKE256 XOF. Squeezes full blocks of
 *              SHAKE256_RATE bytes each. Modifies the state. Can be called
 *              multiple times to keep squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *output: pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed
 *                                (written to output)
 *              - uint64_t *s: pointer to input/output Keccak state
 **************************************************/
void shake256_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *s) {
    keccak_squeezeblocks(output, nblocks, s, SHAKE256_RATE);
}

//MAIN DRIVER
/*************************************************
 * Name:        shake256
 *
 * Description: SHAKE256 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *output: pointer to output
 *              - size_t outlen: requested output length in bytes
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen) {
    
    //Beräkna hur många gånger den behöver squeeza för att ta fram önskad output length
    size_t nblocks = outlen / SHAKE256_RATE;

    uint8_t t[SHAKE256_RATE];
    uint64_t s[25];

    //Absorbera data tills den är redo att squeezas
    shake256_absorb(s, input, inlen);
    //Ta fram hashen till output, funktionen gör inget om nblocks = 0, dvs att en funktionskallelse är tillräckligt för att ta fram hashen.
    //SIDENOTE: Designad så att funktionen här squeezar till output ända frams till sista squeezen som hanteras nedan.
    shake256_squeezeblocks(output, nblocks, s);

    //Gå framåt i output för att göra sista squeezen
    output += nblocks * SHAKE256_RATE;
    //Ta bort längden som redan squeezats
    outlen -= nblocks * SHAKE256_RATE;

    //Sista squeeze om det krävs
    if (outlen) {
        shake256_squeezeblocks(t, 1, s);
        for (size_t i = 0; i < outlen; ++i) {
            output[i] = t[i];
            
        }
    }
}


/*
MAIN
MAIN
*/

int main(int argc, char const *argv[])
{
    size_t outlen = 32;
    uint8_t hash[outlen];

    uint8_t input[] = {
        'f', 'i', 's', 'h', 'e', 's'
    };

    size_t inputLength = sizeof(input)/sizeof(input[0]);

    //printf("%ld", inputLength);

    shake256(hash, outlen, input, inputLength);

    for(int i=0; i<32; i++) {
        printf("%02x", hash[i]);
    }

    return 0;
}


/*
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
ALLT HÄR UNDER ÄR INKREMENTELLA VARIANTEN
*/

//Inkrementella variantens funktioner

        //Intialisera rektangeln
/*************************************************
 * Name:        keccak_inc_init
 *
 * Description: Initializes the incremental Keccak state to zero.
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 **************************************************/
static void keccak_inc_init(uint64_t *s_inc) {
    size_t i;

    for (i = 0; i < 25; ++i) {
        s_inc[i] = 0;
    }
    s_inc[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_absorb
 *
 * Description: Incremental keccak absorb
 *              Preceded by keccak_inc_init, succeeded by keccak_inc_finalize
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *m: pointer to input to be absorbed into s
 *              - size_t mlen: length of input in bytes
 **************************************************/
static void keccak_inc_absorb(uint64_t *s_inc, uint32_t r, const uint8_t *m,
                              size_t mlen) {
    size_t i;

    /* Recall that s_inc[25] is the non-absorbed bytes xored into the state */
    while (mlen + s_inc[25] >= r) {
        for (i = 0; i < r - s_inc[25]; i++) {
            /* Take the i'th byte from message
               xor with the s_inc[25] + i'th byte of the state; little-endian */
            s_inc[(s_inc[25] + i) >> 3] ^= (uint64_t)m[i] << (8 * ((s_inc[25] + i) & 0x07));
        }
        mlen -= (size_t)(r - s_inc[25]);
        m += r - s_inc[25];
        s_inc[25] = 0;

        KeccakF1600_StatePermute(s_inc);
    }

    for (i = 0; i < mlen; i++) {
        s_inc[(s_inc[25] + i) >> 3] ^= (uint64_t)m[i] << (8 * ((s_inc[25] + i) & 0x07));
    }
    s_inc[25] += mlen;
}

/*************************************************
 * Name:        keccak_inc_finalize
 *
 * Description: Finalizes Keccak absorb phase, prepares for squeezing
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - uint8_t p: domain-separation byte for different
 *                                 Keccak-derived functions
 **************************************************/
static void keccak_inc_finalize(uint64_t *s_inc, uint32_t r, uint8_t p) {
    /* After keccak_inc_absorb, we are guaranteed that s_inc[25] < r,
       so we can always use one more byte for p in the current state. */
    s_inc[s_inc[25] >> 3] ^= (uint64_t)p << (8 * (s_inc[25] & 0x07));
    s_inc[(r - 1) >> 3] ^= (uint64_t)128 << (8 * ((r - 1) & 0x07));
    s_inc[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_squeeze
 *
 * Description: Incremental Keccak squeeze; can be called on byte-level
 *
 * Arguments:   - uint8_t *h: pointer to output bytes
 *              - size_t outlen: number of bytes to be squeezed
 *              - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 **************************************************/
static void keccak_inc_squeeze(uint8_t *h, size_t outlen,
                               uint64_t *s_inc, uint32_t r) {
    size_t i;

    /* First consume any bytes we still have sitting around */
    for (i = 0; i < outlen && i < s_inc[25]; i++) {
        /* There are s_inc[25] bytes left, so r - s_inc[25] is the first
           available byte. We consume from there, i.e., up to r. */
        h[i] = (uint8_t)(s_inc[(r - s_inc[25] + i) >> 3] >> (8 * ((r - s_inc[25] + i) & 0x07)));
    }
    h += i;
    outlen -= i;
    s_inc[25] -= i;

    /* Then squeeze the remaining necessary blocks */
    while (outlen > 0) {
        KeccakF1600_StatePermute(s_inc);

        for (i = 0; i < outlen && i < r; i++) {
            h[i] = (uint8_t)(s_inc[i >> 3] >> (8 * (i & 0x07)));
        }
        h += i;
        outlen -= i;
        s_inc[25] = r - i;
    }
}

void shake256_inc_init(uint64_t *s_inc) {
    keccak_inc_init(s_inc);
}

void shake256_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(s_inc, SHAKE256_RATE, input, inlen);
}

void shake256_inc_finalize(uint64_t *s_inc) {
    keccak_inc_finalize(s_inc, SHAKE256_RATE, 0x1F);
}

void shake256_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *s_inc) {
    keccak_inc_squeeze(output, outlen, s_inc, SHAKE256_RATE);
}