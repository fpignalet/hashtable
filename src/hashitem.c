/// @cond PRIVATE
/// @file hashtable.c
/// @copyright BSD 2-clause. See LICENSE.txt for the complete license text
/// @author Dane Larsen

#include "../inc/hashcore.h"
#include "../inc/hashfunc.h"

#ifdef __WITH_MURMUR
#include "../inc/murmur.h"
#endif //__WITH_MURMUR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//----------------------------------
// Debug macro
//----------------------------------

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "%s:%d - " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

//----------------------------------
// HashEntry functions
//----------------------------------

hash_entry_t *he_create_p(int flags, void *pkey, size_t key_size, void *pvalue, size_t value_size)
{
    //-----------------------------------------------------------------------------
    hash_entry_t *pentry = malloc(sizeof(hash_entry_t));
    if(NULL == pentry) {
        debug("Failed to create hash_entry_t\n");
        return NULL;
    }

    //-----------------------------------------------------------------------------
    pentry->key_size = key_size;
    if (flags & HT_KEY_CONST){
        pentry->pkey = pkey;
    }
    else {
        pentry->pkey = malloc(key_size);
        if(NULL == pentry->pkey) {
            debug("Failed to create hash_entry_t\n");
            free(pentry);
            return NULL;
        }

        memcpy(pentry->pkey, pkey, key_size);
    }

    //-----------------------------------------------------------------------------
    pentry->value_size = value_size;
    if (flags & HT_VALUE_CONST){
        pentry->pvalue = pvalue;
    }
    else {
        pentry->pvalue = malloc(value_size);
        if(NULL == pentry->pvalue) {
            debug("Failed to create hash_entry_t\n");
            free(pentry->pkey);
            free(pentry);
            return NULL;
        }

        memcpy(pentry->pvalue, pvalue, value_size);
    }

    //-----------------------------------------------------------------------------
    pentry->pnext = NULL;

    return pentry;
}

void he_destroy(int flags, hash_entry_t *pentry)
{
    //-----------------------------------------------------------------------------
    if (!(flags & HT_KEY_CONST))
        free(pentry->pkey);

    if (!(flags & HT_VALUE_CONST))
        free(pentry->pvalue);

    //-----------------------------------------------------------------------------
    free(pentry);
}

int he_key_compare_i(hash_entry_t *pe1, hash_entry_t *pe2)
{
    char *k1 = pe1->pkey;
    char *k2 = pe2->pkey;

    if(pe1->key_size != pe2->key_size)
        return 0;

    return (memcmp(k1,k2,pe1->key_size) == 0);
}

void he_set_value(int flags, hash_entry_t *pentry, void *pvalue, size_t value_size)
{
    if (!(flags & HT_VALUE_CONST)) {
        if(pentry->pvalue)
            free(pentry->pvalue);

        pentry->pvalue = malloc(value_size);
        if(NULL == pentry->pvalue) {
            debug("Failed to set pentry pvalue\n");
            return;
        }

        memcpy(pentry->pvalue, pvalue, value_size);

    } else {
        pentry->pvalue = pvalue;

    }

    pentry->value_size = value_size;

    return;
}
