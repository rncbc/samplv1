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

#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"

#include <QSocketNotifier>

#ifdef CONFIG_LV2_EXTERNAL_UI
#include <QApplication>
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


//-------------------------------------------------------------------------
// samplv1widget_lv2 - LV2 UI desc.
//

static LV2UI_Handle samplv1_lv2ui_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features )
{
	samplv1_lv2 *pSampl = NULL;

	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0) {
			pSampl = static_cast<samplv1_lv2 *> (features[i]->data);
			break;
		}
	}

	if (pSampl == NULL)
		return NULL;

	samplv1widget_lv2 *pWidget
		= new samplv1widget_lv2(pSampl, controller, write_function);
	*widget = pWidget;

	return pWidget;
}

static void samplv1_lv2ui_cleanup ( LV2UI_Handle ui )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget)
		delete pWidget;
}

static void samplv1_lv2ui_port_event (
	LV2UI_Handle ui, uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget)
		pWidget->port_event(port_index, buffer_size, format, buffer);
}

static const void *samplv1_lv2ui_extension_data ( const char * )
{
	return NULL;
}


#ifdef CONFIG_LV2_EXTERNAL_UI

struct samplv1_lv2ui_external_widget
{
	LV2_External_UI_Widget external;
	static QApplication   *app_instance;
	static unsigned int    app_refcount;
	samplv1widget_lv2     *widget;
};

QApplication *samplv1_lv2ui_external_widget::app_instance = NULL;
unsigned int  samplv1_lv2ui_external_widget::app_refcount = 0;


static void samplv1_lv2ui_external_run ( LV2_External_UI_Widget *ui_external )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= (samplv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget && pExtWidget->app_instance)
		pExtWidget->app_instance->processEvents();
}

static void samplv1_lv2ui_external_show ( LV2_External_UI_Widget *ui_external )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= (samplv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget && pExtWidget->widget)
		pExtWidget->widget->show();
}

static void samplv1_lv2ui_external_hide ( LV2_External_UI_Widget *ui_external )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= (samplv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget && pExtWidget->widget)
		pExtWidget->widget->hide();
}

static LV2UI_Handle samplv1_lv2ui_external_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *ui_features )
{
	samplv1_lv2 *pSampl = NULL;
	LV2_External_UI_Host *external_host = NULL;

	for (int i = 0; ui_features && ui_features[i]; ++i) {
		if (::strcmp(ui_features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
			pSampl = static_cast<samplv1_lv2 *> (ui_features[i]->data);
		else
		if (::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
			::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
			external_host = (LV2_External_UI_Host *) ui_features[i]->data;
	}

	if (pSampl == NULL)
		return NULL;

	samplv1_lv2ui_external_widget *pExtWidget = new samplv1_lv2ui_external_widget;
	if (qApp == NULL && pExtWidget->app_instance == NULL) {
		static int s_argc = 1;
		static const char *s_argv[] = { __func__, NULL };
		pExtWidget->app_instance = new QApplication(s_argc, (char **) s_argv);
	}
	pExtWidget->app_refcount++;

	pExtWidget->external.run  = samplv1_lv2ui_external_run;
	pExtWidget->external.show = samplv1_lv2ui_external_show;
	pExtWidget->external.hide = samplv1_lv2ui_external_hide;
	pExtWidget->widget = new samplv1widget_lv2(pSampl, controller, write_function);
	if (external_host)
		pExtWidget->widget->setExternalHost(external_host);
	*widget = pExtWidget;
	return pExtWidget;
}

static void samplv1_lv2ui_external_cleanup ( LV2UI_Handle ui )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= static_cast<samplv1_lv2ui_external_widget *> (ui);
	if (pExtWidget) {
		if (pExtWidget->widget)
			delete pExtWidget->widget;
		if (--pExtWidget->app_refcount == 0 && pExtWidget->app_instance) {
			delete pExtWidget->app_instance;
			pExtWidget->app_instance = NULL;
		}
		delete pExtWidget;
	}
}

static void samplv1_lv2ui_external_port_event (
	LV2UI_Handle ui, uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= static_cast<samplv1_lv2ui_external_widget *> (ui);
	if (pExtWidget && pExtWidget->widget)
		pExtWidget->widget->port_event(port_index, buffer_size, format, buffer);
}

static const void *samplv1_lv2ui_external_extension_data ( const char * )
{
	return NULL;
}

#endif	// CONFIG_LV2_EXTERNAL_UI


static const LV2UI_Descriptor samplv1_lv2ui_descriptor =
{
	SAMPLV1_LV2UI_URI,
	samplv1_lv2ui_instantiate,
	samplv1_lv2ui_cleanup,
	samplv1_lv2ui_port_event,
	samplv1_lv2ui_extension_data
};

#ifdef CONFIG_LV2_EXTERNAL_UI
static const LV2UI_Descriptor samplv1_lv2ui_external_descriptor =
{
	SAMPLV1_LV2UI_EXTERNAL_URI,
	samplv1_lv2ui_external_instantiate,
	samplv1_lv2ui_external_cleanup,
	samplv1_lv2ui_external_port_event,
	samplv1_lv2ui_external_extension_data
};
#endif	// CONFIG_LV2_EXTERNAL_UI


LV2_SYMBOL_EXPORT const LV2UI_Descriptor *lv2ui_descriptor ( uint32_t index )
{
	if (index == 0)
		return &samplv1_lv2ui_descriptor;
	else
#ifdef CONFIG_LV2_EXTERNAL_UI
	if (index == 1)
		return &samplv1_lv2ui_external_descriptor;
	else
#endif
	return NULL;
}


// end of samplv1widget_lv2.cpp
