// samplv1_resampler.cpp
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

#include "samplv1_resampler.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>


// ----------------------------------------------------------------------------
// samplv1_resampler

static float sinc ( float x )
{
	x = ::fabsf(x);
	if (x < 1e-6f)
		return 1.0f;
	x *= M_PI;
	return ::sinf(x) / x;
}


static float wind ( float x )
{
	x = ::fabsf(x);
	if (x >= 1.0f)
		return 0.0f;
	x *= M_PI;
	return 0.384f + 0.5f * ::cosf(x) + 0.116f * ::cosf(2.0f * x);
}


samplv1_resampler::Table *samplv1_resampler::Table::g_list = nullptr;
samplv1_resampler::Mutex  samplv1_resampler::Table::g_mutex;


samplv1_resampler::Table::Table (
	float fr0, unsigned int hl0, unsigned int np0 )
	: next(nullptr), refc(0), ctab(nullptr), fr(fr0), hl(hl0), np(np0)
{
	unsigned int i, j;
	float t;
	float *ptab;

	ctab = new float [hl * (np + 1)];
	ptab = ctab;
	for (j = 0; j <= np; ++j) {
		t = float(j) / float(np);
		for (i = 0; i < hl; ++i) {
			ptab[hl - i - 1] = float(fr * sinc(t * fr) * wind(t / hl));
			t += 1.0f;
		}
		ptab += hl;
	}
}


samplv1_resampler::Table::~Table (void)
{
	delete [] ctab;
}


samplv1_resampler::Table *samplv1_resampler::Table::create (
	float fr0, unsigned int hl0, unsigned int np0 )
{
	Table *p;

	g_mutex.lock();

	p = g_list;

	while (p) {
		if ((fr0 >= p->fr * 0.999f) &&
			(fr0 <= p->fr * 1.001f) &&
			(hl0 == p->hl) && (np0 == p->np)) {
			p->refc++;
			g_mutex.unlock();
			return p;
		}
		p = p->next;
	}

	p = new samplv1_resampler::Table(fr0, hl0, np0);
	p->refc = 1;
	p->next = g_list;
	g_list  = p;

	g_mutex.unlock();

	return p;
}


void samplv1_resampler::Table::destroy ( samplv1_resampler::Table *table )
{
	Table *p, *q;

	g_mutex.lock();

	if (table) {
		table->refc--;
		if (table->refc == 0) {
			p = g_list;
			q = nullptr;
			while (p) {
				if (p == table) {
					if (q)
						q->next = table->next;
					else
						g_list = table->next;
					break;
				}
				q = p;
				p = p->next;
			}
			delete table;
		}
	}

	g_mutex.unlock();
}


// ----------------------------------------------------------------------------
// samplv1_resampler


static unsigned int gcd ( unsigned int a, unsigned int b )
{
	if (a == 0) return b;
	if (b == 0) return a;

	while (1) {
		if (a > b) {
			a = a % b;
			if (a == 0) return b;
			if (a == 1) return 1;
		} else {
			b = b % a;
			if (b == 0) return a;
			if (b == 1) return 1;
		}
	}    

	return 1; 
}


samplv1_resampler::samplv1_resampler (void)
	: m_table(nullptr), m_nchan(0), m_buff(nullptr)
{
	reset();
}


samplv1_resampler::~samplv1_resampler (void)
{
	clear();
}


bool samplv1_resampler::setup (
	unsigned int fs_inp, unsigned int fs_out,
	unsigned int nchan, unsigned int hlen )
{
	if ((hlen < 8) || (hlen > 96))
		return false;

	return setup(fs_inp, fs_out, nchan, hlen, 1.0f - 2.6f / hlen);
}


