//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file corebits.c
 * per-core (AP) bitfield manipulation
 */

/*------------- Includes -----------------*/
#include "corebits.h"

/*-- Symbolic Constant Macros (defines) --*/
#define BIT(x)            (1ull << x)
#define BITINDEX(x)       (x / BITTYPE_BITS_PER)
#define BITOFFSET(x)      (x % BITTYPE_BITS_PER)
#define BITMASK(x)        ((1ull << x) - 1)
#define BITTYPE_BITS_LAST (COREBITS_MAX_BITS % BITTYPE_BITS_PER)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

void corebits_clear(corebits_t* corebits)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        corebits->bits[i] = 0;
    }
}

void corebits_clear_bit(corebits_t* corebits, unsigned bit)
{
    if (bit >= COREBITS_MAX_BITS)
    {
        return;
    }
    corebits->bits[BITINDEX(bit)] &= (~BIT(BITOFFSET(bit)));
}

void corebits_set_bit(corebits_t* corebits, unsigned bit)
{
    if (bit >= COREBITS_MAX_BITS)
    {
        return;
    }
    corebits->bits[BITINDEX(bit)] |= BIT(BITOFFSET(bit));
}

void corebits_and(corebits_t* corebits, const corebits_t* andbits)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        corebits->bits[i] &= andbits->bits[i];
    }
}

void corebits_or(corebits_t* corebits, const corebits_t* orbits)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        corebits->bits[i] |= orbits->bits[i];
    }
}

void corebits_inv(corebits_t* corebits)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        corebits->bits[i] = ~corebits->bits[i];
        if (i == BITTYPE_COUNT - 1)
        {
            corebits->bits[i] &= BITMASK(BITTYPE_BITS_LAST);
        }
    }
}

bool corebits_is_bit_set(const corebits_t* corebits, unsigned bit)
{
    if (bit >= COREBITS_MAX_BITS)
    {
        return 0;
    }
    return (0 != (corebits->bits[BITINDEX(bit)] & BIT(BITOFFSET(bit))));
}

bool corebits_is_equal(const corebits_t* corebits1, const corebits_t* corebits2)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        BITTYPE mask = (i == BITTYPE_COUNT - 1) ? BITMASK(BITTYPE_BITS_LAST) : BITMASK(BITTYPE_BITS_PER);
        if ((corebits1->bits[i] & mask) != (corebits2->bits[i] & mask))
        {
            return false;
        }
    }
    return true;
}

bool corebits_is_clear(const corebits_t* corebits)
{
    for (unsigned i = 0; i < BITTYPE_COUNT; ++i)
    {
        if (corebits->bits[i] != 0)
        {
            return false;
        }
    }
    return true;
}

int corebits_first(const corebits_t* corebits)
{
    for (int i = BITTYPE_COUNT - 1; i >= 0; --i)
    {
        BITTYPE mask = (i == BITTYPE_COUNT - 1) ? BITMASK(BITTYPE_BITS_LAST) : BITMASK(BITTYPE_BITS_PER);
        const BITTYPE bits = corebits->bits[i] & mask;
        if (bits != 0)
        {
            unsigned bit = __builtin_clz(bits);
            return i * BITTYPE_BITS_PER + ((BITTYPE_BITS_PER - 1) - bit);
        }
    }
    return -1;
}

int corebits_count(const corebits_t* corebits)
{
    int count = 0;
    for (int i = BITTYPE_COUNT - 1; i >= 0; --i)
    {
        BITTYPE mask = (i == BITTYPE_COUNT - 1) ? BITMASK(BITTYPE_BITS_LAST) : BITMASK(BITTYPE_BITS_PER);
        const BITTYPE bits = corebits->bits[i] & mask;

        count += __builtin_popcount(bits);
    }
    return count;
}
