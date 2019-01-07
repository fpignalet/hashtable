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
//#include <tkDecls.h>

static uint32_t global_seed = 2976579765;

//----------------------------------
// Debug macro
//----------------------------------

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "%s:%d - " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

//-----------------------------------
// HashTable functions
//-----------------------------------

void ht_init(hash_table_t *ptable, hash_flags_t flags, double max_load_factor
#ifndef __WITH_MURMUR
        , HashFunc *for_x86_32, HashFunc *for_x86_128, HashFunc *for_x64_128
#endif //__WITH_MURMUR
)
{
    //----------------------------------------------------------------
#   ifdef __WITH_MURMUR
    ptable->phashfunc_x86_32  = MurmurHash3_x86_32;
    ptable->phashfunc_x86_128 = MurmurHash3_x86_128;
    ptable->phashfunc_x64_128 = MurmurHash3_x64_128;

#   else //not __WITH_MURMUR
    ptable->phashfunc_x86_32  = for_x86_32;
    ptable->phashfunc_x86_128 = for_x86_128;
    ptable->phashfunc_x64_128 = for_x64_128;

#   endif //__WITH_MURMUR
    //----------------------------------------------------------------
    ptable->array_size = HT_INITIAL_SIZE;
    ptable->pparray = malloc(ptable->array_size * sizeof(*(ptable->pparray)));
    if(NULL == ptable->pparray) {
        debug("ht_init failed to allocate memory\n");
        exit(-1);
    }

    ptable->key_count            = 0;
    ptable->collisions           = 0;
    ptable->flags                = flags;
    ptable->max_load_factor      = max_load_factor;
    ptable->current_load_factor  = 0.0;

    //----------------------------------------------------------------
    unsigned int index;
    for(index = 0; index < ptable->array_size; index++)
    {
        ptable->pparray[index] = NULL;
    }

    return;
}

void ht_clear(hash_table_t *ptable)
{
    ht_destroy(ptable);

    ht_init(ptable, ptable->flags, ptable->max_load_factor
#   ifndef __WITH_MURMUR
            , ptable->phashfunc_x86_32, ptable->phashfunc_x86_128, ptable->phashfunc_x64_128
#   endif //__WITH_MURMUR
            );
}

void ht_destroy(hash_table_t *ptable)
{
    unsigned int i;

    hash_entry_t *pentry;
    hash_entry_t *ptmp;

    if(NULL == ptable->pparray) {
        debug("ht_destroy got a bad ptable\n");
    }

    // crawl the entries and delete them
    for(i = 0; i < ptable->array_size; i++) {
        pentry = ptable->pparray[i];

        while(NULL != pentry) {
            ptmp = pentry->pnext;
            he_destroy(ptable->flags, pentry);
            pentry = ptmp;
        }
    }

    ptable->phashfunc_x86_32 = NULL;
    ptable->phashfunc_x86_128 = NULL;
    ptable->phashfunc_x64_128 = NULL;

    ptable->array_size = 0;
    ptable->key_count = 0;
    ptable->collisions = 0;

    free(ptable->pparray);
    ptable->pparray = NULL;
}

// new_size can be smaller than current size (downsizing allowed)
void ht_resize(hash_table_t *ptable, unsigned int new_size)
{
    hash_table_t new_table;

    debug("ht_resize(old=%d, new=%d)\n",ptable->array_size,new_size);
    new_table.phashfunc_x86_32 = ptable->phashfunc_x86_32;
    new_table.phashfunc_x86_128 = ptable->phashfunc_x86_128;
    new_table.phashfunc_x64_128 = ptable->phashfunc_x64_128;
    new_table.array_size = new_size;
    new_table.pparray = malloc(new_size * sizeof(hash_entry_t*));
    new_table.key_count = 0;
    new_table.collisions = 0;
    new_table.flags = ptable->flags;
    new_table.max_load_factor = ptable->max_load_factor;

    unsigned int i;
    for(i = 0; i < new_table.array_size; i++)
    {
        new_table.pparray[i] = NULL;
    }

    hash_entry_t *entry;
    hash_entry_t *next;
    for(i = 0; i < ptable->array_size; i++)
    {
        entry = ptable->pparray[i];
        while(NULL != entry)
        {
            next = entry->pnext;
            ht_he_insert(&new_table, entry);
            entry = next;
        }
        ptable->pparray[i]=NULL;
    }

    ht_destroy(ptable);

    ptable->phashfunc_x86_32 = new_table.phashfunc_x86_32;
    ptable->phashfunc_x86_128 = new_table.phashfunc_x86_128;
    ptable->phashfunc_x64_128 = new_table.phashfunc_x64_128;
    ptable->array_size = new_table.array_size;
    ptable->pparray = new_table.pparray;
    ptable->key_count = new_table.key_count;
    ptable->collisions = new_table.collisions;

}

/************************************************************************************************>
 * ACCESS
 ************************************************************************************************/
