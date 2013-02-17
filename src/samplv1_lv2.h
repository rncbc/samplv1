// samplv1_lv2.h
//
/****************************************************************************
   Copyright (C) 2012-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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

#define SAMPLV1_LV2_URI "http://samplv1.sourceforge.net/lv2"
#define SAMPLV1_LV2_PREFIX SAMPLV1_LV2_URI "#"


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

	int update_fds(int mode);

	void update_notify();
	void update_reset();

private:

	LV2_URID_Map *m_urid_map;

	uint32_t m_midi_event_type;

	LV2_Atom_Sequence *m_atom_sequence;

	float **m_ins;
	float **m_outs;

	int m_update_fds[2];
	int m_update_count;
};


#endif// __samplv1_lv2_h

// end of samplv1_lv2.h
