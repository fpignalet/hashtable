/// @file main.c
/// @copyright BSD 2-clause. See LICENSE.txt for the complete license text.
/// @author Dane Larsen
/// @brief The main function is used for testing the hashtable library,
///        and also provides example code.

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../inc/hashcore.h"
#include "../inc/test.h"
#include "../inc/timer.h"

static void main_test1(hash_table_t *pht);
static void main_test2(hash_table_t *pht);
static void main_test3(hash_table_t *pht);
static void main_test4(hash_table_t *pht);

static const char *main_testkey_1 = (const char*)"testKEY 1";
static const char *main_testdata_1 = (const char*)"testDATA 1";
static const char *main_testdata_2 = (const char*)"testDATA 2";

/*!***********************************************************
 * VISIBLE IMPLEMENTATION
 ************************************************************/

/*! \brief desc.
 *  \param x something
 *  \return y something else
 */
int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    //------------------------------------------------------------------------------------
    hash_table_t ht;
    ht_init(&ht, HT_KEY_CONST | HT_VALUE_CONST, 0.05);

    //------------------------------------------------------------------------------------
    main_test1(&ht);
    main_test2(&ht);
    main_test3(&ht);
    main_test4(&ht);

    //------------------------------------------------------------------------------------
    ht_destroy(&ht);

    return test_report_results();
}

/*!***********************************************************
 * NON-VISIBLE IMPLEMENTATION
 ************************************************************/

/*! \brief desc.
 *  \param x something
 *  \return y something else
 */
static void main_test1(hash_table_t *pht)
{
    fprintf(stderr,
            "-----\nInitialising with {\"%s\": \"%s\"}\n",
            main_testkey_1, main_testdata_1);

    //------------------------------------------------------------------------------------
    //action 1
    ht_insert(pht,
              (void*)main_testkey_1, strlen(main_testkey_1)+1,
              (void*)main_testdata_1, strlen(main_testdata_1)+1);

    int contains = ht_contains_i(pht, (void *) main_testkey_1, strlen(main_testkey_1) + 1);
    test(contains,
         "Checking for pkey \"%s\"",
         main_testkey_1);

    //------------------------------------------------------------------------------------
    //verif 1
    size_t value_size;
    char *pgot = ht_get_p(pht,
                          (void *) main_testkey_1, strlen(main_testkey_1) + 1,
                          &value_size);

    fprintf(stderr, "Value size: %zu\n", value_size);
    fprintf(stderr, "Got: {\"%s\": \"%s\"}\n", main_testkey_1, pgot);
    test(value_size == strlen(main_testdata_1)+1,
         "Value size was %zu (desired %lu)",
         value_size, strlen(main_testdata_1)+1);

}

/*! \brief desc.
 *  \param x something
 *  \return y something else
 */
void main_test2(hash_table_t *pht)
{
    fprintf(stderr,
            "-----\nReplacing {\"%s\": \"%s\"} with {\"%s\": \"%s\"}\n",
            main_testkey_1, main_testdata_1, main_testkey_1, main_testdata_2);

    //------------------------------------------------------------------------------------
    //action 2
    ht_insert(pht,
              (void*)main_testkey_1, strlen(main_testkey_1)+1,
              (void*)main_testdata_2, strlen(main_testdata_2)+1);
    unsigned int num_keys;
    void **ppkeys = ht_keys_pp(pht, &num_keys);

    test(num_keys == 1, "HashTable has %d ppkeys", num_keys);
    test(ppkeys != NULL, "Keys is not null");

    if(NULL != ppkeys)
        free(ppkeys);

    //------------------------------------------------------------------------------------
    //verif 2
    size_t value_size;
    char *pgot = ht_get_p(pht,
                          (void *) main_testkey_1, strlen(main_testkey_1) + 1,
                          &value_size);

    fprintf(stderr, "Value size: %zu\n", value_size);
    fprintf(stderr, "Got: {\"%s\": \"%s\"}\n", main_testkey_1, pgot);
    test(value_size == strlen(main_testdata_2)+1,
         "Value size was %zu (desired %lu)",
         value_size, strlen(main_testdata_2)+1);

}

/*! \brief desc.
 *  \param x something
 *  \return y something else
 */
