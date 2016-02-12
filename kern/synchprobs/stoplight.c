/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */
struct lock* global; 
struct lock* locks[4];

void
stoplight_init() {
	global = lock_create("global");
	locks[0]=lock_create("0");
	locks[1]=lock_create("1");
	locks[2]=lock_create("2");
	locks[3]=lock_create("3");
	return;
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {

	lock_destroy(global);
	lock_destroy(locks[0]);
	lock_destroy(locks[1]);
	lock_destroy(locks[2]);
	lock_destroy(locks[3]);
	return;

}

void
turnright(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;


	//lock_acquire(global);
	lock_acquire(locks[direction]);
	inQuadrant(direction,index);
	leaveIntersection(index);
	lock_release(locks[direction]);
	//lock_release(global);	
	
	 
	
	return;
}
void
gostraight(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;

	int max,min;
	if(direction>((direction+3)%4))
	{
		max=direction;
		min=(direction+3)%4;
	}	
	else
	{
		min=direction;
		max=(direction+3)%4;
	}

	//lock_acquire(global);
	lock_acquire(locks[min]);
	lock_acquire(locks[max]);
	inQuadrant(direction,index);
	inQuadrant((direction+3)%4,index);
	leaveIntersection(index);
	lock_release(locks[max]);
	lock_release(locks[min]);
	
	//lock_release(global);
	
		
	
	return;
}
void
turnleft(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;
	
	//lock_acquire(global);
	
	
	int i1,i2,i3;
	i1=direction;
	i2=(direction+3)%4;
	i3=(direction+2)%4;
	//lock_release(global);

	int min,mid,max;

	if(i1>i2 && i1>i3)
	{
		max=i1;
		if(i2>i3)
		{
			mid=i2;
			min=i3;
		}	
		else
		{
			mid=i3;
			min=i2;
		}	
	}	
	else if(i2>i1 && i2>i3)
	{
		max=i2;
		if(i1>i3)
		{
			mid=i1;
			min=i3;
		}
		else
		{
			mid=i3;
			min=i1;
		}	
	}
	else
	{
		max=i3;
		if(i1>i2)
		{
			mid=i1;
			min=i2;
		}
		else
		{
			mid=i2;
			min=i1;
		}	
	}	

	lock_acquire(locks[min]);
	lock_acquire(locks[mid]);
	lock_acquire(locks[max]);
	inQuadrant(direction,index);
	inQuadrant((direction+3)%4,index);
	inQuadrant((direction+2)%4,index);
	leaveIntersection(index);
	lock_release(locks[max]);
	lock_release(locks[mid]);
	lock_release(locks[min]);


	return;
}
