// samplv1_lv2.h
//
/****************************************************************************
   Copyright (C) 2012-2016, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __samplv1_lv2_h
#define __samplv1_lv2_h

#include "samplv1.h"

#include "lv2.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#define SAMPLV1_LV2_URI "http://samplv1.sourceforge.net/lv2"
#define SAMPLV1_LV2_PREFIX SAMPLV1_LV2_URI "#"


#ifdef CONFIG_LV2_PROGRAMS
#include "lv2_programs.h"
#include <QByteArray>
#endif


//-------------------------------------------------------------------------
// samplv1_lv2 - decl.
//

class samplv1_lv2 : public samplv1
{
public:

	samplv1_lv2(double sample_rate, const LV2_Feature *const *host_features);

	~samplv1_lv2();

	enum PortIndex {

		MidiIn = 0,
		Notify,
		AudioInL,
		AudioInR,
		AudioOutL,
		AudioOutR,
		ParamBase
	};

	void connect_port(uint32_t port, void *data);

	void run(uint32_t nframes);

	void activate();
	void deactivate();

	uint32_t urid_map(const char *uri) const;

#ifdef CONFIG_LV2_PROGRAMS
	const LV2_Program_Descriptor *get_program(uint32_t index);
	void select_program(uint32_t bank, uint32_t program);
#endif

	bool patch_put(uint32_t ndelta);

	bool worker_work(const void *data, uint32_t size);
	bool worker_response(const void *data, uint32_t size);

private:

	LV2_URID_Map *m_urid_map;

	struct lv2_urids
	{
		LV2_URID gen1_sample;
		LV2_URID gen1_loop_start;
		LV2_URID gen1_loop_end;
		LV2_URID atom_Blank;
		LV2_URID atom_Object;
		LV2_URID atom_Float;
		LV2_URID atom_Int;
		LV2_URID atom_Path;
		LV2_URID time_Position;
		LV2_URID time_beatsPerMinute;
		LV2_URID midi_MidiEvent;
		LV2_URID bufsz_minBlockLength;
		LV2_URID bufsz_maxBlockLength;
		LV2_URID bufsz_nominalBlockLength;
		LV2_URID patch_Get;
		LV2_URID patch_Set;
		LV2_URID patch_Put;
		LV2_URID patch_body;
		LV2_URID patch_property;
		LV2_URID patch_value;

	} m_urids;

	LV2_Atom_Forge m_forge;
	LV2_Atom_Forge_Frame m_notify_frame;

	LV2_Worker_Schedule *m_schedule;

	uint32_t m_ndelta;

	LV2_Atom_Sequence *m_atom_in;
	LV2_Atom_Sequence *m_atom_out;

	float **m_ins;
	float **m_outs;

#ifdef CONFIG_LV2_PROGRAMS
	LV2_Program_Descriptor m_program;
	QByteArray m_aProgramName;
#endif
};


#endif// __samplv1_lv2_h

// end of samplv1_lv2.h