void main_test3(hash_table_t *pht)
{
    fprintf(stderr,
            "-----\nRemoving entry with pkey \"%s\"\n",
            main_testkey_1);

    //------------------------------------------------------------------------------------
    //action 3
    ht_remove(pht,
              (void*)main_testkey_1, strlen(main_testkey_1)+1);
    int contains =
    ht_contains_i(pht,
                  (void*) main_testkey_1, strlen(main_testkey_1) + 1);

    test(!contains,
         "Checking for removal of pkey \"%s\"",
         main_testkey_1);

    //------------------------------------------------------------------------------------
    //verif 3
    unsigned int num_keys;
    void **ppkeys = ht_keys_pp(pht, &num_keys);

    test(num_keys == 0,
         "HashTable has %d ppkeys", num_keys);

    if(ppkeys)
        free(ppkeys);

}

/*! \brief desc.
 *  \param x something
 *  \return y something else
 */
void main_test4(hash_table_t *pht)
{
    fprintf(stderr, "-----\nStress test");

    int index;

    //------------------------------------------------------------------------------------
    //action 4.1
    int key_count = 1000000; //1 million
    int *pmany_keys = malloc(key_count * sizeof(*pmany_keys));
    int *pmany_values = malloc(key_count * sizeof(*pmany_values));

    for(index = 0; index < key_count; index++)
    {
        pmany_keys[index] = index;
        pmany_values[index] = rand();
    }

    struct timespec t1;
    struct timespec t2;

    srand(time(NULL));

    t1 = snap_time();
    for(index = 0; index < key_count; index++)
    {
        ht_insert(pht,
                  &(pmany_keys[index]), sizeof(pmany_keys[index]),
                  &(pmany_values[index]), sizeof(pmany_values[index]));
    }

    t2 = snap_time();
    fprintf(stderr,
            "\n1-Inserting %d ppkeys took %.2f seconds\n",
            key_count, get_elapsed(t1, t2));

    //------------------------------------------------------------------------------------
    //verif 4.1
    fprintf(stderr, "Checking table contents\n");

    int ok_flag = 1;
    //goal is to have ok_flag still equal to 1 in the end
    for(index = 0; index < key_count; index++)
    {
        if(0 != ht_contains_i(pht, &(pmany_keys[index]), sizeof(pmany_keys[index])))
        {
            size_t value_size;
            int value = *(int*) ht_get_p(pht, &(pmany_keys[index]), sizeof(pmany_keys[index]), &value_size);
            if(value != pmany_values[index])
            {
                fprintf(stderr,
                        "Key pvalue mismatch. Got {%d: %d} expected: {%d: %d}\n",
                        pmany_keys[index], value, pmany_keys[index], pmany_values[index]);

                ok_flag = 0;
                break;
            }
        }
        else
        {
            fprintf(stderr,
                    "Missing pkey-pvalue pair {%d: %d}\n",
                    pmany_keys[index], pmany_values[index]);

            ok_flag = 0;
            break;
        }
    }

    test(ok_flag == 1,
         "Result was %d", ok_flag);

    //------------------------------------------------------------------------------------
    // action 4.2
    ht_clear(pht);
    ht_resize(pht, 4194304);

    t1 = snap_time();
    for(index = 0; index < key_count; index++)
    {
        ht_insert(pht,
                  &(pmany_keys[index]),
                  sizeof(pmany_keys[index]), &(pmany_values[index]),
                  sizeof(pmany_values[index]));
    }

    t2 = snap_time();
    fprintf(stderr,
            "2-Inserting %d ppkeys (on preallocated table) took %.2f seconds\n",
            key_count, get_elapsed(t1, t2));

    for(index = 0; index < key_count; index++)
    {
        ht_remove(pht,
                  &(pmany_keys[index]),
                  sizeof(pmany_keys[index]));
    }

    //------------------------------------------------------------------------------------
    //verif 4.2
    test(ht_size_ui(pht) == 0, "%d ppkeys remaining", ht_size_ui(pht));

    //------------------------------------------------------------------------------------
    free(pmany_keys);
    free(pmany_values);
//    for(int i = 0; i < 50; i++){
//        sleep(1);
//    }

}