// this was separated out of the regular ht_insert for ease of copying hash entries around
void ht_he_insert(hash_table_t *ptable, hash_entry_t *pentry){
    unsigned int index;

    hash_entry_t *ptmp;

    pentry->pnext = NULL;
    index = ht_index_ui(ptable, pentry->pkey, pentry->key_size);
    ptmp = ptable->pparray[index];
    //! if true, no collision
    if(NULL == ptmp)
    {
        ptable->pparray[index] = pentry;
        ptable->key_count++;
        return;
    }

    /*! walk down the chain until we either hit the end
        or find an identical pkey (in which case we replace the pvalue)
    */
    while(NULL != ptmp->pnext)
    {
        if(he_key_compare_i(ptmp, pentry))
            break;
        else
            ptmp = ptmp->pnext;
    }

    if(he_key_compare_i(ptmp, pentry))
    {
        /*! if the keys are identical, throw away the old pentry
            and stick the new one into the ptable */
        he_set_value(ptable->flags, ptmp, pentry->pvalue, pentry->value_size);
        he_destroy(ptable->flags, pentry);
    }
    else
    {
        //! else tack the new pentry onto the end of the chain
        ptmp->pnext = pentry;
        ptable->collisions += 1;
        ptable->key_count ++;
        ptable->current_load_factor = (double)ptable->collisions / ptable->array_size;

        /*! double the size of the ptable if autoresize is on and the
            load factor has gone too high */
        if(!(ptable->flags & HT_NO_AUTORESIZE) &&
                (ptable->current_load_factor > ptable->max_load_factor)) {
            ht_resize(ptable, ptable->array_size * 2);
            ptable->current_load_factor =
                (double)ptable->collisions / ptable->array_size;
        }
    }
}

void* ht_get_p(hash_table_t *ptable, void *pkey, size_t key_size, size_t *pvalue_size)
{
    unsigned int index  = ht_index_ui(ptable, pkey, key_size);

    hash_entry_t *pentry   = ptable->pparray[index];

    hash_entry_t tmp;
    tmp.pkey = pkey;
    tmp.key_size = key_size;

    // once we have the right index, walk down the chain (if any)
    // until we find the right pkey or hit the end
    while(NULL != pentry)
    {
        if(he_key_compare_i(pentry, &tmp))
        {
            if(NULL != pvalue_size)
                *pvalue_size = pentry->value_size;

            return pentry->pvalue;
        }
        else
        {
            pentry = pentry->pnext;
        }
    }

    return NULL;
}

int ht_contains_i(hash_table_t *ptable, void *pkey, size_t key_size)
{
    unsigned int index  = ht_index_ui(ptable, pkey, key_size);

    hash_entry_t *pentry   = ptable->pparray[index];

    hash_entry_t tmp;
    tmp.pkey = pkey;
    tmp.key_size = key_size;

    /// walk down the chain, compare keys
    while(NULL != pentry)
    {
        if(he_key_compare_i(pentry, &tmp))
            return 1;
        else
            pentry = pentry->pnext;
    }

    return 0;
}

/************************************************************************************************>
 * INSERT / REMOVE
 ************************************************************************************************/
void ht_insert(hash_table_t *ptable, void *pkey, size_t key_size, void *pvalue, size_t value_size)
{
    hash_entry_t *pentry = he_create_p(ptable->flags, pkey, key_size, pvalue, value_size);
    ht_he_insert(ptable, pentry);
}

void ht_remove(hash_table_t *ptable, void *pkey, size_t key_size)
{
    unsigned int index  = ht_index_ui(ptable, pkey, key_size);

    hash_entry_t *pentry = ptable->pparray[index];

    hash_entry_t tmp;
    tmp.pkey = pkey;
    tmp.key_size = key_size;

    /// walk down the chain
    hash_entry_t *pprev = NULL;
    while(NULL != pentry)
    {
        /// if the pkey matches, take it out and connect its
        /// parent and child in its place
        if(he_key_compare_i(pentry, &tmp))
        {
            if(NULL == pprev)
                ptable->pparray[index] = pentry->pnext;
            else
                pprev->pnext = pentry->pnext;

            ptable->key_count--;

            if(NULL != pprev)
              ptable->collisions--;

            he_destroy(ptable->flags, pentry);
            return;
        }
        else
        {
            pprev = pentry;
            pentry = pentry->pnext;
        }
    }
}

/************************************************************************************************>
 * UTILS
 ************************************************************************************************/
void ht_set_seed(uint32_t seed){
    global_seed = seed;
}

unsigned int ht_size_ui(hash_table_t *ptable)
{
    return ptable->key_count;
}

void** ht_keys_pp(hash_table_t *ptable, unsigned int *pkey_count)
{
    void **ppret;

    /// table validity check
    if(0 == ptable->key_count){
      *pkey_count = 0;
      return NULL;
    }

    /// pparray of pointers to keys
    ppret = malloc(ptable->key_count * sizeof(void *));
    if(NULL == ppret) {
        debug("ht_keys_pp failed to allocate memory\n");
    }

    /// loop over all of the chains,
    /// walk the chains,
    /// add each entry to the pparray of keys
    *pkey_count = 0;

    unsigned int index;
    hash_entry_t *ptmp;
    for(index = 0; index < ptable->array_size; index++)
    {
        ptmp = ptable->pparray[index];

        while(NULL != ptmp)
        {
            ppret[*pkey_count]=ptmp->pkey;
            *pkey_count += 1;
            ptmp = ptmp->pnext;
            // sanity check, should never actually happen
            if(*pkey_count >= ptable->key_count) {
                debug("ht_keys_pp: too many keys, expected %d, got %d\n",
                        ptable->key_count, *pkey_count);
            }
        }
    }

    return ppret;
}

unsigned int ht_index_ui(hash_table_t *ptable, void *pkey, size_t key_size)
{
    uint32_t index;
    /// 32 bits of murmur seems to fare pretty well
    ptable->phashfunc_x86_32(pkey, key_size, global_seed, &index);
    index %= ptable->array_size;
    return index;
}
