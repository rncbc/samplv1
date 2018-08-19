// samplv1_lv2.cpp
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

#include "samplv1_lv2.h"

#include "samplv1_sched.h"
#include "samplv1_sample.h"

#include "samplv1_programs.h"
#include "samplv1_controls.h"

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"

#ifdef CONFIG_LV2_PATCH
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#endif

#ifndef CONFIG_LV2_ATOM_FORGE_OBJECT
#define lv2_atom_forge_object(forge, frame, id, otype) \
		lv2_atom_forge_blank(forge, frame, id, otype)
#endif

#ifndef CONFIG_LV2_ATOM_FORGE_KEY
#define lv2_atom_forge_key(forge, key) \
		lv2_atom_forge_property_head(forge, key, 0)
#endif

#ifndef LV2_STATE__StateChanged
#define LV2_STATE__StateChanged LV2_STATE_PREFIX "StateChanged"
#endif

#include <stdlib.h>
#include <stdlib.h>
#include <math.h>


//-------------------------------------------------------------------------
// samplv1_lv2 - impl.
//

// atom-like message used internally with worker/schedule
typedef struct {
	LV2_Atom atom;
	const char *sample_path;
} samplv1_lv2_worker_message;


samplv1_lv2::samplv1_lv2 (
	double sample_rate, const LV2_Feature *const *host_features )
	: samplv1(2, float(sample_rate))
{
	::memset(&m_urids, 0, sizeof(m_urids));

	m_urid_map = NULL;
	m_atom_in  = NULL;
	m_atom_out = NULL;
	m_schedule = NULL;
	m_ndelta   = 0;

	const LV2_Options_Option *host_options = NULL;

	for (int i = 0; host_features && host_features[i]; ++i) {
		const LV2_Feature *host_feature = host_features[i];
		if (::strcmp(host_feature->URI, LV2_URID_MAP_URI) == 0) {
			m_urid_map = (LV2_URID_Map *) host_feature->data;
			if (m_urid_map) {
				m_urids.gen1_sample = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_SAMPLE");
				m_urids.gen1_offset_start = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_OFFSET_START");
				m_urids.gen1_offset_end = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_OFFSET_END");
				m_urids.gen1_loop_start = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_LOOP_START");
				m_urids.gen1_loop_end = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_LOOP_END");
				m_urids.gen1_loop_fade = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_LOOP_FADE");
				m_urids.gen1_loop_zero = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_LOOP_ZERO");
				m_urids.gen1_update = m_urid_map->map(
					m_urid_map->handle, SAMPLV1_LV2_PREFIX "GEN1_UPDATE");
				m_urids.atom_Blank = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Blank);
				m_urids.atom_Object = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Object);
				m_urids.atom_Float = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Float);
				m_urids.atom_Int = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Int);
				m_urids.atom_Path = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Path);
				m_urids.time_Position = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__Position);
				m_urids.time_beatsPerMinute = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__beatsPerMinute);
				m_urids.midi_MidiEvent = m_urid_map->map(
					m_urid_map->handle, LV2_MIDI__MidiEvent);
				m_urids.midi_MidiEvent = m_urid_map->map(
					m_urid_map->handle, LV2_MIDI__MidiEvent);
				m_urids.bufsz_minBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__minBlockLength);
				m_urids.bufsz_maxBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__maxBlockLength);
			#ifdef LV2_BUF_SIZE__nominalBlockLength
				m_urids.bufsz_nominalBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__nominalBlockLength);
			#endif
				m_urids.state_StateChanged = m_urid_map->map(
					m_urid_map->handle, LV2_STATE__StateChanged);
			#ifdef CONFIG_LV2_PATCH
				m_urids.patch_Get = m_urid_map->map(
					m_urid_map->handle, LV2_PATCH__Get);
				m_urids.patch_Set = m_urid_map->map(
					m_urid_map->handle, LV2_PATCH__Set);
				m_urids.patch_Put = m_urid_map->map(
					m_urid_map->handle, LV2_PATCH__Put);
				m_urids.patch_body = m_urid_map->map(
					m_urid_map->handle, LV2_PATCH__body);
				m_urids.patch_property = m_urid_map->map(
					m_urid_map->handle, LV2_PATCH__property);
				m_urids.patch_value = m_urid_map->map(
 					m_urid_map->handle, LV2_PATCH__value);
			#endif
			}
		}
		else
		if (::strcmp(host_feature->URI, LV2_WORKER__schedule) == 0)
			m_schedule = (LV2_Worker_Schedule *) host_feature->data;
		else
		if (::strcmp(host_feature->URI, LV2_OPTIONS__options) == 0)
			host_options = (const LV2_Options_Option *) host_feature->data;
	}

	uint32_t buffer_size = 0; // whatever happened to safe default?

	for (int i = 0; host_options && host_options[i].key; ++i) {
		const LV2_Options_Option *host_option = &host_options[i];
		if (host_option->type == m_urids.atom_Int) {
			uint32_t block_length = 0;
			if (host_option->key == m_urids.bufsz_minBlockLength)
				block_length = *(int *) host_option->value;
			else
			if (host_option->key == m_urids.bufsz_maxBlockLength)
				block_length = *(int *) host_option->value;
		#ifdef LV2_BUF_SIZE__nominalBlockLength
			else
			if (host_option->key == m_urids.bufsz_nominalBlockLength)
				block_length = *(int *) host_option->value;
		#endif
			// choose the lengthier...
			if (buffer_size < block_length)
				buffer_size = block_length;
		}
 	}
 
	samplv1::setBufferSize(buffer_size);

	lv2_atom_forge_init(&m_forge, m_urid_map);

	const uint16_t nchannels = samplv1::channels();
	m_ins  = new float * [nchannels];
	m_outs = new float * [nchannels];
	for (uint16_t k = 0; k < nchannels; ++k)
		m_ins[k] = m_outs[k] = NULL;
}


