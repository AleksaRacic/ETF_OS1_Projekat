/*
 * kernel.cpp
 *
 *  Created on: Sep 15, 2021
 *      Author: OS1
 */

#include "kernel.h"
#include "SCHEDULE.H"
#include "hlpThr.h"

#include <STDIO.H>

volatile void interrupt (*Kernel::oldTimer)(...) = NULL;

volatile int Kernel::switch_context_req_timer = 0;
volatile int Kernel::switch_context_req_disp = 0;
volatile PCB* Kernel::idle = NULL;
volatile PCB* Kernel::running = NULL;
volatile PCB* Kernel::mainPCB = NULL;
volatile int Kernel::int_locked = 0;

unsigned int tsp, tss, tbp;

Thread* Kernel::idleT = NULL;

void Kernel::dispatch(){
#ifndef BCC_BLOCK_IGNORE
	lock();
#endif
	switch_context_req_disp = 1;
	myTimer();
#ifndef BCC_BLOCK_IGNORE
	unlock();
#endif

}

void interrupt Kernel::myTimer(...){
	int time_left;
	//printf("_%d_%d_%d//", switch_context_req_disp, switch_context_req_timer, running->getID());
	if(!switch_context_req_disp){
		tick();
		(*oldTimer)();
		switch_context_req_timer = running->decProcessorTime();
		//printf(" %d_%d ", running->time_left, switch_context_req_timer);
		//dodati semafore
	}

	if(!int_locked && (switch_context_req_timer || switch_context_req_disp)){
		//printf("_%d_%d ", switch_context_req_disp, switch_context_req_timer);
		//printf("K d\n");
		switch_context_req_disp = 0;
		switch_context_req_timer = 0;
		PCB *next_thread = NULL;
		printf("_%d->",running->getID());
		if((running->getStatus() == READY || running->getStatus() == RUNNING) && running != idle)
			Scheduler::put((PCB*)running);

		if((next_thread = Scheduler::get())== NULL)
			next_thread = (PCB*) idle;

		#ifndef BCC_BLOCK_IGNORE
		asm{
			mov tsp, sp
			mov tss, ss
			mov tbp, bp
		}
		#endif

		running->sp = tsp;
		running->ss = tss;
		running->bp = tbp;

		running = next_thread;
		printf("%d\n",running->getID());
		running->resetMyTime();

		tsp = running->sp;
		tss = running->ss;
		tbp = running->bp;

		#ifndef BCC_BLOCK_IGNORE
		asm{
			mov sp, tsp
			mov ss, tss
			mov bp, tbp
		}
		#endif
	}
}

void Kernel::boot(){
#ifndef BCC_BLOCK_IGNORE
	lock(); //zaustavljamo sve interrupte dok se sistem pali
	oldTimer = getvect(0x08);
	setvect(0x08, myTimer);
#endif

	mainPCB = new PCB(); //videti da li ce zbog ovoga pucati(ne bi trebalo jer se ne poziva run)
	running = mainPCB;

	idleT = new idleThread();
	idle = idleT->myPCB;
	//dodati semafore i ostalo

#ifndef BCC_BLOCK_IGNORE

	unlock();
#endif
}

void Kernel::restore(){
	#ifndef BCC_BLOCK_IGNORE
		lock(); //zaustavljamo sve interrupte dok se sistem pali
		//setvect(0x08, oldTimer);
	#endif
		//delete idleT;
		delete mainPCB;
	#ifndef BCC_BLOCK_IGNORE
		unlock(); //zaustavljamo sve interrupte dok se sistem pali
	#endif
}


