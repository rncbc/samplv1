// samplv1_sample.h
//
/****************************************************************************
   Copyright (C) 2012-2018, rncbc aka Rui Nuno Capela. All rights reserved.

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


// forward decls.
class samplv1;
class samplv1_reverse_sched;


//-------------------------------------------------------------------------
// samplv1_sample - sampler wave table.
//

class samplv1_sample
{
public:

	// ctor.
	samplv1_sample(samplv1 *pSampl, float srate = 44100.0f);

	// dtor.
	~samplv1_sample();

	// nominal sample-rate.
	void setSampleRate(float srate)
		{ m_srate = srate; }
	float sampleRate() const
		{ return m_srate; }

	// reverse mode.
	void setReverse(bool reverse)
		{ reverse_test(reverse); }

	bool isReverse() const
		{ return m_reverse; }

	// schedule sample reverse.
	void reverse_test(bool reverse)
	{
		if (( m_reverse && !reverse) ||
			(!m_reverse &&  reverse)) {
			m_reverse = reverse;
			reverse_sched();
		}
	}

	// reverse sample buffer.
	void reverse_sched();
	void reverse_sync();

	// loop mode.
	void setLoop(bool loop)
	{
		m_loop = loop;

		if (m_loop && m_loop_start >= m_loop_end) {
			m_loop_start = 0;
			m_loop_end = m_nframes;
			m_loop_phase1 = m_loop_phase2 = float(m_nframes);
		}
	}

	bool isLoop() const
		{ return m_loop && (m_loop_start < m_loop_end); }

	// loop change.
	bool loop_test(bool loop)
	{
		if ((m_loop && !loop) || (!m_loop && loop)) {
			setLoop(loop);
			return true;
		} else {
			return false;
		}
	}

	// loop range.
	void setLoopRange(uint32_t start, uint32_t end);

	uint32_t loopStart() const
		{ return m_loop_start; }
	uint32_t loopEnd() const
		{ return m_loop_end; }

	float loopPhase1() const
		{ return m_loop_phase1; }
	float loopPhase2() const
		{ return m_loop_phase2; }

	// loop cross-fade (in number of frames)
	void setLoopCrossFade(uint32_t xfade)
		{ m_loop_xfade = xfade; }
	uint32_t loopCrossFade() const
		{ return m_loop_xfade; }

	// loop zero-crossing detection
	void setLoopZeroCrossing(bool xzero)
		{ m_loop_xzero = xzero; }
	bool isLoopZeroCrossing() const
		{ return m_loop_xzero; }

	// init.
	bool open(const char *filename, float freq0 = 1.0f);
	void close();

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
		{ return !m_pframes || (frame >= m_nframes); }

protected:

	// zero-crossing aliasing .
	uint32_t zero_crossing_k(uint32_t i, uint16_t k, int *slope) const;
	uint32_t zero_crossing(uint32_t i, int *slope) const;

private:

	// instance variables.
	float    m_srate;
	char    *m_filename;
	uint16_t m_nchannels;
	float    m_rate0;
	float    m_freq0;
	float    m_ratio;
	uint32_t m_nframes;
	float  **m_pframes;
	bool     m_reverse;

	bool     m_loop;
	uint32_t m_loop_start;
	uint32_t m_loop_end;
	float    m_loop_phase1;
	float    m_loop_phase2;
	uint32_t m_loop_xfade;
	bool     m_loop_xzero;

	samplv1_reverse_sched *m_reverse_sched;
};


//-------------------------------------------------------------------------
// samplv1_generator - sampler oscillator (sort of:)

class samplv1_generator
{
public:

	// ctor.
	samplv1_generator(samplv1_sample *sample = NULL) { reset(sample); }

	// sample accessor.
	samplv1_sample *sample() const
		{ return m_sample; }

	// reset.
	void reset(samplv1_sample *sample)
	{
		m_sample = sample;

		m_phase = 0.0f;
		m_index = 0;
		m_alpha = 0.0f;
		m_frame = 0;

		m_loop = false;
		m_loop_phase1 = 0.0f;
		m_loop_phase2 = 0.0f;

		m_phase1 = 0.0f;
		m_index1 = 0;
		m_alpha1 = 0.0f;
		m_xgain1 = 1.0f;
	}

	// reset loop.
	void setLoop(bool loop)
	{
		m_loop = loop;

		if (m_loop) {
			m_loop_phase1 = m_sample->loopPhase1();
			m_loop_phase2 = m_sample->loopPhase2();
		} else {
			m_loop_phase1 = 0.0f;
			m_loop_phase2 = 0.0f;
		}
	}

	// begin.
	void start()
	{
		m_phase  = 0.0f;
		m_index  = 0;
		m_alpha  = 0.0f;
		m_frame  = 0;

		m_phase1 = 0.0f;
		m_index1 = 0;
		m_alpha1 = 0.0f;
		m_xgain1 = 1.0f;

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
			const uint32_t xfade = m_sample->loopCrossFade(); // nframes.
			if (xfade > 0) {
				const float xfade1 = float(xfade); // nframes.
				if (m_phase >= m_loop_phase2 - xfade1) {
					if (//m_sample->isOver(m_index) ||
						m_phase >= m_loop_phase2) {
						m_phase -= m_loop_phase1;
						if (m_phase < 0.0f)
							m_phase = 0.0f;
					}
					if (m_phase1 > 0.0f) {
						m_index1 = int(m_phase1);
						m_alpha1 = m_phase1 - float(m_index1);
						m_phase1 += delta;
						m_xgain1 -= 1.0f / xfade1;
						if (m_xgain1 < 0.0f)
							m_xgain1 = 0.0f;
					} else {
						m_phase1 = m_phase - m_loop_phase1;
						if (m_phase1 < 0.0f)
							m_phase1 = 0.0f;
						m_xgain1 = 1.0f;
					}
				}
				else
				if (m_phase1 > 0.0f) {
					m_phase1 = 0.0f;
					m_index1 = 0;
					m_alpha1 = 0.0f;
					m_xgain1 = 1.0f;
				}
			}
			else
			if (m_phase >= m_loop_phase2) {
				m_phase -= m_loop_phase1;
				if (m_phase < 0.0f)
					m_phase = 0.0f;
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
		float ret = m_xgain1 * interp(k, m_index, m_alpha);
		if (m_index1 > 0)
			ret += (1.0f - m_xgain1) * interp(k, m_index1, m_alpha1);
		return ret;
	}

	// predicate.
	bool isOver() const
		{ return !m_loop && m_sample->isOver(m_frame); }

protected:

	// sample (cubic interpolate).
	float interp(uint16_t k, uint32_t index, float alpha) const
	{
		const float *frames = m_sample->frames(k);

		const float x0 = frames[index];
		const float x1 = frames[index + 1];
		const float x2 = frames[index + 2];
		const float x3 = frames[index + 3];

		const float c1 = (x2 - x0) * 0.5f;
		const float b1 = (x1 - x2);
		const float b2 = (c1 + b1);
		const float c3 = (x3 - x1) * 0.5f + b2 + b1;
		const float c2 = (c3 + b2);

		return (((c3 * alpha) - c2) * alpha + c1) * alpha + x1;
	}

private:

	// iterator variables.
	samplv1_sample *m_sample;

	float    m_phase;
	uint32_t m_index;
	float    m_alpha;
	uint32_t m_frame;

	bool     m_loop;
	float    m_loop_phase1;
	float    m_loop_phase2;

	float    m_phase1;
	uint32_t m_index1;
	float    m_alpha1;
	float    m_xgain1;
};


#endif	// __samplv1_sample_h

// end of samplv1_sample.h