samplv1_lv2::~samplv1_lv2 (void)
{
	delete [] m_outs;
	delete [] m_ins;
}


void samplv1_lv2::connect_port ( uint32_t port, void *data )
{
	switch(PortIndex(port)) {
	case MidiIn:
		m_atom_in = (LV2_Atom_Sequence *) data;
		break;
	case Notify:
		m_atom_out = (LV2_Atom_Sequence *) data;
		break;
	case AudioInL:
		m_ins[0] = (float *) data;
		break;
	case AudioInR:
		m_ins[1] = (float *) data;
		break;
	case AudioOutL:
		m_outs[0] = (float *) data;
		break;
	case AudioOutR:
		m_outs[1] = (float *) data;
		break;
	default:
		samplv1::setParamPort(samplv1::ParamIndex(port - ParamBase), (float *) data);
		break;
	}
}


void samplv1_lv2::run ( uint32_t nframes )
{
	const uint16_t nchannels = samplv1::channels();
	float *ins[nchannels], *outs[nchannels];
	for (uint16_t k = 0; k < nchannels; ++k) {
		ins[k]  = m_ins[k];
		outs[k] = m_outs[k];
	}

	if (m_atom_out) {
		const uint32_t capacity = m_atom_out->atom.size;
		lv2_atom_forge_set_buffer(&m_forge, (uint8_t *) m_atom_out, capacity);
		lv2_atom_forge_sequence_head(&m_forge, &m_notify_frame, 0);
	}

	uint32_t ndelta = 0;

	if (m_atom_in) {
		LV2_ATOM_SEQUENCE_FOREACH(m_atom_in, event) {
			if (event == NULL)
				continue;
			if (event->body.type == m_urids.midi_MidiEvent) {
				uint8_t *data = (uint8_t *) LV2_ATOM_BODY(&event->body);
				if (event->time.frames > ndelta) {
					const uint32_t nread = event->time.frames - ndelta;
					if (nread > 0) {
						samplv1::process(ins, outs, nread);
						for (uint16_t k = 0; k < nchannels; ++k) {
							ins[k]  += nread;
							outs[k] += nread;
						}
					}
				}
				ndelta = event->time.frames;
				samplv1::process_midi(data, event->body.size);
			}
			else
			if (event->body.type == m_urids.atom_Blank ||
				event->body.type == m_urids.atom_Object) {
				const LV2_Atom_Object *object
					= (LV2_Atom_Object *) &event->body;
				if (object->body.otype == m_urids.time_Position) {
					LV2_Atom *atom = NULL;
					lv2_atom_object_get(object,
						m_urids.time_beatsPerMinute, &atom, NULL);
					if (atom && atom->type == m_urids.atom_Float) {
						const float host_bpm = ((LV2_Atom_Float *) atom)->body;
						if (::fabsf(host_bpm - samplv1::tempo()) > 0.001f)
							samplv1::setTempo(host_bpm);
					}
				}
			#ifdef CONFIG_LV2_PATCH
				else 
				if (object->body.otype == m_urids.patch_Set) {
					// set property value
					const LV2_Atom *property = NULL;
					const LV2_Atom *value = NULL;
					lv2_atom_object_get(object,
						m_urids.patch_property, &property,
						m_urids.patch_value, &value, 0);
					if (property && value && property->type == m_forge.URID) {
						const uint32_t key = ((const LV2_Atom_URID *) property)->body;
						const LV2_URID type = value->type;
						if (key == m_urids.gen1_sample
							&& type == m_urids.atom_Path) {
							if (m_schedule) {
								samplv1_lv2_worker_message mesg;
								mesg.atom.type = key;
								mesg.atom.size = sizeof(mesg.sample_path);
								mesg.sample_path
									= (const char *) LV2_ATOM_BODY_CONST(value);
								// schedule loading new sample
								m_schedule->schedule_work(
									m_schedule->handle, sizeof(mesg), &mesg);
							}
						}
						else
						if (key == m_urids.gen1_offset_start
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t offset_start
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								const uint32_t offset_end
									= pSample->offsetEnd();
								setOffsetRange(offset_start, offset_end);
							}
						}
						else
						if (key == m_urids.gen1_offset_end
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t offset_start
									= pSample->offsetStart();
								const uint32_t offset_end
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								setOffsetRange(offset_start, offset_end);
							}
						}
						else
						if (key == m_urids.gen1_loop_start
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t loop_start
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								const uint32_t loop_end
									= pSample->loopEnd();
								setLoopRange(loop_start, loop_end);
							}
						}
						else
						if (key == m_urids.gen1_loop_end
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t loop_start
									= pSample->loopStart();
								const uint32_t loop_end
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								setLoopRange(loop_start, loop_end);
							}
						}
						else
						if (key == m_urids.gen1_loop_fade
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t loop_fade
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								setLoopFade(loop_fade);
							}
						}
						else
						if (key == m_urids.gen1_loop_zero
							&& type == m_urids.atom_Int) {
							samplv1_sample *pSample = samplv1::sample();
							if (pSample) {
								const uint32_t loop_zero
									= *(uint32_t *) LV2_ATOM_BODY_CONST(value);
								setLoopZero(loop_zero > 0);
							}
						}
					}
				}
				else
				if (object->body.otype == m_urids.patch_Get) {
					// put property values (probably to UI)
					patch_put(ndelta);
				}
			#endif	// CONFIG_LV2_PATCH
			}
		}
		// remember last time for worker response
		m_ndelta = ndelta;
	//	m_atom_in = NULL;
	}

	if (nframes > ndelta)
		samplv1::process(ins, outs, nframes - ndelta);

	// test for current sample offset/loop changes
	samplv1::sampleOffsetLoopTest();
}


