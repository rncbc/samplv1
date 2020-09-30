// samplv1_sample.h
//
/****************************************************************************
   Copyright (C) 2012-2020, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <math.h>

// forward decls.
class samplv1;


//-------------------------------------------------------------------------
// samplv1_sample - sampler wave table.
//

class samplv1_sample
{
public:

	// ctor.
	samplv1_sample(float srate = 44100.0f);

	// dtor.
	~samplv1_sample();

	// nominal sample-rate.
	void setSampleRate(float srate)
		{ m_srate = srate; }
	float sampleRate() const
		{ return m_srate; }

	// reverse mode.
	void setReverse(bool reverse)
	{
		if (( m_reverse && !reverse) ||
			(!m_reverse &&  reverse)) {
			m_reverse = reverse;
			reverse_sync();
		}
	}

	bool isReverse() const
		{ return m_reverse; }

	// offset mode.
	void setOffset(bool offset)
	{
		m_offset = offset;

		updateOffset();
	}

	bool isOffset() const
		{ return m_offset; }

	// offset range.
	void setOffsetRange(uint32_t start, uint32_t end);

	uint32_t offsetStart() const
		{ return m_offset_start; }
	uint32_t offsetEnd() const
		{ return m_offset_end; }

	float offsetPhase0(uint16_t itab) const
		{ return (m_offset && m_offset_phase0 ? m_offset_phase0[itab] : 0.0f); }

	// loop mode.
	void setLoop(bool loop)
	{
		m_loop = loop;

		updateLoop();
	}

	bool isLoop() const
		{ return m_loop; }

	// loop range.
	void setLoopRange(uint32_t start, uint32_t end);

	uint32_t loopStart() const
		{ return m_loop_start; }
	uint32_t loopEnd() const
		{ return m_loop_end; }

	float loopPhase1(uint16_t itab) const
		{ return (m_loop_phase1 ? m_loop_phase1[itab] : 0.0f); }
	float loopPhase2(uint16_t itab) const
		{ return (m_loop_phase2 ? m_loop_phase2[itab] : 0.0f); }

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
	bool open(const char *filename, float freq0 = 1.0f, uint16_t otabs = 0);
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

	// sample table index.
	uint16_t itab(float freq) const
	{
		int ret = int(m_ntabs >> 1) + fast_ilog2f(freq / m_freq0);

		if (ret < 0)
			ret = 0;
		else
		if (ret > m_ntabs)
			ret = m_ntabs;

		return ret;
	}

	// sample table ratio.
	float ftab(uint16_t itab) const
	{
		float ret = 1.0f;
		const uint16_t itab0 = (m_ntabs >> 1);
		if (itab < itab0)
			ret *= float((itab0 - itab) << 1);
		else
		if (itab > itab0)
			ret /= float((itab - itab0) << 1);
		return ret;
	}

	// number of pitch-shifted octaves.
	uint16_t otabs() const
		{ return m_ntabs >> 1; }

	// frame value.
	float *frames(uint16_t itab, uint16_t k) const
		{ return m_pframes[itab][k]; }
	float *frames(uint16_t k) const
		{ return frames(m_ntabs >> 1, k); }

	// predicate.
	bool isOver(uint32_t index) const
		{ return !m_pframes || (index >= m_offset_end2); }

protected:

	// reverse sample buffer.
	void reverse_sync();

	// zero-crossing aliasing .
	uint32_t zero_crossing(uint16_t itab, uint32_t i, int *slope = nullptr) const;
	float zero_crossing_k(uint16_t itab, uint32_t i) const;

	// offset/loop update.
	void updateOffset();
	void updateLoop();

	// fast log10(x)/log10(2) approximation.
	static inline int fast_ilog2f ( float x )
	{
		union { float f; int32_t i; } u; u.f = x;
		return (((u.i & 0x7f800000) >> 23) - 0x7f)
			+ (u.i & 0x007fffff) / float(0x800000);
	}

private:

	// instance variables.
	float    m_srate;
	uint16_t m_ntabs;

	char    *m_filename;
	uint16_t m_nchannels;
	float    m_rate0;
	float    m_freq0;
	float    m_ratio;

	uint32_t m_nframes;
	float ***m_pframes;
	bool     m_reverse;

	bool     m_offset;
	uint32_t m_offset_start;
	uint32_t m_offset_end;
	float   *m_offset_phase0;
	uint32_t m_offset_end2;

	bool     m_loop;
	uint32_t m_loop_start;
	uint32_t m_loop_end;
	float   *m_loop_phase1;
	float   *m_loop_phase2;
	uint32_t m_loop_xfade;
	bool     m_loop_xzero;
};


//-------------------------------------------------------------------------
// samplv1_generator - sampler oscillator (sort of:)

class samplv1_generator
{
public:

	// ctor.
	samplv1_generator(samplv1_sample *sample = nullptr) { reset(sample); }

	// sample accessor.
	samplv1_sample *sample() const
		{ return m_sample; }

	// reset.
	void reset(samplv1_sample *sample)
	{
		m_sample = sample;

		start(m_sample ? m_sample->freq() : 1.0f);
	}

	// reset loop.
	void setLoop(bool loop)
	{
		m_loop = loop;

		if (m_loop && m_sample) {
			m_loop_phase1 = m_sample->loopPhase1(m_itab);
			m_loop_phase2 = m_sample->loopPhase2(m_itab);
		} else {
			m_loop_phase1 = 0.0f;
			m_loop_phase2 = 0.0f;
		}
	}

	// begin.
	void start(float freq)
	{
		m_itab   = (m_sample ? m_sample->itab(freq) : 0);
		m_ftab   = (m_sample ? m_sample->ftab(m_itab) : 1.0f);

		m_phase0 = (m_sample ? m_sample->offsetPhase0(m_itab) : 0.0f);
		m_phase  = m_phase0;
		m_index  = 0;
		m_alpha  = 0.0f;

		m_phase1 = 0.0f;
		m_index1 = 0;
		m_alpha1 = 0.0f;
		m_xgain1 = 1.0f;

		setLoop(m_sample ? m_sample->isLoop() : false);
	}

	// iterate.
	void next(float freq)
	{
		const float ratio = (m_sample ? m_sample->ratio() : 1.0f);
		const float delta = freq * ratio * m_ftab;

		m_index  = uint32_t(m_phase);
		m_alpha  = m_phase - float(m_index);
		m_phase += delta;

		if (m_loop && m_sample) {
			const uint32_t xfade = m_sample->loopCrossFade(); // nframes.
			if (xfade > 0) {
				const float xfade1 = float(xfade); // nframes.
				if (m_phase >= m_loop_phase2 - xfade1) {
					if (//m_sample->isOver(m_index) ||
						m_phase >= m_loop_phase2) {
						m_phase -= m_loop_phase1 * ::ceilf(delta / m_loop_phase1);
						if (m_phase < m_phase0)
							m_phase = m_phase0;
					}
					if (m_phase1 > 0.0f) {
						m_index1 = int(m_phase1);
						m_alpha1 = m_phase1 - float(m_index1);
						m_phase1 += delta;
						m_xgain1 -= delta / xfade1;
						if (m_xgain1 < 0.0f)
							m_xgain1 = 0.0f;
					} else {
						m_phase1 = m_phase - m_loop_phase1;
						if (m_phase1 < m_phase0)
							m_phase1 = m_phase0;
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
				m_phase -= m_loop_phase1 * ::ceilf(delta / m_loop_phase1);
				if (m_phase < m_phase0)
					m_phase = m_phase0;
			}
		}
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
		{ return !m_loop && (m_sample ? m_sample->isOver(m_index) : true); }

protected:

	// sample (cubic interpolate).
	float interp(uint16_t k, uint32_t index, float alpha) const
	{
		const float *frames = m_sample->frames(m_itab, k);

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

	uint16_t m_itab;
	float    m_ftab;

	float    m_phase0;
	float    m_phase;
	uint32_t m_index;
	float    m_alpha;

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
