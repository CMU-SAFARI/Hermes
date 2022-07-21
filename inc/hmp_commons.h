/*****************************************************************************************
 * This file defines some common constructs used in HMP definition:
 * Yoaz, A., Erez, M., Ronen, R., & Jourdan, S., 
 * "Speculation techniques for improving load related instruction scheduling", ISCA 1999
 * https://doi.org/10.1145/307338.300983
 *****************************************************************************************
 */

#ifndef HMP_COMMONS_H
#define HMP_COMMONS_H

typedef enum
{
    /* We use the same naming convension 
     * from branch prediction research.
     * In cache hit-miss prediction's context
     * NT(not-taken) should be read as "not-offchip"
     * T(taken) should be read as "offchip"
     */
    STRONG_NT = 0,
    WEAK_NT,
    WEAK_T,
    STRONG_T,
} confidence_state_t;

#endif /* HMP_COMMONS_H */