void samplv1_lv2::activate (void)
{
	samplv1::reset();
}


void samplv1_lv2::deactivate (void)
{
	samplv1::reset();
}


uint32_t samplv1_lv2::urid_map ( const char *uri ) const
{
	return (m_urid_map ? m_urid_map->map(m_urid_map->handle, uri) : 0);
}


//-------------------------------------------------------------------------
// samplv1_lv2 - LV2 State interface.
//

static LV2_State_Status samplv1_lv2_state_save ( LV2_Handle instance,
	LV2_State_Store_Function store, LV2_State_Handle handle,
	uint32_t flags, const LV2_Feature *const *features )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	LV2_State_Map_Path *map_path = NULL;
	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_STATE__mapPath) == 0) {
			map_path = (LV2_State_Map_Path *) features[i]->data;
			break;
		}
	}

	uint32_t key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_SAMPLE");
	if (key == 0)
		return LV2_STATE_ERR_NO_PROPERTY;

	uint32_t type = pPlugin->urid_map(
		map_path ? LV2_ATOM__Path : LV2_ATOM__String);
	if (type == 0)
		return LV2_STATE_ERR_BAD_TYPE;
#if 0
	if (!map_path && (flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) == 0)
		return LV2_STATE_ERR_BAD_FLAGS;
