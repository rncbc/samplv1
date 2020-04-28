// samplv1widget_lv2.cpp
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

#include "samplv1widget_lv2.h"

#include "samplv1_lv2.h"

#include "samplv1widget_palette.h"

#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <QCloseEvent>

#include <QStyleFactory>

#ifndef CONFIG_LIBDIR
#if defined(__x86_64__)
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib64"
#else
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib"
#endif
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt4/plugins"
#else
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt5/plugins"
#endif


//-------------------------------------------------------------------------
// samplv1widget_lv2 - impl.
//

// Constructor.
samplv1widget_lv2::samplv1widget_lv2 ( samplv1_lv2 *pSampl,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: samplv1widget()
{
	// Check whether under a dedicated application instance...
	QApplication *pApp = samplv1_lv2::qapp_instance();
	if (pApp) {
		// Special style paths...
		if (QDir(CONFIG_PLUGINSDIR).exists())
			pApp->addLibraryPath(CONFIG_PLUGINSDIR);
		// Custom color/style themes...
		samplv1_config *pConfig = samplv1_config::getInstance();
		if (pConfig) {
			if (!pConfig->sCustomColorTheme.isEmpty()) {
				QPalette pal;
				if (samplv1widget_palette::namedPalette(
						pConfig, pConfig->sCustomColorTheme, pal))
					pApp->setPalette(pal);
			}
			if (!pConfig->sCustomStyleTheme.isEmpty()) {
				pApp->setStyle(
					QStyleFactory::create(pConfig->sCustomStyleTheme));
			}
		}
	}

	// Initialize (user) interface stuff...
	m_pSamplUi = new samplv1_lv2ui(pSampl, controller, write_function);

#ifdef CONFIG_LV2_UI_EXTERNAL
	m_external_host = nullptr;
#endif
#ifdef CONFIG_LV2_UI_IDLE
	m_bIdleClosed = false;
#endif

	// Initialise preset stuff...
	clearPreset();

	// Initial update, always...
	updateSample(m_pSamplUi->sample());

	//resetParamValues();
	resetParamKnobs();

	// May initialize the scheduler/work notifier.
	openSchedNotifier();
}


// Destructor.
samplv1widget_lv2::~samplv1widget_lv2 (void)
{
	delete m_pSamplUi;
}


// Synth engine accessor.
samplv1_ui *samplv1widget_lv2::ui_instance (void) const
{
	return m_pSamplUi;
}


#ifdef CONFIG_LV2_UI_EXTERNAL

void samplv1widget_lv2::setExternalHost ( LV2_External_UI_Host *external_host )
{
	m_external_host = external_host;
}

const LV2_External_UI_Host *samplv1widget_lv2::externalHost (void) const
{
	return m_external_host;
}

#endif	// CONFIG_LV2_UI_EXTERNAL


#ifdef CONFIG_LV2_UI_IDLE

bool samplv1widget_lv2::isIdleClosed (void) const
{
	return m_bIdleClosed;
}

#endif	// CONFIG_LV2_UI_IDLE


// Close event handler.
void samplv1widget_lv2::closeEvent ( QCloseEvent *pCloseEvent )
{
	samplv1widget::closeEvent(pCloseEvent);

#ifdef CONFIG_LV2_UI_IDLE
	if (pCloseEvent->isAccepted())
		m_bIdleClosed = true;
#endif
#ifdef CONFIG_LV2_UI_EXTERNAL
	if (m_external_host && m_external_host->ui_closed) {
		if (pCloseEvent->isAccepted())
			m_external_host->ui_closed(m_pSamplUi->controller());
	}
#endif
}


// LV2 port event dispatcher.
void samplv1widget_lv2::port_event ( uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	if (format == 0 && buffer_size == sizeof(float)) {
		const samplv1::ParamIndex index
			= samplv1::ParamIndex(port_index - samplv1_lv2::ParamBase);
		const float fValue = *(float *) buffer;
		setParamValue(index, fValue);
	}
}


// Param method.
void samplv1widget_lv2::updateParam (
	samplv1::ParamIndex index, float fValue ) const
{
	m_pSamplUi->write_function(index, fValue);
}


// end of samplv1widget_lv2.cpp
