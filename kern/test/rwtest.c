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
#include <kern/test161.h>
#include <spinlock.h>

#define CREATELOOPS		8
#define NREADERTHREADS      32
#define NWRITERTHREADS      5

static struct semaphore *donesem = NULL;
static struct rwlock *testrwlock = NULL;
static bool test_status = FAIL;

int n_reader_threads_accessing;
int n_writer_threads_accessing;

static
void
rwlock_reader_thread(void *junk, unsigned long num)
{
	(void)junk;
	
	kprintf_t(".");
	rwlock_acquire_read(testrwlock);

	n_reader_threads_accessing++;

	kprintf("Reader thread %lu starting\n",num);

	// check if no writers have access and no more than MAX_READER threads are acessing critical section :)
	if(n_writer_threads_accessing > 0 || n_reader_threads_accessing > MAX_READERS) {
		test_status=FAIL;
	}

	n_reader_threads_accessing--;
	kprintf("Reader thread %lu ending\n",num);

	rwlock_release_read(testrwlock);

	V(donesem);
	return;
}

static
void
rwlock_writer_thread(void *junk, unsigned long num)
{
	(void)junk;
	
	kprintf_t(".");
	rwlock_acquire_write(testrwlock);

	n_writer_threads_accessing++;

	kprintf("Writer thread %lu starting\n",num);

	// check if I have exclusice access :)
	if(n_reader_threads_accessing>0 || n_writer_threads_accessing > 1) {
		test_status=FAIL;
	}

	n_writer_threads_accessing--;
	kprintf("Writer thread %lu ending\n",num);

	rwlock_release_write(testrwlock);
	

	V(donesem);
	return;
}

/*
 * Use these stubs to test your reader-writer locks.
 */
 
int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;

	int i,result;
	n_reader_threads_accessing=0;
	n_writer_threads_accessing=0;

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

	// creating reader threads
	for (i=0; i<NREADERTHREADS; i++) {
		kprintf_t(".");
		result = thread_fork("synchtest", NULL, rwlock_reader_thread, NULL, i);
		if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
	}

	// creating writer threads
	for (i=0; i<NWRITERTHREADS; i++) {
		kprintf_t(".");
		result = thread_fork("synchtest", NULL, rwlock_writer_thread, NULL, i);
		if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
	}

	for (i=0; i<NREADERTHREADS+NWRITERTHREADS; i++) {
		kprintf_t(".");
		P(donesem);
	}
	success(test_status, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