#else
	flags |= (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
#endif
	const char *value = pPlugin->sampleFile();

	if (value && map_path)
		value = (*map_path->abstract_path)(map_path->handle, value);

	if (value == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	size_t size = ::strlen(value) + 1;

	const LV2_State_Status result
		= (*store)(handle, key, value, size, type, flags);

	if (map_path)
		::free((void *) value);

	// Save state properties...
	size = sizeof(uint32_t);
	type = pPlugin->urid_map(LV2_ATOM__Int);
	if (type) {
		// Offset state...
		uint32_t offset_start = pPlugin->offsetStart();
		uint32_t offset_end   = pPlugin->offsetEnd();
		if (offset_start < offset_end) {
			value = (const char *) &offset_start;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_OFFSET_START");
			if (key)
				(*store)(handle, key, value, size, type, flags);
			value = (const char *) &offset_end;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_OFFSET_END");
			if (key)
				(*store)(handle, key, value, size, type, flags);
		}
		// Loop state...
		uint32_t loop_start = pPlugin->loopStart();
		uint32_t loop_end   = pPlugin->loopEnd();
		uint32_t loop_fade  = pPlugin->loopFade();
		uint32_t loop_zero  = pPlugin->isLoopZero() ? 1 : 0;
		if (loop_start < loop_end) {
			value = (const char *) &loop_start;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_START");
			if (key)
				(*store)(handle, key, value, size, type, flags);
			value = (const char *) &loop_end;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_END");
			if (key)
				(*store)(handle, key, value, size, type, flags);
			value = (const char *) &loop_fade;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_FADE");
			if (key)
				(*store)(handle, key, value, size, type, flags);
			value = (const char *) &loop_zero;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_ZERO");
			if (key)
				(*store)(handle, key, value, size, type, flags);
		}
	}

	return result;
}


static LV2_State_Status samplv1_lv2_state_restore ( LV2_Handle instance,
	LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle,
	uint32_t flags, const LV2_Feature *const *features )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	LV2_State_Map_Path *map_path = NULL;
	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_STATE__mapPath) == 0) {
			map_path = (LV2_State_Map_Path *) features[i]->data;
			break;
		}
	}

	uint32_t key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_SAMPLE");
	if (key == 0)
		return LV2_STATE_ERR_NO_PROPERTY;

	uint32_t string_type = pPlugin->urid_map(LV2_ATOM__String);
	if (string_type == 0)
		return LV2_STATE_ERR_BAD_TYPE;

	uint32_t path_type = pPlugin->urid_map(LV2_ATOM__Path);
	if (path_type == 0)
		return LV2_STATE_ERR_BAD_TYPE;

	size_t size = 0;
	uint32_t type = 0;
