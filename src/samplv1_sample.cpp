// samplv1_sample.cpp
//
/****************************************************************************
   Copyright (C) 2012-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_resampler.h"

#include <sndfile.h>


//-------------------------------------------------------------------------
// samplv1_reverse_sched - local module schedule thread stuff.
//

#include "samplv1_sched.h"


class samplv1_reverse_sched : public samplv1_sched
{
public:

	// ctor.
	samplv1_reverse_sched (samplv1 *pSampl, samplv1_sample *sample)
		: samplv1_sched(pSampl, Sample), m_sample(sample) {}

	// process reverse (virtual).
	void process(int)
		{ m_sample->reverse_sync(); }

private:

	// instance variables.
	samplv1_sample *m_sample;
};


//-------------------------------------------------------------------------
// samplv1_sample - sampler wave table.
//

// ctor.
samplv1_sample::samplv1_sample ( samplv1 *pSampl, float srate )
	: m_srate(srate), m_filename(NULL), m_nchannels(0),
		m_rate0(0.0f), m_freq0(1.0f), m_ratio(0.0f),
		m_nframes(0), m_pframes(NULL), m_reverse(false),
		m_loop(false), m_loop_start(0), m_loop_end(0)
{
	m_reverse_sched = new samplv1_reverse_sched(pSampl, this);
}


// dtor.
samplv1_sample::~samplv1_sample (void)
{
	close();

	delete m_reverse_sched;
}


// init.
bool samplv1_sample::open ( const char *filename, float freq0 )
{
	if (filename == NULL)
		return false;

	close();

	m_filename = ::strdup(filename);

	SF_INFO info;
	::memset(&info, 0, sizeof(info));

	SNDFILE *file = ::sf_open(m_filename, SFM_READ, &info);
	if (file == NULL)
		return false;

	m_nchannels = info.channels;
	m_rate0     = float(info.samplerate);
	m_nframes   = info.frames;

	float *buffer = new float [m_nchannels * m_nframes];

	const int nread = ::sf_readf_float(file, buffer, m_nframes);
	if (nread > 0) {
		// resample start...
		const uint32_t ninp = uint32_t(nread);
		const uint32_t rinp = uint32_t(m_rate0);
		const uint32_t rout = uint32_t(m_srate);
		if (rinp != rout) {
			samplv1_resampler resampler;
			const uint32_t nout = uint32_t(float(ninp) * m_srate / m_rate0);
			const uint32_t FILTSIZE = 32; // resample medium quality
			if (resampler.setup(rinp, rout, m_nchannels, FILTSIZE)) {
				float *inpb = buffer;
				float *outb = new float [m_nchannels * nout];
				resampler.inp_count = ninp;
				resampler.inp_data  = inpb;
				resampler.out_count = nout;
				resampler.out_data  = outb;
				resampler.process();
				buffer = outb;
				delete [] inpb;
				// identical rates now...
				m_rate0 = float(rout);
				m_nframes = nout;
			}
		}
		else m_nframes = ninp;
		// resample end.
	}

	const uint32_t nsize = m_nframes + 4;
	m_pframes = new float * [m_nchannels];
	for (uint16_t k = 0; k < m_nchannels; ++k) {
		m_pframes[k] = new float [nsize];
		::memset(m_pframes[k], 0, nsize * sizeof(float));
	}

	uint32_t i = 0;
	for (uint32_t j = 0; j < m_nframes; ++j) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			m_pframes[k][j] = buffer[i++];
	}

	delete [] buffer;
	::sf_close(file);

	if (m_reverse)
		reverse_sync();

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
		m_pframes = NULL;
	}

	m_nframes   = 0;
	m_ratio     = 0.0f;
	m_freq0     = 1.0f;
	m_rate0     = 0.0f;
	m_nchannels = 0;

	if (m_filename) {
		::free(m_filename);
		m_filename = NULL;
	}

	setLoopRange(0, 0);
}


// schedule sample reverse.
void samplv1_sample::reverse_sched (void)
{
	m_reverse_sched->schedule();
}


// reverse sample buffer.
void samplv1_sample::reverse_sync (void)
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
