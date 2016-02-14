/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/secret.h>
#include <spinlock.h>

 #define CREATELOOPS		8

static struct semaphore *donesem = NULL;
static struct rwlock *testrwlock = NULL;
static bool test_status = FAIL;

int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;

	int i;

	kprintf_n("Starting rwt1...\n");
	// Check if rwlock gets destroyed properly
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");
		testrwlock = rwlock_create("testrwlock");
		if (testrwlock == NULL) {
			panic("rwt1: rwlock_create failed\n");
		}
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("lt1: sem_create failed\n");
		}
		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testrwlock);
			sem_destroy(donesem);
		}
	}
	test_status = SUCCESS;

	success(test_status, SECRET, "rwt1");

	return 0;
}
