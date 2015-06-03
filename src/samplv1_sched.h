// samplv1_sched.h
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __samplv1_sched_h
#define __samplv1_sched_h

#include <stdint.h>


//-------------------------------------------------------------------------
// samplv1_sched - worker/scheduled stuff (pure virtual).
//

class samplv1_sched
{
public:

	// plausible sched types.
	enum Type { Sample, Programs, Controls };

	// ctor.
	samplv1_sched(Type stype, uint32_t nsize = 8);

	// virtual dtor.
	virtual ~samplv1_sched();

	// schedule process.
	void schedule(int sid = 0);

	// test-and-set wait.
	bool sync_wait();
	
	// scheduled processor.
	void sync_process();

	// (pure) virtual processor.
	virtual void process(int sid) = 0;

	// signal broadcast (static).
	static void sync_notify(Type stype, int sid);

private:

	// instance variables.
	Type m_stype;

	// sched queue instance reference.
	uint32_t m_nsize;
	uint32_t m_nmask;

	int *m_items;

	volatile uint32_t m_iread;
	volatile uint32_t m_iwrite;

	volatile bool m_sync_wait;
};


//-------------------------------------------------------------------------
// samplv1_sched_notifier - worker/schedule proxy decl.
//

class samplv1_sched_notifier
{
public:

	// ctor.
	samplv1_sched_notifier();

	// dtor.
	~samplv1_sched_notifier();

	// signal notifier.
	virtual void notify(samplv1_sched::Type stype, int sid) const = 0;
};


#endif	// __samplv1_sched_h

// end of samplv1_sched.h
