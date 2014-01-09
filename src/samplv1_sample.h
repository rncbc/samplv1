// samplv1_sample.h
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

#ifndef __samplv1_sample_h
#define __samplv1_sample_h

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

#include <sndfile.h>


//-------------------------------------------------------------------------
// samplv1_sample - sampler wave table.
//

class samplv1_sample
{
public:

	// ctor.
	samplv1_sample(float srate = 44100.0f)
		: m_srate(srate), m_filename(0), m_nchannels(0),
			m_rate0(0.0f), m_freq0(1.0f), m_ratio(0.0f),
			m_nframes(0), m_pframes(0), m_loop(false),
			m_loop_start(0), m_loop_end(0) {}

	// dtor.
	~samplv1_sample()
		{ close(); }

	// nominal sample-rate.
	void setSampleRate(float srate)
		{ m_srate = srate; }
	float sampleRate() const
		{ return m_srate; }

	// loop mode.
	void setLoop ( bool loop )
	{
		m_loop = loop;

		if (m_loop && m_loop_start >= m_loop_end) {
			m_loop_start = 0;
			m_loop_end = m_nframes;
		}
	}

	bool isLoop() const
		{ return m_loop && (m_loop_start < m_loop_end); }

	// loop range.
	void setLoopRange ( uint32_t start, uint32_t end )
	{
		if (start > m_nframes)
			start = m_nframes;

		if (end > m_nframes)
			end = m_nframes;

		if (start < end) {
			m_loop_start = start;
			m_loop_end   = end;
		} else {
			m_loop_start = 0;
			m_loop_end   = 0;
		}
	}

	uint32_t loopStart (void) const
		{ return m_loop_start; }
	uint32_t loopEnd (void) const
		{ return m_loop_end; }

	// init.
	bool open(const char *filename, float freq0 = 1.0f)
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

		m_pframes = new float * [m_nchannels];
		for (uint16_t k = 0; k < m_nchannels; ++k)
			m_pframes[k] = new float [m_nframes];

		float *buffer = new float [m_nchannels * m_nframes];

		int nread = ::sf_readf_float(file, buffer, m_nframes);
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

		reset(freq0);

		setLoop(m_loop);
		return true;
	}

	void close()
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

	// accessors.
	const char *filename() const
		{ return m_filename; }
	uint16_t channels() const
		{ return m_nchannels; }
	float rate() const
		{ return m_rate0; }
	float freq() const
		{ return m_freq0; }
	uint32_t length() const
		{ return m_nframes; }

	// resampler ratio
	float ratio() const
		{ return m_ratio; }

	// reset.
	void reset(float freq0)
	{
		m_freq0 = freq0;
		m_ratio = m_rate0 / (m_freq0 * m_srate);
	}

	// frame value.
	float *frames(uint16_t k) const
		{ return m_pframes[k]; }

	// predicate.
	bool isOver(uint32_t frame) const
		{ return !m_pframes || (frame >= m_nframes - 4); }

private:

	float    m_srate;
	char    *m_filename;
	uint16_t m_nchannels;
	float    m_rate0;
	float    m_freq0;
	float    m_ratio;
	uint32_t m_nframes;
	float  **m_pframes;
	bool     m_loop;
	float    m_loop_start;
	float    m_loop_end;
};


//-------------------------------------------------------------------------
// samplv1_generator - sampler oscillator (sort of:)

class samplv1_generator
{
public:

	// ctor.
	samplv1_generator(samplv1_sample *sample = 0) { reset(sample); }

	// sample accessor.
	samplv1_sample *sample() const
		{ return m_sample; }

	// reset.
	void reset(samplv1_sample *sample)
	{
		m_sample = sample;

		m_phase  = 1.0f;
		m_index  = 1;
		m_alpha  = 0.0f;
		m_frame  = 0;

		m_loop   = false;
		m_phase1 = 0.0f;
		m_phase2 = 0.0f;
		m_gain   = 1.0f;
	}

