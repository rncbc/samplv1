// samplv1_wave.h
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

#ifndef __samplv1_wave_h
#define __samplv1_wave_h

#include <stdint.h>


//-------------------------------------------------------------------------
// samplv1_wave - smoothed (integrating oversampled) wave table.
//

class samplv1_wave
{
public:

	// shape.
	enum Shape { Pulse = 0, Saw, Sine, Noise };

	// ctor.
	samplv1_wave(uint32_t nsize = 1024, uint16_t nover = 24);

	// dtor.
	~samplv1_wave();

	// properties.
	Shape shape() const
		{ return m_shape; }
	float width() const
		{ return m_width; }

	// sample rate.
	void setSampleRate(uint32_t iSampleRate)
		{ m_srate = float(iSampleRate); }
	float sampleRate() const
		{ return uint32_t(m_srate); }

	// init.
	void reset(Shape shape = Pulse, float width = 1.0f);

	// begin.
	float start(float& phase, float pshift = 0.0f, float freq = 0.0f) const
	{
		const float p0 = float(m_nsize);

		phase = m_phase0 + pshift * p0;
		if (phase >= p0)
			phase -= p0;

		return sample(phase, freq);
	}

	// iterate.
	float sample(float& phase, float freq) const
	{
		const uint32_t i = uint32_t(phase);
		const float alpha = phase - float(i);
		const float p0 = float(m_nsize);

		phase += p0 * freq / m_srate;
		if (phase >= p0)
			phase -= p0;

		// cubic interpolation...
		const float x0 = m_table[i];
		const float x1 = m_table[i + 1];
	#if 0	// cubic interp.
		const float x2 = m_table[i + 2];
		const float x3 = m_table[i + 3];

		const float c1 = (x2 - x0) * 0.5f;
		const float b1 = (x1 - x2);
		const float b2 = (c1 + b1);
		const float c3 = (x3 - x1) * 0.5f + b2 + b1;
		const float c2 = (c3 + b2);

		return (((c3 * alpha) - c2) * alpha + c1) * alpha + x1;
	#else	// linear interp.
		return x0 + alpha * (x1 - x0);
	#endif
	}

	// absolute value.
	float value(float phase) const
	{
		const float p0 = float(m_nsize);

		phase *= p0;
		phase += m_phase0;
		if (phase >= p0)
			phase -= p0;

		return m_table[uint32_t(phase)];
	}

protected:

	// init pulse table.
	void reset_pulse();

	// init saw table.
	void reset_saw();

	// init sine table.
	void reset_sine();

	// init noise table.
	void reset_noise();

	// post-processors
	void reset_filter();
	void reset_normalize();
	void reset_interp();

private:

	uint32_t m_nsize;
	uint16_t m_nover;

	Shape    m_shape;
	float    m_width;

	float    m_srate;
	float   *m_table;
	float    m_phase0;
};


//-------------------------------------------------------------------------
// samplv1_wave_lf - hard/non-smoothed wave table (eg. LFO).
//

class samplv1_wave_lf : public samplv1_wave
{
public:

	// ctor.
	samplv1_wave_lf(uint32_t nsize = 128)
		: samplv1_wave(nsize, 0) {}
};


//-------------------------------------------------------------------------
// samplv1_oscillator - wave table oscillator
//

class samplv1_oscillator
{
public:

	// ctor.
	samplv1_oscillator(samplv1_wave *wave = 0) { reset(wave); }

	// wave and phase accessors.
	void reset(samplv1_wave *wave)
		{ m_wave = wave; m_phase = 0.0f; }

	samplv1_wave *wave() const
		{ return m_wave; }

	// begin.
	float start(float pshift = 0.0f, float freq = 0.0f)
		{ return m_wave->start(m_phase, pshift, freq); }

	// iterate.
	float sample(float freq)
		{ return m_wave->sample(m_phase, freq); }

private:

	samplv1_wave *m_wave;

	float m_phase;
};


#endif	// __samplv1_wave_h

// end of samplv1_wave.h
