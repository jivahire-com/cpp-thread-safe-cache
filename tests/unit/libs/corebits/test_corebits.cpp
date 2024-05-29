//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_corebits.cpp
 * Corebits tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <stddef.h>

extern "C" {
#include <corebits.h>

/*-- Symbolic Constant Macros (defines) --*/
#define COREBITS_TEST(fn, setup, teardown) TEST_FUNCTION(corebits__##fn, setup, teardown)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void setup_even_odd_all(corebits_t* evenbits, corebits_t* oddbits, corebits_t* allbits)
{
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        if ((i % 2) == 0)
        {
            corebits_set_bit(evenbits, i);
        }
        else
        {
            corebits_set_bit(oddbits, i);
        }
        corebits_set_bit(allbits, i);
    }
}

/* test for corebits functionality */
COREBITS_TEST(set_bit, NULL, NULL)
{
    corebits_t testbits = {0};

    // verify only one bit is set by set_bit
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        corebits_clear(&testbits);
        corebits_set_bit(&testbits, i);
        for (int j = 0; j < COREBITS_MAX_BITS; ++j)
        {
            if (i == j)
            {
                assert_true(corebits_is_bit_set(&testbits, j));
            }
            else
            {
                assert_false(corebits_is_bit_set(&testbits, j));
            }
        }
    }
}

COREBITS_TEST(clear_bit, NULL, NULL)
{
    corebits_t testbits = {0};
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    // verify only one bit is clear by clear_bit
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        testbits = allbits;
        corebits_clear_bit(&testbits, i);
        for (int j = 0; j < COREBITS_MAX_BITS; ++j)
        {
            if (i == j)
            {
                assert_false(corebits_is_bit_set(&testbits, j));
            }
            else
            {
                assert_true(corebits_is_bit_set(&testbits, j));
            }
        }
    }
}

COREBITS_TEST(clear_bit_invalid, NULL, NULL)
{
    corebits_t testbits = {0};
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    testbits = allbits;
    corebits_clear_bit(&testbits, COREBITS_MAX_BITS);

    assert_true(corebits_is_equal(&testbits, &allbits));
}

COREBITS_TEST(set_bit_invalid, NULL, NULL)
{
    corebits_t testbits = {0};
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    testbits = allbits;
    corebits_set_bit(&testbits, COREBITS_MAX_BITS);

    assert_true(corebits_is_equal(&testbits, &allbits));
}

COREBITS_TEST(first_set, NULL, NULL)
{
    corebits_t testbits = {0};

    assert_int_equal(corebits_first(&testbits), -1);
    assert_true(corebits_is_clear(&testbits));

    // verify we can set bits starting at 0 and get accurate first set bit
    // result
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        corebits_set_bit(&testbits, i);
        assert_false(corebits_is_clear(&testbits));
        assert_int_equal(corebits_first(&testbits), i);
        assert_true(corebits_is_bit_set(&testbits, i));
    }
}

COREBITS_TEST(first_clear, NULL, NULL)
{
    // set all bits
    corebits_t testbits = {{0xffffffff, 0xffffffff, 0xf}};
    // verify we can clear bits starting at MAX-1 (currently 127) and still get
    // accurate first set bit result
    for (int i = (COREBITS_MAX_BITS - 1); i >= 0; --i)
    {
        assert_int_equal(corebits_first(&testbits), i);
        corebits_clear_bit(&testbits, i);
    }
    assert_int_equal(corebits_first(&testbits), -1);
    assert_true(corebits_is_clear(&testbits));

    // verify that is bit set >= MAX_BITS returns 0
    corebits_set_bit(&testbits, COREBITS_MAX_BITS);
    assert_false(corebits_is_bit_set(&testbits, COREBITS_MAX_BITS));
}

COREBITS_TEST(and, NULL, NULL)
{
    corebits_t testbits = {0};
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    // AND
    testbits = oddbits;
    corebits_and(&testbits, &evenbits);
    assert_true(corebits_is_clear(&testbits));
    testbits = evenbits;
    corebits_and(&testbits, &oddbits);
    assert_true(corebits_is_clear(&testbits));
    testbits = oddbits;
    corebits_and(&testbits, &oddbits);
    assert_true(corebits_is_equal(&testbits, &oddbits));
    assert_false(corebits_is_equal(&testbits, &evenbits));
}

COREBITS_TEST(and_single_bit, NULL, NULL)
{
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    // verify any single bit anded with all bits is only that bit
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        corebits_t testop1 = allbits;
        corebits_t testop2 = {0};
        corebits_set_bit(&testop2, i);
        corebits_and(&testop1, &testop2);
        assert_true(corebits_is_equal(&testop1, &testop2));
    }
}

COREBITS_TEST(or, NULL, NULL)
{
    corebits_t testbits = {0};
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    testbits = oddbits;
    corebits_or(&testbits, &evenbits);
    assert_true(corebits_is_equal(&testbits, &allbits));
}

COREBITS_TEST(or_single_bit, NULL, NULL)
{
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);
    // verify any single bit ored with no bits is only that bit
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        corebits_t testop1 = {0};
        corebits_t testop2 = {0};
        corebits_set_bit(&testop2, i);
        corebits_or(&testop1, &testop2);
        assert_true(corebits_is_equal(&testop1, &testop2));
    }
}

COREBITS_TEST(count_bits, NULL, NULL)
{
    corebits_t oddbits = {0};
    corebits_t evenbits = {0};
    corebits_t allbits = {0};

    setup_even_odd_all(&evenbits, &oddbits, &allbits);

    // verify bit counting
    assert_int_equal(corebits_count(&evenbits), COREBITS_MAX_BITS / 2);
    assert_int_equal(corebits_count(&oddbits), COREBITS_MAX_BITS / 2);
    assert_int_equal(corebits_count(&allbits), COREBITS_MAX_BITS);
}

COREBITS_TEST(count_bits_invalid, NULL, NULL)
{
    // purposefully set bits that aren't in range of max bits
    corebits_t allbits = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}};

    assert_int_equal(corebits_count(&allbits), COREBITS_MAX_BITS);
}

COREBITS_TEST(first_set_invalid, NULL, NULL)
{
    // purposefully set bits that aren't in range of max bits
    corebits_t allbits = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}};

    assert_int_equal(corebits_first(&allbits), COREBITS_MAX_BITS - 1);
}

/* test for corebits invert functionality */
COREBITS_TEST(invert, NULL, NULL)
{

    corebits_t testbits = {{0xA5A5A5A5, 0xFFFFFFFF, 0x3C}};
    corebits_t testbits_out = {{0x5A5A5A5A, 0x00000000, 0xC3}};

    corebits_inv(&testbits);
    assert_true(corebits_is_equal(&testbits, &testbits_out));
}
}