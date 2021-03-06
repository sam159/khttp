/* 
 * File:   minunit.h
 * Author: Zed. A. Shaw, Sam
 * 
 * @see http://c.learncodethehardway.org/book/ex30.html
 *
 * Created on 27 August 2014, 22:14
 */

#ifndef MINUNIT_H
#define	MINUNIT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
    
#define log_err(message) printf("\tError: %s\n", message)

#define mu_suite_start() char *message = NULL

#define mu_assert(test, message)    \
    do {                            \
        if (!(test)) {              \
            log_err(message);       \
            return message;         \
        }                           \
    } while(0)
    
#define mu_run_test(test)                   \
    do {                                    \
        printf("\n-----%s", " " #test);     \
        message = test();                   \
        tests_run++;                        \
        if (message) { return message; }    \
    while(0)

#define RUN_TESTS(name)                                 \
    int main(int argc, char *argv[]) {                  \
        tests_run = 0;                                  \
        argc = 1;                                       \
        printf("----\nRUNNING: %s\n", argv[0]);         \
        char *result = name();                          \
        if (result != 0) {                              \
            printf("FAILED: %s\n", result);             \
        }                                               \
        else {                                          \
            printf("ALL TESTS PASSED\n");               \
        }                                               \
        printf("Tests run: %d\n", tests_run);           \
        exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);\
    }


int tests_run;

#ifdef	__cplusplus
}
#endif

#endif	/* MINUNIT_H */

