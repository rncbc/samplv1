// samplv1_lv2.cpp
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

#include "samplv1_lv2.h"

#include "samplv1_sched.h"

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include <stdlib.h>
#include <math.h>


//-------------------------------------------------------------------------
// samplv1_lv2 - impl.
//

samplv1_lv2::samplv1_lv2 (
	double sample_rate, const LV2_Feature *const *host_features )
	: samplv1(2, uint32_t(sample_rate))
{
	m_urid_map = NULL;
	m_atom_sequence = NULL;

	for (int i = 0; host_features && host_features[i]; ++i) {
		if (::strcmp(host_features[i]->URI, LV2_URID_MAP_URI) == 0) {
			m_urid_map = (LV2_URID_Map *) host_features[i]->data;
			if (m_urid_map) {
				m_urids.atom_Blank = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Blank);
				m_urids.atom_Object = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Object);
				m_urids.atom_Float = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Float);
				m_urids.time_Position = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__Position);
				m_urids.time_beatsPerMinute = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__beatsPerMinute);
				m_urids.midi_MidiEvent = m_urid_map->map(
					m_urid_map->handle, LV2_MIDI__MidiEvent);
				break;
			}
		}
	}

	const uint16_t nchannels = channels();
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
		m_atom_sequence = (LV2_Atom_Sequence *) data;
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
		setParamPort(ParamIndex(port - ParamBase), (float *) data);
		break;
	}
}


void samplv1_lv2::run ( uint32_t nframes )
{
	const uint16_t nchannels = channels();
	float *ins[nchannels], *outs[nchannels];
	for (uint16_t k = 0; k < nchannels; ++k) {
		ins[k]  = m_ins[k];
		outs[k] = m_outs[k];
	}

	uint32_t ndelta = 0;

	if (m_atom_sequence) {
		LV2_ATOM_SEQUENCE_FOREACH(m_atom_sequence, event) {
			if (event == NULL)
				continue;
			if (event->body.type == m_urids.midi_MidiEvent) {
				uint8_t *data = (uint8_t *) LV2_ATOM_BODY(&event->body);
				const uint32_t nread = event->time.frames - ndelta;
				if (nread > 0) {
					process(ins, outs, nread);
					for (uint16_t k = 0; k < nchannels; ++k) {
						ins[k]  += nread;
						outs[k] += nread;
					}
				}
				ndelta = event->time.frames;
				process_midi(data, event->body.size);
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
						const float bpm_sync = paramValue(samplv1::DEL1_BPMSYNC);
						if (bpm_sync > 0.0f) {
							const float bpm_host = paramValue(samplv1::DEL1_BPMHOST);
							if (bpm_host > 0.0f) {
								const float bpm	= ((LV2_Atom_Float *) atom)->body;
								if (::fabs(bpm_host - bpm) > 0.01f)
									setParamValue(samplv1::DEL1_BPMHOST, bpm);
							}
						}
					}
				}
			}
		}
	//	m_atom_sequence = NULL;
	}

	process(ins, outs, nframes - ndelta);
}


void samplv1_lv2::activate (void)
{
	reset();
}


void samplv1_lv2::deactivate (void)
{
	reset();
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

	LV2_State_Status result = (*store)(handle, key, value, size, type, flags);

	if (map_path)
		::free((void *) value);

	// Save extra loop points...
	uint32_t loop_start = pPlugin->loopStart();
	uint32_t loop_end   = pPlugin->loopEnd();

	if (loop_start < loop_end) {
		type = pPlugin->urid_map(LV2_ATOM__Int);
		if (type) {
			size = sizeof(uint32_t);
			value = (const char *) &loop_start;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_START");
			if (key)
				(*store)(handle, key, value, size, type, flags);
			value = (const char *) &loop_end;
			key = pPlugin->urid_map(SAMPLV1_LV2_PREFIX "GEN1_LOOP_END");
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

	// Restore extra loop points.
	uint32_t loop_start = 0;
	uint32_t loop_end   = 0;

	uint32_t int_type = pPlugin->urid_map(LV2_ATOM__Int);
	if (int_type) {
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
	}

	if (loop_start < loop_end)
		pPlugin->setLoopRange(loop_start, loop_end);

	samplv1_sched::sync_notify(samplv1_sched::Sample);

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

static const void *samplv1_lv2_extension_data ( const char *uri )
{
#ifdef CONFIG_LV2_PROGRAMS
	if (::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
		return (void *) &samplv1_lv2_programs_interface;
	else
#endif
	if (::strcmp(uri, LV2_STATE__interface))
		return NULL;

	return &samplv1_lv2_state_interface;
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
