// samplv1_sample.cpp
//
/****************************************************************************
   Copyright (C) 2012-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_sample.h"

#include <sndfile.h>


//-------------------------------------------------------------------------
// samplv1_sample - sampler wave table.
//

// ctor.
samplv1_sample::samplv1_sample ( float srate )
	: m_srate(srate), m_filename(0), m_nchannels(0),
		m_rate0(0.0f), m_freq0(1.0f), m_ratio(0.0f),
		m_nframes(0), m_pframes(0), m_reverse(false),
		m_loop(false), m_loop_start(0), m_loop_end(0)
{
}


// dtor.
samplv1_sample::~samplv1_sample (void)
{
	close();
}


// init.
bool samplv1_sample::open ( const char *filename, float freq0 )
{
	if (!filename)
		return false;

	close();

	m_filename = ::strdup(filename);

	SF_INFO info;
	::memset(&info, 0, sizeof(info));

	SNDFILE *file = ::sf_open(m_filename, SFM_READ, &info);
	if (!file)
		return false;

	m_nchannels = info.channels;
	m_rate0     = float(info.samplerate);
	m_nframes   = info.frames;

	const uint32_t nsize = m_nframes + 4;

	m_pframes = new float * [m_nchannels];
	for (uint16_t k = 0; k < m_nchannels; ++k) {
		m_pframes[k] = new float [nsize];
		::memset(m_pframes[k], 0, nsize * sizeof(float));
	}

	float *buffer = new float [m_nchannels * m_nframes];

	const int nread = ::sf_readf_float(file, buffer, m_nframes);
	if (nread > 0) {
		const uint32_t n = uint32_t(nread);
		uint32_t i = 0;
		for (uint32_t j = 0; j < n; ++j) {
			for (uint16_t k = 0; k < m_nchannels; ++k)
				m_pframes[k][j] = buffer[i++];
		}
	}

	delete [] buffer;
	::sf_close(file);

	if (m_reverse)
		reverse_sample();

	reset(freq0);

	setLoop(m_loop);
	return true;
}


void samplv1_sample::close (void)
{
	if (m_pframes) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			delete [] m_pframes[k];
		delete [] m_pframes;
		m_pframes = 0;
	}

	m_nframes   = 0;
	m_ratio     = 0.0f;
	m_freq0     = 1.0f;
	m_rate0     = 0.0f;
	m_nchannels = 0;

	if (m_filename) {
		::free(m_filename);
		m_filename = 0;
	}

	setLoopRange(0, 0);
}


// reverse sample buffer.
void samplv1_sample::reverse_sample (void)
{
	if (m_nframes > 0 && m_pframes) {
		const uint32_t nsize1 = (m_nframes - 1);
		const uint32_t nsize2 = (m_nframes >> 1);
		for (uint16_t k = 0; k < m_nchannels; ++k) {
			float *frames = m_pframes[k];
			for (uint32_t i = 0; i < nsize2; ++i) {
				const uint32_t j = nsize1 - i;
				const float sample = frames[i];
				frames[i] = frames[j];
				frames[j] = sample;
			}
		}
	}
}


// end of samplv1_sample.cpp
