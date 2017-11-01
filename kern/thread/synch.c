/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
        struct semaphore *sem;

        sem = kmalloc(sizeof(*sem));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
        struct lock *lock;

        lock = kmalloc(sizeof(*lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }
/***************CODE ADDED FROM HERE********************/
        lock->lk_wchan = wchan_create(lock->lk_name);
      	if (lock->lk_wchan == NULL) {
      		kfree(lock->lk_name);
      		kfree(lock);
      		return NULL;
      	}

      	spinlock_init(&lock->lk_spinlock);
        lock->lk_flag = false;
        lock->lk_thread = NULL;
/***************CODE ADDED TO HERE*********************/

        return lock;
}

void
lock_destroy(struct lock *lock)
{
        //KASSERT(lock != NULL);

/********************CODE ADDED FROM HERE************************/
        spinlock_cleanup(&lock->lk_spinlock);
      	wchan_destroy(lock->lk_wchan);
/*******************CODE ADDED TO HERE***************************/

        kfree(lock->lk_name);
        kfree(lock);
}
/*
Goal: SEt the flag if nobody has the lock

                  Steps
1)Aquire the spinlock for the lock before accessing the member fields.
2)Put the thread to sleep if the lock is not avaliable
    a)wchan_sleep internally release the spinlock
    b)loop over this because other threads may be competing
3)Set the lock when exiting the loop
4)Release the spinlock
*/
void
lock_acquire(struct lock *lock)
{

//  KASSERT(lock != NULL);

//KASSERT(curthread->t_in_interrupt == false);

/****************CODE ADDED FROM HERE************************/
  spinlock_acquire(&lock->lk_spinlock);

  while (lock->lk_flag) {
    wchan_sleep(lock->lk_wchan, &lock->lk_spinlock);
  }

  //KASSERT(!lock->lk_flag);
  lock->lk_flag = true;

  lock->lk_thread = curthread;

                                                          //KASSERT()
  spinlock_release(&lock->lk_spinlock);
/*****************CODE ADDED TO HERE********************/

//        (void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
  KASSERT(lock != NULL);

  spinlock_acquire(&lock->lk_spinlock);

  lock->lk_flag = false;
  lock->lk_thread = NULL;
  //KASSERT(!lock->lk_flag);

  wchan_wakeone(lock->lk_wchan, &lock->lk_spinlock);

  spinlock_release(&lock->lk_spinlock);
        //(void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
        // Write this
        //if (!CURCPU_EXISTS()) {
      		//return true;
      	//}

        return (lock->lk_thread == curthread);

      	/* Assume we can read splk_holder atomically enough for this to work

      	return (splk->splk_holder == curcpu->c_self);
        (void)lock;  // suppress warning until code gets written

        return true; // dummy until code gets written*/
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(*cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }
/*********CODE ADDED FROM HERE************************/
        cv->cv_wchan = wchan_create(cv->cv_name);
      	if (cv->cv_wchan == NULL) {
      		kfree(cv->cv_name);
      		kfree(cv);
      		return NULL;
      	}

      	spinlock_init(&cv->cv_spinlock);
/******************CODE ADDED TO HERE******************/
        return cv;
}

void
cv_destroy(struct cv *cv)
{
  /********************CODE ADDED FROM HERE**********************/
        KASSERT(cv != NULL);

        spinlock_cleanup(&cv->cv_spinlock);
        wchan_destroy(cv->cv_wchan);

        kfree(cv->cv_name);
        kfree(cv);
/******************CODE ADDED TO HERE******************************/
}

/*********cv_wait(struct cv *cv, struct lock *lock)***********/
//CV must be passed as a parameter because C functions do not have
//an inplicit parent object
/*
Goal: Atomically put the thread to sleep and release the lock.
      And acquire the lock back when awaken
                Steps:
1)Acquire the spinlock for the CV first to ensure atomicity for
  the following steps:
      a)Release the lock
      b)Put the thread to sleep in the CV's wchan
2)Release the spinlock
3)Acquire the lock before completing the function.
      a)refer to wchan_sleep for a similar manipulation
*/
void
cv_wait(struct cv *cv, struct lock *lock)
{
        spinlock_acquire(&cv->cv_spinlock);

        lock_release(lock);
        wchan_sleep(cv->cv_wchan, &cv->cv_spinlock);
        spinlock_release(&cv->cv_spinlock);
        lock_acquire(lock);

        //(void)cv;    // suppress warning until code gets written
        //(void)lock;  // suppress warning until code gets written
}
/*
*    cv_signal    - Wake up one thread that's sleeping on this CV.
*/
void
cv_signal(struct cv *cv, struct lock *lock)
{

  spinlock_acquire(&cv->cv_spinlock);

  if( lock_do_i_hold(lock) ){
      wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
  }

  spinlock_release(&cv->cv_spinlock);

	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written
}

/*
*    cv_broadcast - Wake up all threads sleeping on this CV.
*/
void
cv_broadcast(struct cv *cv, struct lock *lock)
{
  spinlock_acquire(&cv->cv_spinlock);

  if( lock_do_i_hold(lock) ){
      wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
  }

  spinlock_release(&cv->cv_spinlock);
  //(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written
}
