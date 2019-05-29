// samplv1_wave.cpp
//
/****************************************************************************
   Copyright (C) 2012-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_wave.h"

#include <stdlib.h>
#include <math.h>


//-------------------------------------------------------------------------
// samplv1_wave - smoothed (integrating oversampled) wave table.
//

// ctor.
samplv1_wave::samplv1_wave ( uint32_t nsize, uint16_t nover )
	: m_nsize(nsize), m_nover(nover),
		m_shape(Pulse), m_width(1.0f),
		m_srate(44100.0f), m_phase0(0.0f), m_srand(0)
{
	m_table = new float [m_nsize + 4];

	reset(m_shape, m_width);
}


// dtor.
samplv1_wave::~samplv1_wave (void)
{
	delete [] m_table;
}


// init.
void samplv1_wave::reset ( Shape shape, float width )
{
	m_shape = shape;
	m_width = width;;

	switch (m_shape) {
	case Pulse:
		reset_pulse();
		break;
	case Saw:
		reset_saw();
		break;
	case Sine:
		reset_sine();
		break;
	case Rand:
		reset_rand();
		break;
	case Noise:
		reset_noise();
		// Fall thru...
	default:
		break;
	}
}


// init pulse table.
void samplv1_wave::reset_pulse (void)
{
	const float p0 = float(m_nsize);
	const float w2 = p0 * m_width * 0.5f;

	for (uint32_t i = 0; i < m_nsize; ++i) {
		const float p = float(i);
		m_table[i] = (p < w2 ? 1.0f : -1.0f);
	}

	reset_filter();
	reset_normalize();
	reset_interp();
}


// init saw table.
void samplv1_wave::reset_saw (void)
{
	const float p0 = float(m_nsize);
	const float w0 = p0 * m_width;

	for (uint32_t i = 0; i < m_nsize; ++i) {
		const float p = float(i);
		if (p < w0) {
			m_table[i] = 2.0f * p / w0 - 1.0f;
		} else {
			m_table[i] = 1.0f - 2.0f * (1.0f + (p - w0)) / (p0 - w0);
		}
	}

	reset_filter();
	reset_normalize();
	reset_interp();
}


// init sine table.
void samplv1_wave::reset_sine (void)
{
	const float p0 = float(m_nsize);
	const float w0 = p0 * m_width;
	const float w2 = w0 * 0.5f;

	for (uint32_t i = 0; i < m_nsize; ++i) {
		float p = float(i);
		if (p < w2)
			m_table[i] = ::sinf(2.0f * M_PI * p / w0);
		else
			m_table[i] = ::sinf(M_PI * (p + (p0 - w0)) / (p0 - w2));
	}

	if (m_width < 1.0f) {
		reset_filter();
		reset_normalize();
	}

	reset_interp();
}


// init random table.
void samplv1_wave::reset_rand (void)
{
	const float p0 = float(m_nsize);
	const float w0 = p0 * m_width;
	const uint32_t ihold = (uint32_t(p0 - w0) >> 3) + 1;

	m_srand = uint32_t(w0);

	float p = 0.0f;

	for (uint32_t i = 0; i < m_nsize; ++i) {
		if ((i % ihold) == 0)
			p = pseudo_randf();
		m_table[i] = p;
	}

	reset_filter();
	reset_normalize();
	reset_interp();
}


// init noise table.
void samplv1_wave::reset_noise (void)
{
	const float p0 = float(m_nsize);
	const float w0 = p0 * m_width;

	m_srand = uint32_t(w0) ^ 0x9631; // magic!

	for (uint32_t i = 0; i < m_nsize; ++i)
		m_table[i] = pseudo_randf();

//	reset_filter();
//	reset_normalize();
	reset_interp();
}


// post-processors
void samplv1_wave::reset_filter (void)
{
	uint32_t i, k = 0;

	for (i = 1; i < m_nsize; ++i) {
		const float p1 = m_table[i - 1];
		const float p2 = m_table[i];
		if (p1 < 0.0f && p2 >= 0.0f) {
			k = i;
			break;
		}
	}

	for (uint16_t n = 0; n < m_nover; ++n) {
		float p = m_table[k];
		for (i = 0; i < m_nsize; ++i) {
			if (++k >= m_nsize) k = 0;
			p = 0.5f * (m_table[k] + p);
			m_table[k] = p;
		}
	}
}


void samplv1_wave::reset_normalize (void)
{
	uint32_t i;

	float pmax = 0.0f;
	float pmin = 0.0f;

	for (i = 0; i < m_nsize; ++i) {
		const float p = m_table[i];
		if (pmax < p)
			pmax = p;
		else
		if (pmin > p)
			pmin = p;
	}

	const float pmid = 0.5f * (pmax + pmin);

	pmax = 0.0f;
	for (i = 0; i < m_nsize; ++i) {
		m_table[i] -= pmid;
		const float p = ::fabsf(m_table[i]);
		if (pmax < p)
			pmax = p;
	}

	if (pmax > 0.0f) {
		const float gain = 1.0f / pmax;
		for (i = 0; i < m_nsize; ++i)
			m_table[i] *= gain;
	}
}


void samplv1_wave::reset_interp (void)
{
	uint32_t i, k = 0;

	for (i = m_nsize; i < m_nsize + 4; ++i)
		m_table[i] = m_table[i - m_nsize];

	for (i = 1; i < m_nsize; ++i) {
		const float p1 = m_table[i - 1];
		const float p2 = m_table[i];
		if (p1 < 0.0f && p2 >= 0.0f)
			k = i;
	}

	m_phase0 = float(k) / float(m_nsize);
}


// end of samplv1_wave.cpp