//	flags = 0;

	const char *value
		= (const char *) (*retrieve)(handle, key, &size, &type, &flags);

	if (size < 2)
		return LV2_STATE_ERR_UNKNOWN;

	if (type != string_type && type != path_type)
		return LV2_STATE_ERR_BAD_TYPE;

	if (!map_path && (flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) == 0)
		return LV2_STATE_ERR_BAD_FLAGS;

	if (value && map_path)
		value = (*map_path->absolute_path)(map_path->handle, value);

	if (value == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	pPlugin->setSampleFile((const char *) value);

	if (map_path)
		::free((void *) value);

	// Restore state properties...
	uint32_t offset_start = 0;
	uint32_t offset_end = 0;
	uint32_t loop_start = 0;
	uint32_t loop_end   = 0;
	uint32_t loop_fade  = 0;
	uint32_t loop_zero  = 1; // default: true.

	const uint32_t int_type = pPlugin->urid_map(LV2_ATOM__Int);
	if (int_type) {
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_OFFSET_START");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				offset_start = *(uint32_t *) value;
		}
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_OFFSET_END");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				offset_end = *(uint32_t *) value;
		}
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_START");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				loop_start = *(uint32_t *) value;
		}
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_END");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				loop_end = *(uint32_t *) value;
		}
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_FADE");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				loop_fade = *(uint32_t *) value;
		}
		key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_ZERO");
		if (key) {
			size = 0;
			type = 0;
			value = (const char *) (*retrieve)(handle, key, &size, &type, &flags);
			if (value && size == sizeof(uint32_t) && type == int_type)
				loop_zero = *(uint32_t *) value;
		}
	}

	pPlugin->setLoopZero(loop_zero > 0);
	pPlugin->setLoopFade(loop_fade);

	if (loop_start < loop_end)
		pPlugin->setLoopRange(loop_start, loop_end);

	if (offset_start < offset_end)
		pPlugin->setOffsetRange(offset_start, offset_end);

	pPlugin->reset();

	samplv1_sched::sync_notify(pPlugin, samplv1_sched::Sample, 1);

	return LV2_STATE_SUCCESS;
}


static const LV2_State_Interface samplv1_lv2_state_interface =
{
	samplv1_lv2_state_save,
	samplv1_lv2_state_restore
};


#ifdef CONFIG_LV2_PROGRAMS

#include "samplv1_programs.h"

const LV2_Program_Descriptor *samplv1_lv2::get_program ( uint32_t index )
{
	samplv1_programs *pPrograms = samplv1::programs();
	const samplv1_programs::Banks& banks = pPrograms->banks();
	samplv1_programs::Banks::ConstIterator bank_iter = banks.constBegin();
	const samplv1_programs::Banks::ConstIterator& bank_end = banks.constEnd();
	for (uint32_t i = 0; bank_iter != bank_end; ++bank_iter) {
		samplv1_programs::Bank *pBank = bank_iter.value();
		const samplv1_programs::Progs& progs = pBank->progs();
		samplv1_programs::Progs::ConstIterator prog_iter = progs.constBegin();
		const samplv1_programs::Progs::ConstIterator& prog_end = progs.constEnd();
		for ( ; prog_iter != prog_end; ++prog_iter, ++i) {
			samplv1_programs::Prog *pProg = prog_iter.value();
			if (i >= index) {
				m_aProgramName = pProg->name().toUtf8();
				m_program.bank = pBank->id();
				m_program.program = pProg->id();
				m_program.name = m_aProgramName.constData();
				return &m_program;
			}
		}
	}

	return NULL;
}

void samplv1_lv2::select_program ( uint32_t bank, uint32_t program )
{
	samplv1::programs()->select_program(bank, program);
}

#endif	// CONFIG_LV2_PROGRAMS


void samplv1_lv2::updatePreset ( bool /*bDirty*/ )
{
	if (m_schedule /*&& bDirty*/) {
		samplv1_lv2_worker_message mesg;
		mesg.atom.type = m_urids.state_StateChanged;
		mesg.atom.size = 0; // nothing else matters.
		mesg.sample_path = NULL;
		m_schedule->schedule_work(
			m_schedule->handle, sizeof(mesg), &mesg);
	}
}


