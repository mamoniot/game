/////////////////////////////////////////////////////////////
// NOTICE: This file was heavily modified by Monica Moniot //
// Thank you to the original author, Melissa O'Neill       //
/////////////////////////////////////////////////////////////
/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */
 ////////////////////////////////////////////////////////////////////////////
 // Just #include this header file and #define PCG_IMPLEMENTATION
 // once to use this library.
 // ~~~LIBRARY CONTENTS~~~
 // Macro Options:
 //     PCG_STATIC - #define this to force the library to compile staticly
 //     PCG_NO_STDINT - #define this to disable the use of stdint.h
 //     PCG_INT32 - #define this to replace the default 32-bit integer type
 //     PCG_UINT32 - #define this to replace the 32-bit unsigned int integer type
 //     PCG_UINT64 - #define this to replace the 64-bit unsigned int integer type
 //
 // PCG:
 //     A struct that contains the entire state of a pcg random number generator
 //
 // pcg_seeds(rng, initstate, initseq):
 //     Seed the rng, either seeds or seed must be called to initialize a rng
 //     Specified in two parts, state initializer and a sequence selection
 //     constant/stream id (determines the sequence of random numbers produced)
 //     The stream id is kept constant while the state initializer is variable
 // pcg_seed(rng, seed):
 //     Sets both the state initializer and stream id to the given seed
 //
 // pcg_random(rng):
 //     Generate a uniformly distributed 32-bit random integer
 //
 // pcg_advance(rng, delta):
 //     Advances the rng stream by delta steps in log time
 //     So pcg_advance(rng, 3) behaves as though pcg_random(rng) was called 3 times
 //     Also takes negative numbers to reverse the rng stream
 //
 // pcg_random_in(rng, lower, upper):
 //     Generate a uniformly distributed int32, r, where lower <= r <= upper
 // pcg_random_uniform(rng):
 //     Generate a uniformly distributed float, r, where 0 <= r < 1
 // pcg_random_uniform_in(rng):
 //     Generate a uniformly distributed float, r, where 0 <= r <= 1
 // pcg_random_uniform_ex(rng):
 //     Generate a uniformly distributed float, r, where 0 < r < 1
 //
 // PCGF:
 //     A PCG_UINT64 that contains the entire state of a xorshift* rng
 //     Works identically to a normal pcg rng, except it is smaller and
 //     faster, producing only slightly lower quality randomness
 //     To get the PCGF variant of any particular function, prefix it with "pcgf"
 //     (there does not exist a pcgf_seeds or pcgf_advance function)
 //
 // pcgf_hash:
 //     A stateless version of a PCGF designed for hashing 64-bit integers.
 //     These functions take 64-bit integers as input instead of a PCGF*.
 //     To get the pcgf_hash variant of a function, prefix it with "pcgf_hash"
 //
 // *** THIS LIBRARY IS NOT CRYPTOGRAPHICALLY SECURE ***

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INCLUDE__PCG_H
#define INCLUDE__PCG_H


#define PCG__DECLS static inline
#ifdef PCG_STATIC
 #define PCG__DECLR static
#else
 #define PCG__DECLR
#endif

#ifdef PCG_NO_STDINT
 #ifndef PCG_INT32
  #define PCG_INT32 int
 #endif
 #ifndef PCG_UINT32
  #define PCG_UINT32 unisgned int
 #endif
 #ifndef PCG_UINT64
  #define PCG_UINT64 unsigned long long
 #endif
#else
 #include <stdint.h>
 #ifndef PCG_INT32
  #define PCG_INT32 int32_t
 #endif
 #ifndef PCG_UINT32
  #define PCG_UINT32 uint32_t
 #endif
 #ifndef PCG_UINT64
  #define PCG_UINT64 uint64_t
 #endif
#endif

typedef struct PCG {// Only modify the internals if you are a stats PhD.
    PCG_UINT64 state;             // RNG state.  All values are possible.
    PCG_UINT64 inc;               // Controls which RNG sequence (stream) is
                                // selected. Must *always* be odd.
} PCG;
typedef PCG_UINT64 PCGF;

// If you *must* statically initialize it, use this.
#define PCG_INITIALIZER {0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL}
#define PCGF_INITIALIZER 0xda3e39cb94b95bdbULL


PCG__DECLR void pcg_seeds(PCG* rng, PCG_UINT64 initstate, PCG_UINT64 sequence);

PCG__DECLR PCG_UINT32 pcg_random(PCG* rng);

PCG__DECLR void pcg_advance(PCG* rng, PCG_UINT64 delta);

PCG__DECLR PCG_INT32 pcg_random_in(PCG* rng, PCG_INT32 lower, PCG_INT32 upper);



