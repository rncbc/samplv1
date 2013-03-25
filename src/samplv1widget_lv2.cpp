// samplv1widget_lv2.cpp
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

#include "samplv1widget_lv2.h"

#include "samplv1_lv2.h"

#include <QSocketNotifier>


#ifdef CONFIG_LV2_EXTERNAL_UI
#include <QCloseEvent>
#endif


//-------------------------------------------------------------------------
// samplv1widget_lv2 - impl.
//

// Constructor.
samplv1widget_lv2::samplv1widget_lv2 ( samplv1_lv2 *pSampl,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: samplv1widget(), m_pSampl(pSampl)
{
	m_controller = controller;
	m_write_function = write_function;

	// Update notifier setup.
	m_pUpdateNotifier = new QSocketNotifier(
		m_pSampl->update_fds(1), QSocketNotifier::Read, this);

#ifdef CONFIG_LV2_EXTERNAL_UI
	m_external_host = NULL;
#endif
	
	QObject::connect(m_pUpdateNotifier,
		SIGNAL(activated(int)),
		SLOT(updateNotify()));

	// Initial update, always...
	if (m_pSampl->sampleFile())
		updateSample(m_pSampl->sample());
	else
		initPreset();
}


// Destructor.
samplv1widget_lv2::~samplv1widget_lv2 (void)
{
	delete m_pUpdateNotifier;
}


// Synth engine accessor.
samplv1 *samplv1widget_lv2::instance (void) const
{
	return m_pSampl;
}


#ifdef CONFIG_LV2_EXTERNAL_UI

void samplv1widget_lv2::setExternalHost ( LV2_External_UI_Host *external_host )
{
	m_external_host = external_host;

	if (m_external_host && m_external_host->plugin_human_id)
		samplv1widget::setWindowTitle(m_external_host->plugin_human_id);
}

const LV2_External_UI_Host *samplv1widget_lv2::externalHost (void) const
{
	return m_external_host;
}

void samplv1widget_lv2::closeEvent ( QCloseEvent *pCloseEvent )
{
	samplv1widget::closeEvent(pCloseEvent);

	if (m_external_host && m_external_host->ui_closed) {
		if (pCloseEvent->isAccepted())
			m_external_host->ui_closed(m_controller);
	}
}

#endif	// CONFIG_LV2_EXTERNAL_UI


// Plugin port event notification.
void samplv1widget_lv2::port_event ( uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	if (format == 0 && buffer_size == sizeof(float)) {
		samplv1::ParamIndex index
			= samplv1::ParamIndex(port_index - samplv1_lv2::ParamBase);
		float fValue = *(float *) buffer;
	//--legacy support < 0.3.0.4 -- begin
		if (index == samplv1::DEL1_BPM && fValue < 3.6f)
			fValue *= 100.0f;
	//--legacy support < 0.3.0.4 -- end.
		setParamValue(index, fValue);
	}
}


// Param method.
void samplv1widget_lv2::updateParam (
	samplv1::ParamIndex index, float fValue ) const
{
	m_write_function(m_controller,
		samplv1_lv2::ParamBase + index, sizeof(float), 0, &fValue);
}


// Update notification slot.
void samplv1widget_lv2::updateNotify (void)
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_lv2::updateNotify()");
#endif

	updateSample(m_pSampl->sample());

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1::ParamIndex index = samplv1::ParamIndex(i);
		const float *pfValue = m_pSampl->paramPort(index);
		setParamValue(index, (pfValue ? *pfValue : 0.0f));
	}

	m_pSampl->update_reset();
}


// end of samplv1widget_lv2.cpp
