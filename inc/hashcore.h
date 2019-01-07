/// @file hashtable.h
/// @copyright BSD 2-clause. See LICENSE.txt for the complete license text
/// @author Dane Larsen
/// @brief The primary header for the hashtable library. All public functions are defined here.

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <stddef.h>

#include "hashfunc.h"

/// The initial size of the hash table.
#ifndef HT_INITIAL_SIZE
#define HT_INITIAL_SIZE 64
#endif //HT_INITIAL_SIZE

/// The hash entry struct. Acts as a node in a linked list.
struct hash_entry {
    /// A pointer to the key.
    void *pkey;
    /// The size of the value in bytes.
    size_t value_size;

    /// A pointer to the value.
    void *pvalue;
    /// The size of the key in bytes.
    size_t key_size;

    /// A pointer to the next hash entry in the chain (or NULL if none).
    /// This is used for collision resolution.
    struct hash_entry *pnext;
};

/// The hash_entry struct. This is considered to be private
typedef struct hash_entry hash_entry_t;

/// The primary hashtable struct
typedef struct hash_table {
    // hash function for x86_32
    HashFunc *phashfunc_x86_32;
    // hash function for x86_128
    HashFunc *phashfunc_x86_128;
    // hash function for x64_128
    HashFunc *phashfunc_x64_128;

    /// The number of keys in the hash table.
    unsigned int key_count;
    /// The internal hash table array.
    hash_entry_t **pparray;

    /// The size of the internal array.
    unsigned int array_size;

    /// A count of the number of hash collisions.
    unsigned int collisions;

    /// Any flags that have been set. (See the ht_flags enum).
    int flags;

    /// The max load factor that is acceptable before an autoresize is triggered
    /// (where load_factor is the ratio of collisions to table size).
    double max_load_factor;
    /// The current load factor.
    double current_load_factor;

} hash_table_t;

/// Hashtable initialization flags (passed to ht_init)
typedef enum {

    /// No options set
    HT_NONE = 0,

    /// Constant length key, useful if your keys are going to be a fixed size.
    HT_KEY_CONST = 1,

    /// Constant length value.
    HT_VALUE_CONST = 2,

    /// Don't automatically resize hashtable when the load factor
    /// goes above the trigger value
    HT_NO_AUTORESIZE = 4

} hash_flags_t;

//----------------------------------
// HashEntry functions
//----------------------------------

/// @brief Creates a new hash entry.
/// @param flags Hash table flags.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
/// @param pvalue A pointer to the value.
/// @param value_size The size of the value in bytes.
/// @returns A pointer to the hash entry.
hash_entry_t *he_create_p(int flags, void *pkey, size_t key_size, void *pvalue, size_t value_size);

/// @brief Destroys the hash entry and frees all associated memory.
/// @param flags The hash table flags.
/// @param hash_entry A pointer to the hash entry.
void he_destroy(int flags, hash_entry_t *pentry);

/// @brief Compare two hash entries.
/// @param pe1 A pointer to the first entry.
/// @param pe2 A pointer to the second entry.
/// @returns 1 if both the keys and the values of e1 and e2 match, 0 otherwise.
///          This is a "deep" compare, rather than just comparing pointers.
int he_key_compare_i(hash_entry_t *pe1, hash_entry_t *pe2);

/// @brief Sets the value on an existing hash entry.
/// @param flags The hashtable flags.
/// @param pentry A pointer to the hash entry.
/// @param pvalue A pointer to the new value.
/// @param value_size The size of the new value in bytes.
void he_set_value(int flags, hash_entry_t *pentry, void *pvalue, size_t value_size);

//----------------------------------
// HashTable functions
//----------------------------------

/// @brief Initializes the hash_table struct.
/// @param ptable A pointer to the hash table.
/// @param flags Options for the way the table behaves.
/// @param max_load_factor The ratio of collisions:table_size before an autoresize is triggered
///        for example: if max_load_factor = 0.1, the table will resize if the number
///        of collisions increases beyond 1/10th of the size of the table
void ht_init(hash_table_t *ptable, hash_flags_t flags, double max_load_factor
#ifndef __WITH_MURMUR
        , HashFunc *for_x86_32, HashFunc *for_x86_128, HashFunc *for_x64_128
#endif //__WITH_MURMUR
);

/// @brief Removes all entries from the hash table.
/// @param ptable A pointer to the hash table.
void ht_clear(hash_table_t *ptable);

/// @brief Destroys the hash_table struct and frees all relevant memory.
/// @param ptable A pointer to the hash table.
void ht_destroy(hash_table_t *ptable);

/// @brief Resizes the hash table's internal array. This operation is
///        _expensive_, however it can make an overfull table run faster
///        if the table is expanded. The table can also be shrunk to reduce
///        memory usage.
/// @param ptable A pointer to the table.
/// @param new_size The desired size of the table.
void ht_resize(hash_table_t *ptable, unsigned int new_size);

/// @brief Inserts an existing hash entry into the hash table.
/// @param ptable A pointer to the hash table.
/// @param pentry A pointer to the hash entry.
void ht_he_insert(hash_table_t *ptable, hash_entry_t *pentry);

/// @brief Returns a pointer to the value with the matching key,
///         value_size is set to the size in bytes of the value
/// @param ptable A pointer to the hash table.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
/// @param pvalue_size A pointer to a size_t where the size of the return
///         value will be stored.
/// @returns A pointer to the requested value. If the return value
///           is NULL, the requested key-value pair was not in the table.
void* ht_get_p(hash_table_t *ptable, void *pkey, size_t key_size, size_t *pvalue_size);

/// @brief Used to see if the hash table contains a key-value pair.
/// @param ptable A pointer to the hash table.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
/// @returns 1 if the key is in the table, 0 otherwise
int ht_contains_i(hash_table_t *ptable, void *pkey, size_t key_size);

/// @brief Inserts the {key: value} pair into the hash table, makes copies of both key and value.
/// @param ptable A pointer to the hash table.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
/// @param pvalue A pointer to the value.
/// @param value_size The size of the value in bytes.
void ht_insert(hash_table_t *ptable, void *pkey, size_t key_size, void *pvalue, size_t value_size);

/// @brief Removes the entry corresponding to the specified key from the hash table.
/// @param ptable A pointer to the hash table.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
void ht_remove(hash_table_t *ptable, void *pkey, size_t key_size);

/// @brief Sets the global security seed to be used in hash function.
/// @param seed The seed to use.
void ht_set_seed(uint32_t seed);

/// @brief Returns the number of entries in the hash table.
/// @param ptable A pointer to the table.
/// @returns The number of entries in the hash table.
unsigned int ht_size_ui(hash_table_t *ptable);

/// @brief Returns an array of all the keys in the hash table.
/// @param ptable A pointer to the hash table.
/// @param pkey_count A pointer to an unsigned int that
///        will be set to the number of keys in the returned array.
/// @returns A pointer to an array of keys.
/// TODO: Add a key_lengths return value as well?
void** ht_keys_pp(hash_table_t *ptable, unsigned int *pkey_count);

/// @brief Calulates the index in the hash table's internal array
///        from the given key (used for debugging currently).
/// @param ptable A pointer to the hash table.
/// @param pkey A pointer to the key.
/// @param key_size The size of the key in bytes.
/// @returns The index into the hash table's internal array.
unsigned int ht_index_ui(hash_table_t *ptable, void *pkey, size_t key_size);

#endif