	// reset loop.
	void setLoop(bool loop)
	{
		m_loop = loop;

		if (m_loop) {
			int slope = 0;
			uint32_t end = zero_crossing(m_sample->loopEnd(), &slope);
			uint32_t start = zero_crossing(m_sample->loopStart(), &slope);
			if (start >= end) {
				start = m_sample->loopStart();
				end = m_sample->loopEnd();
			}
			m_phase1 = float(end - start);
			m_phase2 = float(end);
		}

		m_gain = 1.0f;
	}

	// begin.
	void start(void)
	{
		m_phase = 1.0f;
		m_index = 1;
		m_alpha = 0.0f;
		m_frame = 0;

		setLoop(m_sample->isLoop());
	}

	// iterate.
	void next(float freq)
	{
		const float delta = freq * m_sample->ratio();

		m_index  = int(m_phase);
		m_alpha  = m_phase - float(m_index);
		m_phase += delta;

		if (m_loop) {
			const float xtime = 32.0f; // frames.
			const float xstep = 1.0f / xtime;
			const float xfade = xtime * delta;
			if (m_phase >= m_phase2 - xfade)
				m_gain -= xstep;
			else
			if (m_gain < 1.0f)
				m_gain += xstep;
			if (m_phase >= m_phase2) {
				m_phase -= m_phase1;
				if (m_phase < 1.0f) {
					m_phase = 1.0f;
					m_gain = xstep;
				} else {
					m_gain = 0.0f;
				}
			}
		}

		if (m_frame < m_index)
			m_frame = m_index;
	}

	// sample.
	float value(uint16_t k) const
	{
		if (isOver())
			return 0.0f;

		const float *frames = m_sample->frames(k);

		const float x0 = frames[m_index - 1];
		const float x1 = frames[m_index];
		const float x2 = frames[m_index + 1];
		const float x3 = frames[m_index + 2];

		const float c1 = (x2 - x0) * 0.5f;
		const float b1 = (x1 - x2);
		const float b2 = (c1 + b1);
		const float c3 = (x3 - x1) * 0.5f + b2 + b1;
		const float c2 = (c3 + b2);

		return m_gain * ((((c3 * m_alpha) - c2) * m_alpha + c1) * m_alpha + x1);
	}

	// predicate.
	bool isOver() const
		{ return !m_loop && m_sample->isOver(m_frame); }

protected:

	// zero-crossing aliasing (single channel).
	uint32_t zero_crossing_k ( uint32_t i, uint16_t k, int *slope ) const
	{
		const uint32_t nframes = m_sample->length();
		const float *frames = m_sample->frames(k);
		const int s0 = (slope ? *slope : 0);

		float v0 = frames[i];
		for (++i; i < nframes; ++i) {
			const float v1 = frames[i];
			if ((0 >= s0 && v0 >= 0.0f && v1 < 0.0f) ||
				(s0 >= 0 && v1 >= 0.0f && v0 < 0.0f)) {
				if (slope) *slope = (v1 < v0 ? -1 : +1);
				return i;
			}
			v0 = v1;
		}

		return nframes;
	}

	// zero-crossing aliasing (median).
	uint32_t zero_crossing ( uint32_t i, int *slope = 0 ) const
	{
		const uint16_t nchannels = m_sample->channels();

		uint32_t sum = 0;
		for (uint16_t k = 0; k < nchannels; ++k)
			sum += zero_crossing_k(i, k, slope);

		return (sum / nchannels);
	}

private:

	// iterator variables.
	samplv1_sample *m_sample;

	float    m_phase;
	uint32_t m_index;
	float    m_alpha;
	uint32_t m_frame;

	bool     m_loop;
	float    m_phase1;
	float    m_phase2;
	float    m_gain;
};


#endif	// __samplv1_sample_h

// end of samplv1_sample.h