void samplv1_lv2::updateSample (void)
{
	if (m_schedule) {
		samplv1_lv2_worker_message mesg;
		mesg.atom.type = m_urids.gen1_update;
		mesg.atom.size = sizeof(mesg.sample_path);
		mesg.sample_path = samplv1::sampleFile();
		m_schedule->schedule_work(
			m_schedule->handle, sizeof(mesg), &mesg);
	}
}


bool samplv1_lv2::worker_work ( const void *data, uint32_t size )
{
	if (size != sizeof(samplv1_lv2_worker_message))
		return false;

	const samplv1_lv2_worker_message *mesg
		= (const samplv1_lv2_worker_message *) data;

	if (mesg->atom.type == m_urids.state_StateChanged)
		return true;
	else
	if (mesg->atom.type == m_urids.gen1_update)
		return true;
	else
	if (mesg->atom.type == m_urids.gen1_sample) {
		samplv1::setSampleFile(mesg->sample_path);
		return true;
	}

	return false;
}


bool samplv1_lv2::worker_response ( const void *data, uint32_t size )
{
	if (size != sizeof(samplv1_lv2_worker_message))
		return false;

	const samplv1_lv2_worker_message *mesg
		= (const samplv1_lv2_worker_message *) data;
	if (mesg->atom.type == m_urids.state_StateChanged)
		return state_changed();

	// update all properties, and eventually, any observers...
	samplv1_sched::sync_notify(this, samplv1_sched::Sample, 0);

#ifdef CONFIG_LV2_PATCH
	return patch_put(m_ndelta);
#else
	return true;
#endif
}


bool samplv1_lv2::state_changed (void)
{
	lv2_atom_forge_frame_time(&m_forge, m_ndelta);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(&m_forge, &frame, 0, m_urids.state_StateChanged);
	lv2_atom_forge_pop(&m_forge, &frame);

	return true;
}


#ifdef CONFIG_LV2_PATCH

bool samplv1_lv2::patch_put ( uint32_t ndelta )
{
	static char s_szNull[1] = {'\0'};
	const char *pszSampleFile = NULL;
	samplv1_sample *pSample = samplv1::sample();
	if (pSample)
		pszSampleFile = pSample->filename();
	if (pszSampleFile == NULL)
		pszSampleFile = s_szNull;

	lv2_atom_forge_frame_time(&m_forge, ndelta);

	LV2_Atom_Forge_Frame patch_frame;
	lv2_atom_forge_object(&m_forge, &patch_frame, 0, m_urids.patch_Put);
	lv2_atom_forge_key(&m_forge, m_urids.patch_body);

	LV2_Atom_Forge_Frame body_frame;
	lv2_atom_forge_object(&m_forge, &body_frame, 0, 0);
	lv2_atom_forge_key(&m_forge, m_urids.gen1_sample);
	lv2_atom_forge_path(&m_forge, pszSampleFile, ::strlen(pszSampleFile) + 1);
	lv2_atom_forge_key(&m_forge, m_urids.gen1_offset_start);
	lv2_atom_forge_int(&m_forge, pSample->offsetStart());
	lv2_atom_forge_key(&m_forge, m_urids.gen1_offset_end);
	lv2_atom_forge_int(&m_forge, pSample->offsetEnd());
	lv2_atom_forge_key(&m_forge, m_urids.gen1_loop_start);
	lv2_atom_forge_int(&m_forge, pSample->loopStart());
	lv2_atom_forge_key(&m_forge, m_urids.gen1_loop_end);
	lv2_atom_forge_int(&m_forge, pSample->loopEnd());
	lv2_atom_forge_key(&m_forge, m_urids.gen1_loop_fade);
	lv2_atom_forge_int(&m_forge, pSample->loopCrossFade());
	lv2_atom_forge_key(&m_forge, m_urids.gen1_loop_zero);
	lv2_atom_forge_int(&m_forge, pSample->isLoopZeroCrossing() ? 1 : 0);

	lv2_atom_forge_pop(&m_forge, &body_frame);
	lv2_atom_forge_pop(&m_forge, &patch_frame);

	return true;
}

