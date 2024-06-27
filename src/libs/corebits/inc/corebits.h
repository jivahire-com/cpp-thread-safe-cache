//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file corebits.h
 * Private header for simple, per-core (AP) bitfield manipulation
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define COREBITS_MAX_BITS 68

#define BITTYPE          uint32_t /* to use alternate type, would need update to _count method */
#define BITTYPE_BITS_PER (sizeof(BITTYPE) * 8)
#define BITTYPE_COUNT    ((COREBITS_MAX_BITS + (BITTYPE_BITS_PER - 1)) / BITTYPE_BITS_PER)

// helpers for printf
#define COREBITS_FMT_STR            "%x:%08x:%08x"
#define COREBITS_FMT_DATA(corebits) (unsigned int)corebits.bits[2], (unsigned int)corebits.bits[1], (unsigned int)corebits.bits[0]

// helpers for init
#define COREBITS_INIT_UINT32(core0_31, core32_63, core64_95) \
    {                                                        \
        {                                                    \
            core0_31, core32_63, core64_95                   \
        }                                                    \
    }

/*-------------- Typedefs ----------------*/

/**
 *  @brief Struct for 1bit/core
 */
typedef struct _corebits_t
{
    BITTYPE bits[BITTYPE_COUNT];
} corebits_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Clear all bits in a corebits structure
 *
 * \b Description:
 *      Clear all bits in a corebits structure
 *
 * @param[inout] corebits - Bit object to perform operation on
 *
 * @return None
 *
 */
void corebits_clear(corebits_t* corebits);

/**
 * @brief Clear a bit
 *
 * \b Description:
 *      Clears a specific bit in a corebits structure
 *
 * @param[inout] corebits - Bit object to perform operation on
 * @param[in] bit - Bit to clear
 *
 * @return None
 *
 */
void corebits_clear_bit(corebits_t* corebits, unsigned bit);

/**
 * @brief Set a bit
 *
 * \b Description:
 *      Set a specific bit in a corebits structure
 *
 * @param[inout] corebits - Bit object to perform operation on
 * @param[in] bit - Bit to set
 *
 * @return None
 *
 */
void corebits_set_bit(corebits_t* corebits, unsigned bit);

/**
 * @brief AND two sets of bits
 *
 * \b Description:
 *      AND two sets of bits passed in and return result in first parameter
 *
 * @param[inout] corebits - Bit object to perform operation on
 * @param[in] andbits - Bit object to AND with corebits
 *
 * @return None
 *
 */
void corebits_and(corebits_t* corebits, const corebits_t* andbits);

/**
 * @brief OR two sets of bits
 *
 * \b Description:
 *      OR two sets of bits passed in and return result in first parameter
 *
 * @param[inout] corebits - Bit object to perform operation on
 * @param[in] orbits - Bit object to OR with corebits
 *
 * @return None
 *
 */
void corebits_or(corebits_t* corebits, const corebits_t* orbits);

/**
 * @brief Inverts the provided corebits structure
 *
 * \b Description:
 *      Inverts the set of bits
 *
 * @param[inout] corebits - Bit object to perform operation on
 *
 * @return None
 *
 */
void corebits_inv(corebits_t* corebits);

/**
 * @brief Check bit set
 *
 * \b Description:
 *      Returns true if bit is set
 *
 * @param[in] corebits - Bit object to perform operation on
 * @param[in] bit - Bit to check
 *
 * @return true/false
 *
 */
bool corebits_is_bit_set(const corebits_t* corebits, unsigned bit);

/**
 * @brief Check if equal
 *
 * \b Description:
 *      Returns true if same bits set in both inputs
 *
 * @param[in] corebits1 - First input
 * @param[in] corebits2 - Second input
 *
 * @return true/false
 *
 */
bool corebits_is_equal(const corebits_t* corebits1, const corebits_t* corebits2);

/**
 * @brief Check all bits clear
 *
 * \b Description:
 *      Returns true if no bit is set
 *
 * @param[in] corebits - Bit object to perform operation on
 *
 * @return true/false
 *
 */
bool corebits_is_clear(const corebits_t* corebits);

/**
 * @brief Find bit index of most significant set bit
 *
 * \b Description:
 *      Returns first set bit or -1
 *
 * @param[in] corebits - Bit object to perform operation on
 *
 * @return (COREBITS_MAX_BITS-1) -- 0, -1
 *
 */
int corebits_first(const corebits_t* corebits);

/**
 * @brief Find count of set bits
 *
 * \b Description:
 *      Returns number of bits set in corebits structure
 *
 * @param[in] corebits - Bit object to perform operation on
 *
 * @return 0--COREBITS_MAX_BITS
 *
 */
int corebits_count(const corebits_t* corebits);