PCG__DECLS PCG_UINT64 pcg__advance_lcg_64(PCG_UINT64 state, PCG_UINT64 delta, PCG_UINT64 cur_mult, PCG_UINT64 cur_plus) {
    PCG_UINT64 acc_mult = 1u;
    PCG_UINT64 acc_plus = 0u;
    while (delta > 0) {
        if (delta & 1) {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta /= 2;
    }
    return acc_mult * state + acc_plus;
}

PCG__DECLS void pcg_seed(PCG* rng, PCG_UINT64 seed) {
    pcg_seeds(rng, seed, seed);
}


#define PCG_INTMAX ((float)(1ll<<32))
PCG__DECLS float pcg_random_uniform(PCG* rng) {
	return pcg_random(rng)*(1/PCG_INTMAX);
}
PCG__DECLS float pcg_random_uniform_in(PCG* rng) {
	return ((float)pcg_random(rng))/(PCG_INTMAX - 1);
}
PCG__DECLS float pcg_random_uniform_ex(PCG* rng) {
	return (((float)pcg_random(rng)) + 1)/(PCG_INTMAX + 1);
}



PCG__DECLR PCG_UINT64 pcgf__hash64(PCG_UINT64 state);
PCG__DECLS PCG_UINT32 pcgf_hash(PCG_UINT64 state) {
	return (PCG_UINT32)pcgf__hash64(state);
}

PCG__DECLS PCG_INT32 pcgf_hash_in(PCG_UINT64 state, PCG_INT32 lower, PCG_INT32 upper) {
	return (pcgf_hash(state)%(upper - lower + 1)) + lower;
}
PCG__DECLS float pcgf_hash_uniform(PCG_UINT64 state) {
	return ((float)pcgf_hash(state))*(1/PCG_INTMAX);
}
PCG__DECLS float pcgf_hash_uniform_in(PCG_UINT64 state) {
	return ((float)pcgf_hash(state))/(PCG_INTMAX - 1);
}
PCG__DECLS float pcgf_hash_uniform_ex(PCG_UINT64 state) {
	return (((float)pcgf_hash(state)) + 1)/(PCG_INTMAX + 1);
}



PCG__DECLS void pcgf_seed(PCGF* rng, PCG_UINT64 seed) {
	*rng = (seed ? seed : PCGF_INITIALIZER);
}
PCG__DECLS PCG_UINT32 pcgf_random(PCGF* rng) {
	*rng = pcgf__hash64(*rng);
	return (PCG_UINT32)(*rng >> 32);
}


PCG__DECLS PCG_INT32 pcgf_random_in(PCGF* rng, PCG_INT32 lower, PCG_INT32 upper) {
	return (pcgf_random(rng)%(upper - lower + 1)) + lower;
}
PCG__DECLS float pcgf_random_uniform(PCGF* rng) {
	return ((float)pcgf_random(rng))*(1/PCG_INTMAX);
}
PCG__DECLS float pcgf_random_uniform_in(PCGF* rng) {
	return ((float)pcgf_random(rng))/(PCG_INTMAX - 1);
}
PCG__DECLS float pcgf_random_uniform_ex(PCGF* rng) {
	return (((float)pcgf_random(rng)) + 1)/(PCG_INTMAX + 1);
}

#endif

#ifdef PCG_IMPLEMENTATION
#undef PCG_IMPLEMENTATION

PCG_UINT32 pcg_random(PCG* rng) {
    PCG_UINT64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    PCG_UINT32 xorshifted = (((oldstate >> 18u) ^ oldstate) >> 27u);
    PCG_UINT32 rot = (oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << (((PCG_UINT32)(-(PCG_INT32)rot)) & 31));
}

void pcg_seeds(PCG* rng, PCG_UINT64 initstate, PCG_UINT64 sequence) {
    rng->state = 0U;
    rng->inc = (sequence << 1u) | 1u;
    pcg_random(rng);
    rng->state += initstate;
    pcg_random(rng);
}

void pcg_advance(PCG* rng, PCG_UINT64 delta) {
    rng->state = pcg__advance_lcg_64(rng->state, delta, 6364136223846793005ULL, rng->inc);
}

PCG_INT32 pcg_random_in(PCG* rng, PCG_INT32 lower, PCG_INT32 upper) {
	PCG_INT32 bound = upper - lower + 1;
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     PCG_INT32 threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     PCG_INT32 threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    PCG_INT32 threshold = (-bound) % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        PCG_INT32 r = pcg_random(rng);
        if (r >= threshold)
            return (r % bound) + lower;
    }
}

PCG_UINT64 pcgf__hash64(PCG_UINT64 state) {
	/* Algorithm "xorshift*" from Marsaglia, "Xorshift RNGs" */
	state ^= state >> 12; // a
	state ^= state << 25; // b
	state ^= state >> 27; // c
	return state*0x2545F4914F6CDD1Dull;
}

#endif

#ifdef __cplusplus
}
#endif
