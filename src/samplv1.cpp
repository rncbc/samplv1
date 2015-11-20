// samplv1.cpp
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

#include "samplv1.h"

#include "samplv1_sample.h"

#include "samplv1_wave.h"
#include "samplv1_ramp.h"

#include "samplv1_list.h"

#include "samplv1_filter.h"
#include "samplv1_formant.h"

#include "samplv1_fx.h"
#include "samplv1_reverb.h"

#include "samplv1_config.h"
#include "samplv1_controls.h"
#include "samplv1_programs.h"


#ifdef CONFIG_DEBUG_0
#include <stdio.h>
#endif

#include <string.h>


//-------------------------------------------------------------------------
// samplv1_impl
//
// -- borrowed and revamped from synth.h of synth4
//    Copyright (C) 2007 jorgen, linux-vst.com
//

const uint16_t MAX_VOICES = 32;			// polyphony
const uint8_t  MAX_NOTES  = 128;

const float MIN_ENV_MSECS = 2.0f;		// min 2msec per stage
const float MAX_ENV_MSECS = 5000.0f;	// max 5 sec per stage (default)

const float DETUNE_SCALE  = 0.5f;
const float PHASE_SCALE   = 0.5f;
const float OCTAVE_SCALE  = 12.0f;
const float TUNING_SCALE  = 1.0f;
const float SWEEP_SCALE   = 0.5f;
const float PITCH_SCALE   = 0.5f;

const float LFO_FREQ_MIN  = 0.4f;
const float LFO_FREQ_MAX  = 40.0f;


// maximum helper

inline float samplv1_max ( float a, float b )
{
	return (a > b ? a : b);
}


// hyperbolic-tangent fast approximation

inline float samplv1_tanhf ( const float x )
{
	const float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}


// sigmoids

inline float samplv1_sigmoid ( const float x )
{
//	return 2.0f / (1.0f + ::expf(-5.0f * x)) - 1.0f;
	return samplv1_tanhf(2.0f * x);
}

inline float samplv1_sigmoid_0 ( const float x, const float t0 )
{
	const float t1 = 1.0f - t0;
#if 0
	if (x > +t1)
		return +t1 + t0 * samplv1_tanhf(+(x - t1) / t0);
	else
	if (x < -t1)
		return -t1 - t0 * samplv1_tanhf(-(x + t1) / t0);
	else
		return x;
#else
	return (x < -1.0f ? -t1 : (x > +1.0f ? t1 : t1 * x * (1.5f - 0.5f * x * x)));
#endif
}

inline float samplv1_sigmoid_1 ( const float x, const float t0 = 0.01f )
{
	return 0.5f * (1.0f + samplv1_sigmoid_0(2.0f * x - 1.0f, t0));
}


// velocity hard-split curve

inline float samplv1_velocity ( const float x, const float p = 0.2f )
{
	return ::powf(x, (1.0f - p));
}


// pitchbend curve

inline float samplv1_pow2f ( const float x )
{
// simplest power-of-2 straight linearization
// -- x argument valid in [-1, 1] interval
//	return 1.0f + (x < 0.0f ? 0.5f : 1.0f) * x;
	return ::powf(2.0f, x);
}


// convert note to frequency (hertz)

inline float samplv1_freq ( float note )
{
	return (440.0f / 32.0f) * ::powf(2.0f, (note - 9.0f) / 12.0f);
}


// envelope

struct samplv1_env
{
	// envelope stages

	enum Stage { Idle = 0, Attack, Decay, Sustain, Release };

	// per voice

	struct State
	{
		// ctor.
		State() : running(false), stage(Idle),
			phase(0.0f), delta(0.0f), value(0.0f),
			c1(1.0f), c0(0.0f), frames(0) {}

		// process
		float tick()
		{
			if (running && frames > 0) {
				phase += delta;
				value = c1 * phase * (2.0f - phase) + c0;
				--frames;
			}
			return value;
		}

		// state
		bool running;
		Stage stage;
		float phase;
		float delta;
		float value;
		float c1, c0;
		uint32_t frames;
	};

	void start(State *p)
	{
		p->running = true;
		p->stage = Attack;
		p->frames = uint32_t(*attack * *attack * max_frames);
		p->phase = 0.0f;
		if (p->frames > 0) {
			p->delta = 1.0f / float(p->frames);
			p->value = 0.0f;
		} else {
			p->delta = 0.0f;
			p->value = 1.0f;
		}
		p->c1 = 1.0f;
		p->c0 = 0.0f;
	}

	void next(State *p)
	{
		if (p->stage == Attack) {
			p->stage = Decay;
			p->frames = uint32_t(*decay * *decay * max_frames);
			if (p->frames < min_frames) // prevent click on too fast decay
				p->frames = min_frames;
			p->phase = 0.0f;
			p->delta = 1.0f / float(p->frames);
			p->c1 = *sustain - 1.0f;
			p->c0 = p->value;
		}
		else if (p->stage == Decay) {
			p->running = false; // stay at this stage until note_off received
			p->stage = Sustain;
			p->frames = 0;
			p->phase = 0.0f;
			p->delta = 0.0f;
			p->c1 = 0.0f;
			p->c0 = p->value;
		}
		else if (p->stage == Release) {
			p->running = false;
			p->stage = Idle;
			p->frames = 0;
			p->phase = 0.0f;
			p->delta = 0.0f;
			p->value = 0.0f;
			p->c1 = 0.0f;
			p->c0 = 0.0f;
		}
	}

