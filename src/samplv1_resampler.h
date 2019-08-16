// samplv1_resampler.h
//
/****************************************************************************
   Copyright (C) 2017-2019, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2006-2012 Fons Adriaensen <fons@linuxaudio.org>

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

#ifndef __samplv1_resampler_h
#define __samplv1_resampler_h

#include <pthread.h>


// ----------------------------------------------------------------------------
// samplv1_resampler

class samplv1_resampler
{
public:

	samplv1_resampler();
	~samplv1_resampler();

	bool setup(unsigned int fs_inp, unsigned int fs_out,
		unsigned int nchan, unsigned int hlen);

	bool setup(unsigned int fs_inp, unsigned int fs_out,
		unsigned int nchan, unsigned int hlen, float frel);

	void clear();
	bool reset();
	int  nchan()   const { return m_nchan; }
	int  inpsize() const;
	int  inpdist() const; 
	bool process();

	unsigned int inp_count;
	unsigned int out_count;
	float       *inp_data;
	float       *out_data;

	class Mutex
	{
	public:

		Mutex() { pthread_mutex_init(&m_mutex, 0); }
		~Mutex() { pthread_mutex_destroy(&m_mutex); }

		void lock() { pthread_mutex_lock(&m_mutex); }
		void unlock() { pthread_mutex_unlock(&m_mutex); }

	private:

		pthread_mutex_t m_mutex;
	};

	class Table
	{
	public:

		Table(float fr0, unsigned int hl0, unsigned int np0);
		~Table();

		Table        *next;
		unsigned int  refc;
		float        *ctab;
		float         fr;
		unsigned int  hl;
		unsigned int  np;

		static Table *create(float fr0, unsigned int hl0, unsigned int np0);
		static void destroy(Table *table);

	private:

		static Table *g_list;
		static Mutex  g_mutex;
	};

private:

	Table        *m_table;
	unsigned int  m_nchan;
	unsigned int  m_inmax;
	unsigned int  m_index;
	unsigned int  m_nread;
	unsigned int  m_nzero;
	unsigned int  m_phase;
	unsigned int  m_pstep;
	float        *m_buff;
	void         *m_dummy[8];
};


#endif	// __samplv1_resampler_h

// end of psolva1_resampler.h