bool samplv1_resampler::setup (
	unsigned int fs_inp, unsigned int fs_out,
	unsigned int nchan, unsigned int hlen, float frel )
{
	unsigned int g, h, k, n, s;
	float r;
	float *buff = 0;
	Table *table = 0;

	k = s = 0;

	if (fs_inp && fs_out && nchan) {
		r = float(fs_out) / float(fs_inp);
			g = gcd(fs_out, fs_inp);
			n = fs_out / g;
		s = fs_inp / g;
		if ((16 * r >= 1) && (n <= 1000)) {
			h = hlen;
			k = 250;
			if (r < 1) {
				frel *= r;
				h = (unsigned int) (::ceilf(h / r));
				k = (unsigned int) (::ceilf(k / r));
			}
			table = Table::create(frel, h, n);
			buff = new float [nchan * (2 * h - 1 + k)];
		}
	}

	clear();

	if (table) {
		m_table = table;
		m_buff  = buff;
		m_nchan = nchan;
		m_inmax = k;
		m_pstep = s;
		return reset();
	}

	return false;
}


void samplv1_resampler::clear (void)
{
	Table::destroy(m_table);

	delete [] m_buff;

	m_buff  = nullptr;
	m_table = nullptr;
	m_nchan = 0;
	m_inmax = 0;
	m_pstep = 0;

	reset();
}


int samplv1_resampler::inpdist (void) const
{
	if (m_table)
		return int(m_table->hl + 1 - m_nread) - int(m_phase / m_table->np);
	else
		return 0;
}


int samplv1_resampler::inpsize (void) const
{
	if (m_table)
		return 2 * m_table->hl;
	else
		return 0;
}


bool samplv1_resampler::reset (void)
{
	if (m_table == nullptr)
		return false;

	inp_count = 0;
	out_count = 0;
	inp_data  = nullptr;
	out_data  = nullptr;

	m_index = 0;
	m_nread = 0;
	m_nzero = 0;
	m_phase = 0; 

	if (m_table) {
		m_nread = 2 * m_table->hl;
		return true;
	}

	return false;
}


bool samplv1_resampler::process (void)
{
	unsigned int hl, ph, np, dp, in, nr, nz, i, n, c;
	float *p1, *p2;

	if (m_table == nullptr)
		return false;

	hl = m_table->hl;
	np = m_table->np;
	dp = m_pstep;
	in = m_index;
	nr = m_nread;
	ph = m_phase;
	nz = m_nzero;
	n  = (2 * hl - nr) * m_nchan;
	p1 = m_buff + in * m_nchan;
	p2 = p1 + n;

	while (out_count) {
		if (nr) {
			if (inp_count == 0)
				break;
			if (inp_data) {
				for (c = 0; c < m_nchan; ++c)
					p2[c] = inp_data[c];
				inp_data += m_nchan;
				nz = 0;
			} else {
				for (c = 0; c < m_nchan; ++c)
					p2[c] = 0;
				if (nz < 2 * hl)
					nz++;
			}
			nr--;
			p2 += m_nchan;
			inp_count--;
		} else {
			if (out_data) {
				if (nz < 2 * hl) {
					float *c1 = m_table->ctab + hl * ph;
					float *c2 = m_table->ctab + hl * (np - ph);
					for (c = 0; c < m_nchan; ++c) {
						float *q1 = p1 + c;
						float *q2 = p2 + c;
						float s = 1e-20f;
						for (i = 0; i < hl; ++i) {
							q2 -= m_nchan;
							s += *q1 * c1[i] + *q2 * c2[i];
							q1 += m_nchan;
						}
						*out_data++ = s - 1e-20f;
					}
				} else {
					for (c = 0; c < m_nchan; ++c)
						*out_data++ = 0.0f;
				}
			}
			out_count--;
			ph += dp;
			if (ph >= np) {
				nr = ph / np;
				ph -= nr * np;
				in += nr;
				p1 += nr * m_nchan;;
				if (in >= m_inmax) {
					n = (2 * hl - nr) * m_nchan;
					::memcpy(m_buff, p1, n * sizeof(float));
					in = 0;
					p1 = m_buff;
					p2 = p1 + n;
				}
			}
		}
	}

	m_index = in;
	m_nread = nr;
	m_phase = ph;
	m_nzero = nz;

    return true;
}


// end of samplv1_resampler.cpp