	void note_off(State *p)
	{
		p->running = true;
		p->stage = Release;
		p->frames = uint32_t(*release * *release * max_frames);
		if (p->frames < min_frames) // prevent click on too fast release
			p->frames = min_frames;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	void note_off_fast(State *p)
	{
		p->running = true;
		p->stage = Release;
		p->frames = min_frames;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	// parameters

	float *attack;
	float *decay;
	float *sustain;
	float *release;

	uint32_t min_frames;
	uint32_t max_frames;
};


// midi control

struct samplv1_ctl
{
	samplv1_ctl() { reset(); }

	void reset()
	{
		pressure = 0.0f;
		pitchbend = 1.0f;
		modwheel = 0.0f;
		panning = 0.0f;
		volume = 1.0f;
		sustain = false;
	}

	float pressure;
	float pitchbend;
	float modwheel;
	float panning;
	float volume;
	bool  sustain;
};


// internal control

struct samplv1_aux
{
	samplv1_aux() { reset(); }

	void reset()
	{
		panning = 0.0f;
		volume = 1.0f;
	}

	float panning;
	float volume;
};


// dco

struct samplv1_gen
{
	float *sample, sample0;
	float *reverse;
	float *loop;
	float *octave;
	float *tuning;
	float *glide;
	float *envtime, envtime0;
};


// dcf

struct samplv1_dcf
{
	float *cutoff;
	float *reso;
	float *type;
	float *slope;
	float *envelope;

	samplv1_env env;
};


// lfo

struct samplv1_lfo
{
	float *shape;
	float *width;
	float *rate;
	float *sweep;
	float *pitch;
	float *cutoff;
	float *reso;
	float *panning;
	float *volume;

	samplv1_env env;
};


// dca

struct samplv1_dca
{
	float *volume;

	samplv1_env env;
};



// def (ranges)

struct samplv1_def
{
	float *pitchbend;
	float *modwheel;
	float *pressure;
	float *velocity;
	float *channel;
	float *mono;
};


// out (mix)

struct samplv1_out
{
	float *width;
	float *panning;
	float *fxsend;
	float *volume;
};


// chorus (fx)

struct samplv1_cho
{
	float *wet;
	float *delay;
	float *feedb;
	float *rate;
	float *mod;
};


// flanger (fx)

struct samplv1_fla
{
	float *wet;
	float *delay;
	float *feedb;
	float *daft;
};


// phaser (fx)

struct samplv1_pha
{
	float *wet;
	float *rate;
	float *feedb;
	float *depth;
	float *daft;
};


// delay (fx)

struct samplv1_del
{
	float *wet;
	float *delay;
	float *feedb;
	float *bpm, *bpm0;
	float *bpmsync, bpmsync0;
	float *bpmhost;
};


// reverb

struct samplv1_rev
{
	float *wet;
	float *room;
	float *damp;
	float *feedb;
	float *width;
};


// dynamic(compressor/limiter)

struct samplv1_dyn
{
	float *compress;
	float *limiter;
};


// glide (portamento)

struct samplv1_glide
{
	samplv1_glide(float& last) : m_last(last) { reset(); }

	void reset( uint32_t frames = 0, float freq = 0.0f )
	{
		m_frames = frames;

		if (m_frames > 0) {
			m_freq = m_last - freq;
			m_step = m_freq / float(m_frames);
		} else {
			m_freq = 0.0f;
			m_step = 0.0f;
		}

		m_last = freq;
	}

	float tick()
	{
		if (m_frames > 0) {
			m_freq -= m_step;
			--m_frames;
		}
		return m_freq;
	}

private:

	uint32_t m_frames;

	float m_freq;
	float m_step;

	float& m_last;
};


// panning smoother (3 parameters)

class samplv1_pan : public samplv1_ramp3
{
public:

	samplv1_pan() : samplv1_ramp3(2) {}

protected:

	float evaluate(uint16_t i)
	{
		samplv1_ramp3::update();

		const float wpan = 0.25f * M_PI
			* (1.0f + m_param1_v)
			* (1.0f + m_param2_v)
			* (1.0f + m_param3_v);

		return M_SQRT2 * (i == 0 ? ::cosf(wpan) : ::sinf(wpan));
	}
};


// pressure smoother (3 parameters)

class samplv1_pre : public samplv1_ramp3
{
public:

	samplv1_pre() : samplv1_ramp3() {}

protected:

	float evaluate(uint16_t)
	{
		samplv1_ramp3::update();

		return m_param1_v * samplv1_max(m_param2_v, m_param3_v);
	}
};


// forward decl.

class samplv1_impl;


// voice

struct samplv1_voice : public samplv1_list<samplv1_voice>
{
	samplv1_voice(samplv1_impl *pImpl);

	int note;									// voice note

	float vel;									// key velocity
	float pre;									// key pressure/after-touch

	samplv1_generator  gen1;					// generator
	samplv1_oscillator lfo1;					// low frequency oscilattor

	float gen1_freq;							// frequency and phase

	float lfo1_sample;

	samplv1_filter1 dcf11, dcf12;				// filters
	samplv1_filter2 dcf13, dcf14;
	samplv1_filter3 dcf15, dcf16;
	samplv1_formant dcf17, dcf18;

	samplv1_env::State dca1_env;				// envelope states
	samplv1_env::State dcf1_env;
	samplv1_env::State lfo1_env;

	samplv1_glide gen1_glide;					// glides (portamento)

	samplv1_pre dca1_pre;

	bool sustain;
};


// polyphonic synth implementation

class samplv1_impl
{
public:

	samplv1_impl(samplv1 *pSampl, uint16_t nchannels, float srate);

	~samplv1_impl();

	void setChannels(uint16_t nchannels);
	uint16_t channels() const;

	void setSampleRate(float srate);
	float sampleRate() const;

	void setSampleFile(const char *pszSampleFile);
	const char *sampleFile() const;

	void setBufferSize(uint32_t nsize);
	uint32_t bufferSize() const;

	void setParamPort(samplv1::ParamIndex index, float *pfParam = 0);
	float *paramPort(samplv1::ParamIndex index) const;

	void setParamValue(samplv1::ParamIndex index, float fValue);
	float paramValue(samplv1::ParamIndex index) const;

	samplv1_controls *controls();
	samplv1_programs *programs();

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

	void reset();

	samplv1_sample  gen1_sample;
	samplv1_wave_lf lfo1_wave;

	float gen1_last;

	samplv1_formant::Impl dcf1_formant;

protected:

	void updateEnvTimes();

	void allSoundOff();
	void allControllersOff();
	void allNotesOff();
	void allSustainOff();

	samplv1_voice *alloc_voice ()
	{
		samplv1_voice *pv = m_free_list.next();
		if (pv) {
			m_free_list.remove(pv);
			m_play_list.append(pv);
		}
		return pv;
	}

	void free_voice ( samplv1_voice *pv )
	{
		m_play_list.remove(pv);
		m_free_list.append(pv);
	}

	void alloc_sfxs(uint32_t nsize);

private:

	samplv1_config   m_config;
	samplv1_controls m_controls;
	samplv1_programs m_programs;

	uint16_t m_nchannels;
	float    m_srate;

	samplv1_ctl m_ctl1;

	samplv1_gen m_gen1;
	samplv1_dcf m_dcf1;
	samplv1_lfo m_lfo1;
	samplv1_dca m_dca1;
	samplv1_out m_out1;

	samplv1_def m_def;

	samplv1_cho m_cho;
	samplv1_fla m_fla;
	samplv1_pha m_pha;
	samplv1_del m_del;
	samplv1_rev m_rev;
	samplv1_dyn m_dyn;

	samplv1_voice **m_voices;
	samplv1_voice  *m_notes[MAX_NOTES];

	samplv1_list<samplv1_voice> m_free_list;
	samplv1_list<samplv1_voice> m_play_list;

	samplv1_aux   m_aux1;

	samplv1_ramp1 m_wid1;
	samplv1_pan   m_pan1;
	samplv1_ramp4 m_vol1;

	float  **m_sfxs;
	uint32_t m_nsize;

	samplv1_fx_chorus   m_chorus;
	samplv1_fx_flanger *m_flanger;
	samplv1_fx_phaser  *m_phaser;
	samplv1_fx_delay   *m_delay;
	samplv1_fx_comp    *m_comp;

	samplv1_reverb m_reverb;
};


// voice constructor

samplv1_voice::samplv1_voice ( samplv1_impl *pImpl ) :
	note(-1),
	vel(0.0f),
	pre(0.0f),
	gen1(&pImpl->gen1_sample),
	lfo1(&pImpl->lfo1_wave),
	gen1_freq(0.0f),
	lfo1_sample(0.0f),
	dcf17(&pImpl->dcf1_formant),
	dcf18(&pImpl->dcf1_formant),
	gen1_glide(pImpl->gen1_last),
	sustain(false)
{
}


// engine constructor

samplv1_impl::samplv1_impl (
	samplv1 *pSampl, uint16_t nchannels, float srate )
	: gen1_sample(pSampl), m_controls(pSampl), m_programs(pSampl)
{
	// null sample.
	m_gen1.sample0 = 0.0f;

	// max env. stage length (default)
	m_gen1.envtime0 = 0.0001f * MAX_ENV_MSECS;

	// glide note.
	gen1_last = 0.0f;

	// allocate voice pool.
	m_voices = new samplv1_voice * [MAX_VOICES];

	for (int i = 0; i < MAX_VOICES; ++i) {
		m_voices[i] = new samplv1_voice(this);
		m_free_list.append(m_voices[i]);
	}

	for (int note = 0; note < MAX_NOTES; ++note)
		m_notes[note] = 0;

	// local buffers none yet
	m_sfxs = NULL;
	m_nsize = 0;

	// flangers none yet
	m_flanger = 0;

	// phasers none yet
	m_phaser = 0;

	// delays none yet
	m_delay = 0;

	// compressors none yet
	m_comp = 0;

	// no delay sync yet
	m_del.bpmsync0 = 0.0f;
	m_del.bpm0 = 0;

	// load controllers & programs database...
	m_config.loadControls(&m_controls);
	m_config.loadPrograms(&m_programs);

	// number of channels
	setChannels(nchannels);

	// parameters
	for (int i = 0; i < int(samplv1::NUM_PARAMS); ++i)
		setParamPort(samplv1::ParamIndex(i));

	// set default sample rate
	setSampleRate(srate);

	// reset all voices
	allControllersOff();
	allNotesOff();
}


// destructor

samplv1_impl::~samplv1_impl (void)
{
#if 0
	// DO NOT save programs database here:
	// prevent multi-instance clash...
	m_config.savePrograms(&m_programs);
#endif

	// deallocate sample filenames
	setSampleFile(0);

	// deallocate voice pool.
	for (int i = 0; i < MAX_VOICES; ++i)
		delete m_voices[i];

	delete [] m_voices;

	// deallocate channels
	setChannels(0);

	// deallocate local buffers
	alloc_sfxs(0);
}


void samplv1_impl::setChannels ( uint16_t nchannels )
{
	m_nchannels = nchannels;

	// deallocate flangers
	if (m_flanger) {
		delete [] m_flanger;
		m_flanger = 0;
	}

	// deallocate phasers
	if (m_phaser) {
		delete [] m_phaser;
		m_phaser = 0;
	}

	// deallocate delays
	if (m_delay) {
		delete [] m_delay;
		m_delay = 0;
	}

	// deallocate compressors
	if (m_comp) {
		delete [] m_comp;
		m_comp = 0;
	}
}


uint16_t samplv1_impl::channels (void) const
{
	return m_nchannels;
}


void samplv1_impl::setSampleRate ( float srate )
{
	// set internal sample rate
	m_srate = srate;

	// update waves sample rate
	gen1_sample.setSampleRate(m_srate);
	lfo1_wave.setSampleRate(m_srate);

	updateEnvTimes();

	dcf1_formant.setSampleRate(m_srate);
}


float samplv1_impl::sampleRate (void) const
{
	return m_srate;
}


void samplv1_impl::setBufferSize ( uint32_t nsize )
{
	// set nominal buffer size
	if (m_nsize < nsize) alloc_sfxs(nsize);
}


uint32_t samplv1_impl::bufferSize (void) const
{
	return m_nsize;
}


// allocate local buffers
void samplv1_impl::alloc_sfxs ( uint32_t nsize )
{
	if (m_sfxs) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			delete [] m_sfxs[k];
		delete [] m_sfxs;
		m_sfxs = NULL;
		m_nsize = 0;
	}

	if (m_nsize < nsize) {
		m_nsize = nsize;
		m_sfxs = new float * [m_nchannels];
		for (uint16_t k = 0; k < m_nchannels; ++k)
			m_sfxs[k] = new float [m_nsize];
	}
}


void samplv1_impl::updateEnvTimes (void)
{
	// update envelope range times in frames
	const float srate_ms = 0.001f * m_srate;

	float envtime_msecs = 10000.0f * m_gen1.envtime0;
	if (envtime_msecs < MIN_ENV_MSECS)
		envtime_msecs = (gen1_sample.length() >> 1) / srate_ms;
	if (envtime_msecs < MIN_ENV_MSECS)
		envtime_msecs = MIN_ENV_MSECS + 1.0f;

	const uint32_t min_frames = uint32_t(srate_ms * MIN_ENV_MSECS);
	const uint32_t max_frames = uint32_t(srate_ms * envtime_msecs);

	m_dcf1.env.min_frames = min_frames;
	m_dcf1.env.max_frames = max_frames;

	m_lfo1.env.min_frames = min_frames;
	m_lfo1.env.max_frames = max_frames;

	m_dca1.env.min_frames = min_frames;
	m_dca1.env.max_frames = max_frames;
}


void samplv1_impl::setSampleFile ( const char *pszSampleFile )
{
	reset();

	gen1_sample.close();

	if (pszSampleFile) {
		m_gen1.sample0 = *m_gen1.sample;
		gen1_sample.open(pszSampleFile, samplv1_freq(m_gen1.sample0));
	}
}


const char *samplv1_impl::sampleFile (void) const
{
	return gen1_sample.filename();
}


void samplv1_impl::setParamPort ( samplv1::ParamIndex index, float *pfParam )
{
	static float s_fDummy = 0.0f;

	if (pfParam == 0)
		pfParam = &s_fDummy;

	switch (index) {
	case samplv1::GEN1_SAMPLE:    m_gen1.sample      = pfParam; break;
	case samplv1::GEN1_REVERSE:   m_gen1.reverse     = pfParam; break;
	case samplv1::GEN1_LOOP:      m_gen1.loop        = pfParam; break;
	case samplv1::GEN1_OCTAVE:    m_gen1.octave      = pfParam; break;
	case samplv1::GEN1_TUNING:    m_gen1.tuning      = pfParam; break;
	case samplv1::GEN1_GLIDE:     m_gen1.glide       = pfParam; break;
	case samplv1::GEN1_ENVTIME:   m_gen1.envtime     = pfParam; break;
	case samplv1::DCF1_CUTOFF:    m_dcf1.cutoff      = pfParam; break;
	case samplv1::DCF1_RESO:      m_dcf1.reso        = pfParam; break;
	case samplv1::DCF1_TYPE:      m_dcf1.type        = pfParam; break;
	case samplv1::DCF1_SLOPE:     m_dcf1.slope       = pfParam; break;
	case samplv1::DCF1_ENVELOPE:  m_dcf1.envelope    = pfParam; break;
	case samplv1::DCF1_ATTACK:    m_dcf1.env.attack  = pfParam; break;
	case samplv1::DCF1_DECAY:     m_dcf1.env.decay   = pfParam; break;
	case samplv1::DCF1_SUSTAIN:   m_dcf1.env.sustain = pfParam; break;
	case samplv1::DCF1_RELEASE:   m_dcf1.env.release = pfParam; break;
	case samplv1::LFO1_SHAPE:     m_lfo1.shape       = pfParam; break;
	case samplv1::LFO1_WIDTH:     m_lfo1.width       = pfParam; break;
	case samplv1::LFO1_RATE:      m_lfo1.rate        = pfParam; break;
	case samplv1::LFO1_SWEEP:     m_lfo1.sweep       = pfParam; break;
	case samplv1::LFO1_PITCH:     m_lfo1.pitch       = pfParam; break;
	case samplv1::LFO1_CUTOFF:    m_lfo1.cutoff      = pfParam; break;
	case samplv1::LFO1_RESO:      m_lfo1.reso        = pfParam; break;
	case samplv1::LFO1_PANNING:   m_lfo1.panning     = pfParam; break;
	case samplv1::LFO1_VOLUME:    m_lfo1.volume      = pfParam; break;
	case samplv1::LFO1_ATTACK:    m_lfo1.env.attack  = pfParam; break;
	case samplv1::LFO1_DECAY:     m_lfo1.env.decay   = pfParam; break;
	case samplv1::LFO1_SUSTAIN:   m_lfo1.env.sustain = pfParam; break;
	case samplv1::LFO1_RELEASE:   m_lfo1.env.release = pfParam; break;
	case samplv1::DCA1_VOLUME:    m_dca1.volume      = pfParam; break;
	case samplv1::DCA1_ATTACK:    m_dca1.env.attack  = pfParam; break;
	case samplv1::DCA1_DECAY:     m_dca1.env.decay   = pfParam; break;
	case samplv1::DCA1_SUSTAIN:   m_dca1.env.sustain = pfParam; break;
	case samplv1::DCA1_RELEASE:   m_dca1.env.release = pfParam; break;
	case samplv1::OUT1_WIDTH:     m_out1.width       = pfParam; break;
	case samplv1::OUT1_PANNING:   m_out1.panning     = pfParam; break;
	case samplv1::OUT1_FXSEND:    m_out1.fxsend      = pfParam; break;
	case samplv1::OUT1_VOLUME:    m_out1.volume      = pfParam; break;
	case samplv1::DEF1_PITCHBEND: m_def.pitchbend    = pfParam; break;
	case samplv1::DEF1_MODWHEEL:  m_def.modwheel     = pfParam; break;
	case samplv1::DEF1_PRESSURE:  m_def.pressure     = pfParam; break;
	case samplv1::DEF1_VELOCITY:  m_def.velocity     = pfParam; break;
	case samplv1::DEF1_CHANNEL:   m_def.channel      = pfParam; break;
	case samplv1::DEF1_MONO:      m_def.mono         = pfParam; break;
	case samplv1::CHO1_WET:       m_cho.wet          = pfParam; break;
	case samplv1::CHO1_DELAY:     m_cho.delay        = pfParam; break;
	case samplv1::CHO1_FEEDB:     m_cho.feedb        = pfParam; break;
	case samplv1::CHO1_RATE:      m_cho.rate         = pfParam; break;
	case samplv1::CHO1_MOD:       m_cho.mod          = pfParam; break;
	case samplv1::FLA1_WET:       m_fla.wet          = pfParam; break;
	case samplv1::FLA1_DELAY:     m_fla.delay        = pfParam; break;
	case samplv1::FLA1_FEEDB:     m_fla.feedb        = pfParam; break;
	case samplv1::FLA1_DAFT:      m_fla.daft         = pfParam; break;
	case samplv1::PHA1_WET:       m_pha.wet          = pfParam; break;
	case samplv1::PHA1_RATE:      m_pha.rate         = pfParam; break;
	case samplv1::PHA1_FEEDB:     m_pha.feedb        = pfParam; break;
	case samplv1::PHA1_DEPTH:     m_pha.depth        = pfParam; break;
	case samplv1::PHA1_DAFT:      m_pha.daft         = pfParam; break;
	case samplv1::DEL1_WET:       m_del.wet          = pfParam; break;
	case samplv1::DEL1_DELAY:     m_del.delay        = pfParam; break;
	case samplv1::DEL1_FEEDB:     m_del.feedb        = pfParam; break;
	case samplv1::DEL1_BPM:       m_del.bpm          = pfParam; break;
	case samplv1::DEL1_BPMSYNC:   m_del.bpmsync      = pfParam; break;
	case samplv1::DEL1_BPMHOST:   m_del.bpmhost      = pfParam; break;
	case samplv1::REV1_WET:       m_rev.wet          = pfParam; break;
	case samplv1::REV1_ROOM:      m_rev.room         = pfParam; break;
	case samplv1::REV1_DAMP:      m_rev.damp         = pfParam; break;
	case samplv1::REV1_FEEDB:     m_rev.feedb        = pfParam; break;
	case samplv1::REV1_WIDTH:     m_rev.width        = pfParam; break;
	case samplv1::DYN1_COMPRESS:  m_dyn.compress     = pfParam; break;
	case samplv1::DYN1_LIMITER:   m_dyn.limiter      = pfParam; break;
	default: break;
	}

	// check null connections.
	if (pfParam == &s_fDummy)
		return;

	// reset ramps after port (re)connection.
	switch (index) {
	case samplv1::OUT1_VOLUME:
	case samplv1::DCA1_VOLUME:
		m_vol1.reset(
			m_out1.volume,
			m_dca1.volume,
			&m_ctl1.volume,
			&m_aux1.volume);
		break;
	case samplv1::OUT1_WIDTH:
		m_wid1.reset(
			m_out1.width);
		break;
	case samplv1::OUT1_PANNING:
		m_pan1.reset(
			m_out1.panning,
			&m_ctl1.panning,
			&m_aux1.panning);
		break;
	default:
		break;
	}
}


float *samplv1_impl::paramPort ( samplv1::ParamIndex index ) const
{
	float *pfParam= 0;

	switch (index) {
	case samplv1::GEN1_SAMPLE:    pfParam = m_gen1.sample;      break;
	case samplv1::GEN1_REVERSE:   pfParam = m_gen1.reverse;     break;
	case samplv1::GEN1_LOOP:      pfParam = m_gen1.loop;        break;
	case samplv1::GEN1_OCTAVE:    pfParam = m_gen1.octave;      break;
	case samplv1::GEN1_TUNING:    pfParam = m_gen1.tuning;      break;
	case samplv1::GEN1_GLIDE:     pfParam = m_gen1.glide;       break;
	case samplv1::GEN1_ENVTIME:   pfParam = m_gen1.envtime;     break;
	case samplv1::DCF1_CUTOFF:    pfParam = m_dcf1.cutoff;      break;
	case samplv1::DCF1_RESO:      pfParam = m_dcf1.reso;        break;
	case samplv1::DCF1_TYPE:      pfParam = m_dcf1.type;        break;
	case samplv1::DCF1_SLOPE:     pfParam = m_dcf1.slope;       break;
	case samplv1::DCF1_ENVELOPE:  pfParam = m_dcf1.envelope;    break;
	case samplv1::DCF1_ATTACK:    pfParam = m_dcf1.env.attack;  break;
	case samplv1::DCF1_DECAY:     pfParam = m_dcf1.env.decay;   break;
	case samplv1::DCF1_SUSTAIN:   pfParam = m_dcf1.env.sustain; break;
	case samplv1::DCF1_RELEASE:   pfParam = m_dcf1.env.release; break;
	case samplv1::LFO1_SHAPE:     pfParam = m_lfo1.shape;       break;
	case samplv1::LFO1_WIDTH:     pfParam = m_lfo1.width;       break;
	case samplv1::LFO1_RATE:      pfParam = m_lfo1.rate;        break;
	case samplv1::LFO1_SWEEP:     pfParam = m_lfo1.sweep;       break;
	case samplv1::LFO1_PITCH:     pfParam = m_lfo1.pitch;       break;
	case samplv1::LFO1_CUTOFF:    pfParam = m_lfo1.cutoff;      break;
	case samplv1::LFO1_RESO:      pfParam = m_lfo1.reso;        break;
	case samplv1::LFO1_PANNING:   pfParam = m_lfo1.panning;     break;
	case samplv1::LFO1_VOLUME:    pfParam = m_lfo1.volume;      break;
	case samplv1::LFO1_ATTACK:    pfParam = m_lfo1.env.attack;  break;
	case samplv1::LFO1_DECAY:     pfParam = m_lfo1.env.decay;   break;
	case samplv1::LFO1_SUSTAIN:   pfParam = m_lfo1.env.sustain; break;
	case samplv1::LFO1_RELEASE:   pfParam = m_lfo1.env.release; break;
	case samplv1::DCA1_VOLUME:    pfParam = m_dca1.volume;      break;
	case samplv1::DCA1_ATTACK:    pfParam = m_dca1.env.attack;  break;
	case samplv1::DCA1_DECAY:     pfParam = m_dca1.env.decay;   break;
	case samplv1::DCA1_SUSTAIN:   pfParam = m_dca1.env.sustain; break;
	case samplv1::DCA1_RELEASE:   pfParam = m_dca1.env.release; break;
	case samplv1::OUT1_WIDTH:     pfParam = m_out1.width;       break;
	case samplv1::OUT1_PANNING:   pfParam = m_out1.panning;     break;
	case samplv1::OUT1_FXSEND:    pfParam = m_out1.fxsend;      break;
	case samplv1::OUT1_VOLUME:    pfParam = m_out1.volume;      break;
	case samplv1::DEF1_PITCHBEND: pfParam = m_def.pitchbend;    break;
	case samplv1::DEF1_MODWHEEL:  pfParam = m_def.modwheel;     break;
	case samplv1::DEF1_PRESSURE:  pfParam = m_def.pressure;     break;
	case samplv1::DEF1_VELOCITY:  pfParam = m_def.velocity;     break;
	case samplv1::DEF1_CHANNEL:   pfParam = m_def.channel;      break;
	case samplv1::DEF1_MONO:      pfParam = m_def.mono;         break;
	case samplv1::CHO1_WET:       pfParam = m_cho.wet;          break;
	case samplv1::CHO1_DELAY:     pfParam = m_cho.delay;        break;
	case samplv1::CHO1_FEEDB:     pfParam = m_cho.feedb;        break;
	case samplv1::CHO1_RATE:      pfParam = m_cho.rate;         break;
	case samplv1::CHO1_MOD:       pfParam = m_cho.mod;          break;
	case samplv1::FLA1_WET:       pfParam = m_fla.wet;          break;
	case samplv1::FLA1_DELAY:     pfParam = m_fla.delay;        break;
	case samplv1::FLA1_FEEDB:     pfParam = m_fla.feedb;        break;
	case samplv1::FLA1_DAFT:      pfParam = m_fla.daft;         break;
	case samplv1::PHA1_WET:       pfParam = m_pha.wet;          break;
	case samplv1::PHA1_RATE:      pfParam = m_pha.rate;         break;
	case samplv1::PHA1_FEEDB:     pfParam = m_pha.feedb;        break;
	case samplv1::PHA1_DEPTH:     pfParam = m_pha.depth;        break;
	case samplv1::PHA1_DAFT:      pfParam = m_pha.daft;         break;
	case samplv1::DEL1_WET:       pfParam = m_del.wet;          break;
	case samplv1::DEL1_DELAY:     pfParam = m_del.delay;        break;
	case samplv1::DEL1_FEEDB:     pfParam = m_del.feedb;        break;
	case samplv1::DEL1_BPM:       pfParam = m_del.bpm;          break;
	case samplv1::DEL1_BPMSYNC:   pfParam = m_del.bpmsync;      break;
	case samplv1::DEL1_BPMHOST:   pfParam = m_del.bpmhost;      break;
	case samplv1::REV1_WET:       pfParam = m_rev.wet;          break;
	case samplv1::REV1_ROOM:      pfParam = m_rev.room;         break;
	case samplv1::REV1_DAMP:      pfParam = m_rev.damp;         break;
	case samplv1::REV1_FEEDB:     pfParam = m_rev.feedb;        break;
	case samplv1::REV1_WIDTH:     pfParam = m_rev.width;        break;
	case samplv1::DYN1_COMPRESS:  pfParam = m_dyn.compress;     break;
	case samplv1::DYN1_LIMITER:   pfParam = m_dyn.limiter;      break;
	default: break;
	}

	return pfParam;
}


void samplv1_impl::setParamValue ( samplv1::ParamIndex index, float fValue )
{
	float *pfParamPort = paramPort(index);
	if (pfParamPort)
		*pfParamPort = fValue; // LV2:BUG?
}


float samplv1_impl::paramValue ( samplv1::ParamIndex index ) const
{
	float *pfParamPort = paramPort(index);
	return (pfParamPort ? *pfParamPort : 0.0f);
}


// handle midi input

void samplv1_impl::process_midi ( uint8_t *data, uint32_t size )
{
	for (uint32_t i = 0; i < size; ++i) {

		// channel status
		const int channel = (data[i] & 0x0f) + 1;
		const int status  = (data[i] & 0xf0);

		// channel filter
		const int ch = int(*m_def.channel);
		const int on = (ch == 0 || ch == channel);

		// all system common/real-time ignored
		if (status == 0xf0)
			continue;

		// check data size (#1)
		if (++i >= size)
			break;

		const int key = (data[i] & 0x7f);

		// program change
		if (status == 0xc0) {
			if (on) m_programs.prog_change(key);
			continue;
		}

		// channel aftertouch
		if (status == 0xd0) {
			if (on) m_ctl1.pressure = float(key) / 127.0f;
			continue;
		}

		// check data size (#2)
		if (++i >= size)
			break;

		// channel filter
		if (!on)
			continue;

		const int value = (data[i] & 0x7f);

		// note on
		if (status == 0x90 && value > 0) {
			samplv1_voice *pv;
			// mono voice modes
			if (*m_def.mono > 0.0f) {
				for (pv = m_play_list.next(); pv; pv = pv->next()) {
					if (pv->note >= 0
						&& pv->dca1_env.stage != samplv1_env::Release) {
						m_dcf1.env.note_off_fast(&pv->dcf1_env);
						m_lfo1.env.note_off_fast(&pv->lfo1_env);
						m_dca1.env.note_off_fast(&pv->dca1_env);
						m_notes[pv->note] = 0;
						pv->note = -1;
					}
				}
			}
			pv = m_notes[key];
			if (pv && pv->note >= 0/* && !m_ctl1.sustain*/) {
				// retrigger fast release
				m_dcf1.env.note_off_fast(&pv->dcf1_env);
				m_lfo1.env.note_off_fast(&pv->lfo1_env);
				m_dca1.env.note_off_fast(&pv->dca1_env);
				m_notes[pv->note] = 0;
				pv->note = -1;
			}
			// find free voice
			pv = alloc_voice();
			if (pv) {
				// waveform
				pv->note = key;
				// velocity
				const float vel = float(value) / 127.0f;
				// quadratic velocity law
				pv->vel = samplv1_velocity(vel * vel, *m_def.velocity);
				// pressure/after-touch
				pv->pre = 0.0f;
				pv->dca1_pre.reset(m_def.pressure, &m_ctl1.pressure, &pv->pre);
				// generate
				pv->gen1.start();
				// frequencies
				const float freq1 = float(key)
					+ *m_gen1.octave * OCTAVE_SCALE
					+ *m_gen1.tuning * TUNING_SCALE;
				pv->gen1_freq = samplv1_freq(freq1);
				// filters
				const int type1 = int(*m_dcf1.type);
				pv->dcf11.reset(samplv1_filter1::Type(type1));
				pv->dcf12.reset(samplv1_filter1::Type(type1));
				pv->dcf13.reset(samplv1_filter2::Type(type1));
				pv->dcf14.reset(samplv1_filter2::Type(type1));
				pv->dcf15.reset(samplv1_filter3::Type(type1));
				pv->dcf16.reset(samplv1_filter3::Type(type1));
				pv->dcf17.reset_filters();
				pv->dcf18.reset_filters();
				// envelopes
				m_dcf1.env.start(&pv->dcf1_env);
				m_lfo1.env.start(&pv->lfo1_env);
				m_dca1.env.start(&pv->dca1_env);
				// lfos
				pv->lfo1_sample = pv->lfo1.start();
				// glides (portamentoa)
				const float frames
					= uint32_t(*m_gen1.glide * *m_gen1.glide * m_srate);
				pv->gen1_glide.reset(frames, pv->gen1_freq);
				// sustain
				pv->sustain = false;
				// allocated
				m_notes[key] = pv;
			}
		}
		// note off
		else if (status == 0x80 || (status == 0x90 && value == 0)) {
			samplv1_voice *pv = m_notes[key];
			if (pv && pv->note >= 0) {
				if (m_ctl1.sustain)
					pv->sustain = true;
				else
				if (pv->dca1_env.stage != samplv1_env::Release) {
					m_dca1.env.note_off(&pv->dca1_env);
					m_dcf1.env.note_off(&pv->dcf1_env);
					m_lfo1.env.note_off(&pv->lfo1_env);
					pv->gen1.setLoop(false);
				}
			}
		}
		// key pressure/poly.aftertouch
		else if (status == 0xa0) {
			samplv1_voice *pv = m_notes[key];
			if (pv && pv->note >= 0)
				pv->pre = *m_def.pressure * float(value) / 127.0f;
		}
		// control change
		else if (status == 0xb0) {
		switch (key) {
			case 0x00:
				// bank-select MSB (cc#0)
				m_programs.bank_select_msb(value);
				break;
			case 0x01:
				// modulation wheel (cc#1)
				m_ctl1.modwheel = *m_def.modwheel * float(value) / 127.0f;
				break;
			case 0x07:
				// channel volume (cc#7)
				m_ctl1.volume = float(value) / 127.0f;
				break;
			case 0x0a:
				// channel panning (cc#10)
				m_ctl1.panning = float(value - 64) / 64.0f;
				break;
			case 0x20:
				// bank-select LSB (cc#32)
				m_programs.bank_select_lsb(value);
				break;
			case 0x40:
				// sustain/damper pedal (cc#64)
				if (m_ctl1.sustain && value <  64)
					allSustainOff();
				m_ctl1.sustain = bool(value >= 64);
				break;
			case 0x78:
				// all sound off (cc#120)
				allSoundOff();
				break;
			case 0x79:
				// all controllers off (cc#121)
				allControllersOff();
				break;
			case 0x7b:
				// all notes off (cc#123)
				allNotesOff();
				break;
			}
			// process controllers...
			m_controls.process_enqueue(channel, key, value);
		}
		// pitch bend
		else if (status == 0xe0) {
			const float pitchbend = float(key + (value << 7) - 0x2000) / 8192.0f;
			m_ctl1.pitchbend = samplv1_pow2f(*m_def.pitchbend * pitchbend);
		}

	}

	// process pending controllers...
	m_controls.process_dequeue();
}


// all controllers off

void samplv1_impl::allControllersOff (void)
{
	m_ctl1.reset();
}


// all sound off

void samplv1_impl::allSoundOff (void)
{
	m_chorus.setSampleRate(m_srate);
	m_chorus.reset();

	for (uint16_t k = 0; k < m_nchannels; ++k) {
		m_phaser[k].setSampleRate(m_srate);
		m_delay[k].setSampleRate(m_srate);
		m_comp[k].setSampleRate(m_srate);
		m_flanger[k].reset();
		m_phaser[k].reset();
		m_delay[k].reset();
		m_comp[k].reset();
	}

	m_reverb.setSampleRate(m_srate);
	m_reverb.reset();
}


// all notes off

void samplv1_impl::allNotesOff (void)
{
	samplv1_voice *pv = m_play_list.next();
	while (pv) {
		if (pv->note >= 0)
			m_notes[pv->note] = 0;
		free_voice(pv);
		pv = m_play_list.next();
	}

	gen1_last = 0.0f;

	m_aux1.reset();
}


// all sustained notes off

void samplv1_impl::allSustainOff (void)
{
	samplv1_voice *pv = m_play_list.next();
	while (pv) {
		if (pv->note >= 0 && pv->sustain) {
			pv->sustain = false;
			if (pv->dca1_env.stage != samplv1_env::Release) {
				m_dca1.env.note_off(&pv->dca1_env);
				m_dcf1.env.note_off(&pv->dcf1_env);
				m_lfo1.env.note_off(&pv->lfo1_env);
				pv->gen1.setLoop(false);
			}
		}
		pv = pv->next();
	}
}


// all reset clear

void samplv1_impl::reset (void)
{
#if 0//--legacy support < 0.3.0.4
	if (*m_del.bpm < 3.6f)
		*m_del.bpm *= 100.0f;
#endif

	// make sure dangling states aren't...
	m_del.bpmsync0 = 0.0f;
	m_del.bpm0 = m_del.bpm;

	m_vol1.reset(m_out1.volume, m_dca1.volume, &m_ctl1.volume, &m_aux1.volume);
	m_pan1.reset(m_out1.panning, &m_ctl1.panning, &m_aux1.panning);
	m_wid1.reset(m_out1.width);

	// flangers
	if (m_flanger == 0)
		m_flanger = new samplv1_fx_flanger [m_nchannels];

	// phasers
	if (m_phaser == 0)
		m_phaser = new samplv1_fx_phaser [m_nchannels];

	// delays
	if (m_delay == 0)
		m_delay = new samplv1_fx_delay [m_nchannels];

	// compressors
	if (m_comp == 0)
		m_comp = new samplv1_fx_comp [m_nchannels];

	// reverbs
	m_reverb.reset();

	// controllers reset.
	m_controls.reset();

	allSoundOff();
//	allControllersOff();
	allNotesOff();
}


// controllers accessor

samplv1_controls *samplv1_impl::controls (void)
{
	return &m_controls;
}


// programs accessor

samplv1_programs *samplv1_impl::programs (void)
{
	return &m_programs;
}


// synthesize

void samplv1_impl::process ( float **ins, float **outs, uint32_t nframes )
{
	float *v_outs[m_nchannels];
	float *v_sfxs[m_nchannels];

	// buffer i/o transfer
	if (m_nsize < nframes) alloc_sfxs(nframes);

	uint16_t k;

	for (k = 0; k < m_nchannels; ++k) {
		::memcpy(m_sfxs[k], ins[k], nframes * sizeof(float));
		::memset(outs[k], 0, nframes * sizeof(float));
	}

	// channel indexes

	const uint16_t k11 = 0;
	const uint16_t k12 = (gen1_sample.channels() > 1 ? 1 : 0);

	// controls

	const float lfo1_rate = *m_lfo1.rate * *m_lfo1.rate;

	const float lfo1_freq
		= LFO_FREQ_MIN + lfo1_rate * (LFO_FREQ_MAX - LFO_FREQ_MIN);

	const float modwheel1
		= m_ctl1.modwheel + PITCH_SCALE * *m_lfo1.pitch;

	const float fxsend1 = *m_out1.fxsend * *m_out1.fxsend;

	if (m_gen1.sample0 != *m_gen1.sample) {
		m_gen1.sample0  = *m_gen1.sample;
		gen1_sample.reset(samplv1_freq(m_gen1.sample0));
	}

	if (m_gen1.envtime0 != *m_gen1.envtime) {
		m_gen1.envtime0  = *m_gen1.envtime;
		updateEnvTimes();
	}

	gen1_sample.reverse_test(*m_gen1.reverse > 0.5f);
	gen1_sample.loop_test(*m_gen1.loop > 0.5f);

	lfo1_wave.reset_test(
		samplv1_wave::Shape(*m_lfo1.shape), *m_lfo1.width);

	// per voice

	samplv1_voice *pv = m_play_list.next();

	while (pv) {

		samplv1_voice *pv_next = pv->next();

		// output buffers

		for (k = 0; k < m_nchannels; ++k) {
			v_outs[k] = outs[k];
			v_sfxs[k] = m_sfxs[k];
		}

		uint32_t nblock = nframes;

		while (nblock > 0) {

			uint32_t ngen = nblock;

			// process envelope stages

			if (pv->dca1_env.running && pv->dca1_env.frames < ngen)
				ngen = pv->dca1_env.frames;
			if (pv->dcf1_env.running && pv->dcf1_env.frames < ngen)
				ngen = pv->dcf1_env.frames;
			if (pv->lfo1_env.running && pv->lfo1_env.frames < ngen)
				ngen = pv->lfo1_env.frames;

			for (uint32_t j = 0; j < ngen; ++j) {

				// velocities

				const float vel1
					= (pv->vel + (1.0f - pv->vel) * pv->dca1_pre.value(j));

				// generators

				const float lfo1_env = pv->lfo1_env.tick();
				const float lfo1 = pv->lfo1_sample * lfo1_env;

				pv->gen1.next(pv->gen1_freq
					* (m_ctl1.pitchbend + modwheel1 * lfo1)
					+ pv->gen1_glide.tick());

				float gen1 = pv->gen1.value(k11);
				float gen2 = pv->gen1.value(k12);

				pv->lfo1_sample = pv->lfo1.sample(lfo1_freq
					* (1.0f + SWEEP_SCALE * *m_lfo1.sweep * lfo1_env));

				// filters

				const float env1 = 0.5f * (1.0f + vel1
					* *m_dcf1.envelope * pv->dcf1_env.tick());
				const float cutoff1 = samplv1_sigmoid_1(*m_dcf1.cutoff
					* env1 * (1.0f + *m_lfo1.cutoff * lfo1));
				const float reso1 = samplv1_sigmoid_1(*m_dcf1.reso
					* env1 * (1.0f + *m_lfo1.reso * lfo1));

				switch (int(*m_dcf1.slope)) {
				case 3: // Formant
					gen1 = pv->dcf17.output(gen1, cutoff1, reso1);
					gen2 = pv->dcf18.output(gen2, cutoff1, reso1);
					break;
				case 2: // Biquad
					gen1 = pv->dcf15.output(gen1, cutoff1, reso1);
					gen2 = pv->dcf16.output(gen2, cutoff1, reso1);
					break;
				case 1: // 24db/octave
					gen1 = pv->dcf13.output(gen1, cutoff1, reso1);
					gen2 = pv->dcf14.output(gen2, cutoff1, reso1);
					break;
				case 0: // 12db/octave
				default:
					gen1 = pv->dcf11.output(gen1, cutoff1, reso1);
					gen2 = pv->dcf12.output(gen2, cutoff1, reso1);
					break;
				}

				// volumes

				const float wid1 = m_wid1.value(j);
				const float mid1 = 0.5f * (gen1 + gen2);
				const float sid1 = 0.5f * (gen1 - gen2);
				const float vol1 = vel1 * m_vol1.value(j)
					* pv->dca1_env.tick();

				// outputs

				const float out1
					= vol1 * (mid1 + sid1 * wid1) * m_pan1.value(j, 0);
				const float out2
					= vol1 * (mid1 - sid1 * wid1) * m_pan1.value(j, 1);

				for (k = 0; k < m_nchannels; ++k) {
					const float dry = (k & 1 ? out2 : out1);
					const float wet = fxsend1 * dry;
					*v_outs[k]++ += dry - wet;
					*v_sfxs[k]++ += wet;
				}

				if (j == 0) {
					m_aux1.panning = lfo1 * *m_lfo1.panning;
					m_aux1.volume  = lfo1 * *m_lfo1.volume + 1.0f;
				}
			}

			nblock -= ngen;

			// voice ramps countdown

			pv->dca1_pre.process(ngen);

			// envelope countdowns

			if (pv->dca1_env.running && pv->dca1_env.frames == 0)
				m_dca1.env.next(&pv->dca1_env);

			if (pv->dca1_env.stage == samplv1_env::Idle || pv->gen1.isOver()) {
				if (pv->note >= 0)
					m_notes[pv->note] = 0;
				free_voice(pv);
				nblock = 0;
			} else {
				if (pv->dcf1_env.running && pv->dcf1_env.frames == 0)
					m_dcf1.env.next(&pv->dcf1_env);
				if (pv->lfo1_env.running && pv->lfo1_env.frames == 0)
					m_lfo1.env.next(&pv->lfo1_env);
			}
		}

		// next playing voice

		pv = pv_next;
	}

	// delay sync toggle
	if (int(*m_del.bpmsync) != int(m_del.bpmsync0)) {
		m_del.bpmsync0 = *m_del.bpmsync;
		m_del.bpm0 = (m_del.bpmsync0 > 0.0f ? m_del.bpmhost : m_del.bpm);
	}


	// chorus
	if (m_nchannels > 1) {
		m_chorus.process(m_sfxs[0], m_sfxs[1], nframes, *m_cho.wet,
			*m_cho.delay, *m_cho.feedb, *m_cho.rate, *m_cho.mod);
	}

	// effects
	for (k = 0; k < m_nchannels; ++k) {
		float *in = m_sfxs[k];
		// flanger
		m_flanger[k].process(in, nframes, *m_fla.wet,
			*m_fla.delay, *m_fla.feedb, *m_fla.daft * float(k));
		// phaser
		m_phaser[k].process(in, nframes, *m_pha.wet,
			*m_pha.rate, *m_pha.feedb, *m_pha.depth, *m_pha.daft * float(k));
		// delay
		m_delay[k].process(in, nframes, *m_del.wet,
			*m_del.delay, *m_del.feedb, *m_del.bpm0);
	}

	// reverb
	if (m_nchannels > 1) {
		m_reverb.process(m_sfxs[0], m_sfxs[1], nframes, *m_rev.wet,
			*m_rev.feedb, *m_rev.room, *m_rev.damp, *m_rev.width);
	}

	// output mix-down
	for (k = 0; k < m_nchannels; ++k) {
		uint32_t n;
		// fx sends
		float *in  = m_sfxs[k];
		float *out = outs[k];
		for (n = 0; n < nframes; ++n)
			*out++ += *in++;
		// dynamics
		in = outs[k];
		// compressor
		if (int(*m_dyn.compress) > 0)
			m_comp[k].process(in, nframes);
		// limiter
		if (int(*m_dyn.limiter) > 0) {
			out = in;
			for (n = 0; n < nframes; ++n)
				*out++ = samplv1_sigmoid(*in++);
		}
	}

	// post-processing

	m_wid1.process(nframes);
	m_pan1.process(nframes);
	m_vol1.process(nframes);

	m_controls.process(nframes);
}



//-------------------------------------------------------------------------
// samplv1 - decl.
//

samplv1::samplv1 ( uint16_t nchannels, float srate )
{
	m_pImpl = new samplv1_impl(this, nchannels, srate);
}


samplv1::~samplv1 (void)
{
	delete m_pImpl;
}


void samplv1::setChannels ( uint16_t nchannels )
{
	m_pImpl->setChannels(nchannels);
}


uint16_t samplv1::channels (void) const
{
	return m_pImpl->channels();
}


void samplv1::setSampleRate ( float srate )
{
	m_pImpl->setSampleRate(srate);
}


float samplv1::sampleRate (void) const
{
	return m_pImpl->sampleRate();
}


void samplv1::setSampleFile ( const char *pszSampleFile )
{
	m_pImpl->setSampleFile(pszSampleFile);
}


const char *samplv1::sampleFile (void) const
{
	return m_pImpl->sampleFile();
}


samplv1_sample *samplv1::sample (void) const
{
	return &(m_pImpl->gen1_sample);
}


void samplv1::setReverse ( bool bReverse )
{
	m_pImpl->gen1_sample.setReverse(bReverse);
}

bool samplv1::isReverse (void) const
{
	return m_pImpl->gen1_sample.isReverse();
}


void samplv1::setLoop ( bool bLoop )
{
	m_pImpl->gen1_sample.setLoop(bLoop);
}

bool samplv1::isLoop (void) const
{
	return m_pImpl->gen1_sample.isLoop();
}


void samplv1::setLoopRange ( uint32_t iLoopStart, uint32_t iLoopEnd )
{
	m_pImpl->gen1_sample.setLoopRange(iLoopStart, iLoopEnd);
}

uint32_t samplv1::loopStart (void) const
{
	return m_pImpl->gen1_sample.loopStart();
}

uint32_t samplv1::loopEnd (void) const
{
	return m_pImpl->gen1_sample.loopEnd();
}


void samplv1::setBufferSize ( uint32_t nsize )
{
	m_pImpl->setBufferSize(nsize);
}


uint32_t samplv1::bufferSize (void) const
{
	return m_pImpl->bufferSize();
}


void samplv1::setParamPort ( ParamIndex index, float *pfParam )
{
	m_pImpl->setParamPort(index, pfParam);
}

float *samplv1::paramPort ( ParamIndex index ) const
{
	return m_pImpl->paramPort(index);
}


void samplv1::setParamValue ( ParamIndex index, float fValue )
{
	m_pImpl->setParamValue(index, fValue);
}

float samplv1::paramValue ( ParamIndex index ) const
{
	return m_pImpl->paramValue(index);
}


void samplv1::process_midi ( uint8_t *data, uint32_t size )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "samplv1[%p]::process_midi(%u)", this, size);
	for (uint32_t i = 0; i < size; ++i)
		fprintf(stderr, " %02x", data[i]);
	fprintf(stderr, "\n");
#endif

	m_pImpl->process_midi(data, size);
}


void samplv1::process ( float **ins, float **outs, uint32_t nframes )
{
	m_pImpl->process(ins, outs, nframes);
}


// controllers accessor

samplv1_controls *samplv1::controls (void) const
{
	return m_pImpl->controls();
}


// programs accessor

samplv1_programs *samplv1::programs (void) const
{
	return m_pImpl->programs();
}


// all reset clear

void samplv1::reset (void)
{
	m_pImpl->reset();
}


// end of samplv1.cpp