#endif	// CONFIG_LV2_PATCH


//-------------------------------------------------------------------------
// samplv1_lv2 - LV2 desc.
//

static LV2_Handle samplv1_lv2_instantiate (
	const LV2_Descriptor *, double sample_rate, const char *,
	const LV2_Feature *const *host_features )
{
	return new samplv1_lv2(sample_rate, host_features);
}


static void samplv1_lv2_connect_port (
	LV2_Handle instance, uint32_t port, void *data )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->connect_port(port, data);
}


static void samplv1_lv2_run ( LV2_Handle instance, uint32_t nframes )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->run(nframes);
}


static void samplv1_lv2_activate ( LV2_Handle instance )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->activate();
}


static void samplv1_lv2_deactivate ( LV2_Handle instance )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->deactivate();
}


static void samplv1_lv2_cleanup ( LV2_Handle instance )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		delete pPlugin;
}


#ifdef CONFIG_LV2_PROGRAMS

static const LV2_Program_Descriptor *samplv1_lv2_programs_get_program (
	LV2_Handle instance, uint32_t index )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		return pPlugin->get_program(index);
	else
		return NULL;
}

static void samplv1_lv2_programs_select_program (
	LV2_Handle instance, uint32_t bank, uint32_t program )
{
	samplv1_lv2 *pPlugin = static_cast<samplv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->select_program(bank, program);
}

static const LV2_Programs_Interface samplv1_lv2_programs_interface =
{
	samplv1_lv2_programs_get_program,
	samplv1_lv2_programs_select_program,
};

#endif	// CONFIG_LV2_PROGRAMS


static LV2_Worker_Status samplv1_lv2_worker_work (
	LV2_Handle instance, LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle handle, uint32_t size, const void *data )
{
	samplv1_lv2 *pSampl = static_cast<samplv1_lv2 *> (instance);
	if (pSampl && pSampl->worker_work(data, size)) {
		respond(handle, size, data);
		return LV2_WORKER_SUCCESS;
	}

	return LV2_WORKER_ERR_UNKNOWN;
}


static LV2_Worker_Status samplv1_lv2_worker_response (
	LV2_Handle instance, uint32_t size, const void *data )
{
	samplv1_lv2 *pSampl = static_cast<samplv1_lv2 *> (instance);
	if (pSampl && pSampl->worker_response(data, size))
		return LV2_WORKER_SUCCESS;
	else
		return LV2_WORKER_ERR_UNKNOWN;
}


static const LV2_Worker_Interface samplv1_lv2_worker_interface =
{
	samplv1_lv2_worker_work,
	samplv1_lv2_worker_response,
	NULL
};


static const void *samplv1_lv2_extension_data ( const char *uri )
{
#ifdef CONFIG_LV2_PROGRAMS
	if (::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
		return &samplv1_lv2_programs_interface;
	else
#endif
	if (::strcmp(uri, LV2_WORKER__interface) == 0)
		return &samplv1_lv2_worker_interface;
	else
	if (::strcmp(uri, LV2_STATE__interface) == 0)
		return &samplv1_lv2_state_interface;

	return NULL;
}


static const LV2_Descriptor samplv1_lv2_descriptor =
{
	SAMPLV1_LV2_URI,
	samplv1_lv2_instantiate,
	samplv1_lv2_connect_port,
	samplv1_lv2_activate,
	samplv1_lv2_run,
	samplv1_lv2_deactivate,
	samplv1_lv2_cleanup,
	samplv1_lv2_extension_data
};


LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor ( uint32_t index )
{
	return (index == 0 ? &samplv1_lv2_descriptor : NULL);
}


// end of samplv1_lv2.cpp

