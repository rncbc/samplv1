// samplv1_lv2ui.cpp
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

#include "samplv1_lv2ui.h"
#include "samplv1_lv2.h"

#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"

#include <samplv1widget_lv2.h>

#include <QApplication>


//-------------------------------------------------------------------------
// samplv1_lv2ui - impl.
//

samplv1_lv2ui::samplv1_lv2ui ( samplv1_lv2 *pSynth,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: samplv1_ui(pSynth)
{
	m_controller = controller;
	m_write_function = write_function;
}


// Accessors.
const LV2UI_Controller& samplv1_lv2ui::controller (void) const
{
	return m_controller;
}


void samplv1_lv2ui::write_function (
	samplv1::ParamIndex index, float fValue ) const
{
	m_write_function(m_controller,
		samplv1_lv2::ParamBase + index, sizeof(float), 0, &fValue);
}


//-------------------------------------------------------------------------
// samplv1_lv2ui - LV2 UI desc.
//

static QApplication *samplv1_lv2ui_qapp_instance = NULL;
static unsigned int  samplv1_lv2ui_qapp_refcount = 0;


static LV2UI_Handle samplv1_lv2ui_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features )
{
	samplv1_lv2 *pSynth = NULL;

	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0) {
			pSynth = static_cast<samplv1_lv2 *> (features[i]->data);
			break;
		}
	}

	if (pSynth == NULL)
		return NULL;

	if (qApp == NULL && samplv1_lv2ui_qapp_instance == NULL) {
		static int s_argc = 1;
		static const char *s_argv[] = { __func__, NULL };
		samplv1_lv2ui_qapp_instance = new QApplication(s_argc, (char **) s_argv);
	}
	samplv1_lv2ui_qapp_refcount++;

	samplv1widget_lv2 *pWidget
		= new samplv1widget_lv2(pSynth, controller, write_function);
	*widget = pWidget;
	return pWidget;
}

static void samplv1_lv2ui_cleanup ( LV2UI_Handle ui )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget) {
		delete pWidget;
		if (--samplv1_lv2ui_qapp_refcount == 0 && samplv1_lv2ui_qapp_instance) {
			delete samplv1_lv2ui_qapp_instance;
			samplv1_lv2ui_qapp_instance = NULL;
		}
	}
}

static void samplv1_lv2ui_port_event (
	LV2UI_Handle ui, uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget)
		pWidget->port_event(port_index, buffer_size, format, buffer);
}


#ifdef CONFIG_LV2_UI_IDLE

int samplv1_lv2ui_idle ( LV2UI_Handle ui )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if  (pWidget && !pWidget->isIdleClosed()) {
		QApplication::processEvents();
		return 0;
	} else {
		return 1;
	}
}

static const LV2UI_Idle_Interface samplv1_lv2ui_idle_interface =
{
	samplv1_lv2ui_idle
};

#endif	// CONFIG_LV2_UI_IDLE


#ifdef CONFIG_LV2_UI_SHOW

int samplv1_lv2ui_show ( LV2UI_Handle ui )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget) {
		pWidget->show();
		pWidget->raise();
		pWidget->activateWindow();
		return 0;
	} else {
		return 1;
	}
}

int samplv1_lv2ui_hide ( LV2UI_Handle ui )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget) {
		pWidget->hide();
		return 0;
	} else {
		return 1;
	}
}

static const LV2UI_Show_Interface samplv1_lv2ui_show_interface =
{
	samplv1_lv2ui_show,
	samplv1_lv2ui_hide
};

#endif	// CONFIG_LV2_UI_IDLE

static const void *samplv1_lv2ui_extension_data ( const char *uri )
{
#ifdef CONFIG_LV2_UI_IDLE
	if (::strcmp(uri, LV2_UI__idleInterface) == 0)
		return (void *) &samplv1_lv2ui_idle_interface;
	else
#endif
#ifdef CONFIG_LV2_UI_SHOW
	if (::strcmp(uri, LV2_UI__showInterface) == 0)
		return (void *) &samplv1_lv2ui_show_interface;
	else
#endif
	return NULL;
}


#ifdef CONFIG_LV2_EXTERNAL_UI

struct samplv1_lv2ui_external_widget
{
	LV2_External_UI_Widget external;
	samplv1widget_lv2     *widget;
};

static void samplv1_lv2ui_external_run ( LV2_External_UI_Widget *ui_external )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= (samplv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget)
		QApplication::processEvents();
}

static void samplv1_lv2ui_external_show ( LV2_External_UI_Widget *ui_external )
{
	samplv1_lv2ui_external_widget *pExtWidget
		= (samplv1_lv2ui_external_widget *) (ui_external);
	if (pExtWidget) {
		samplv1widget_lv2 *widget = pExtWidget->widget;
		if (widget) {
			widget->show();
			widget->raise();
			widget->activateWindow();
		}
	}
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
	samplv1_lv2 *pSynth = NULL;
	LV2_External_UI_Host *external_host = NULL;

	for (int i = 0; ui_features[i] && !external_host; ++i) {
		if (::strcmp(ui_features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
			pSynth = static_cast<samplv1_lv2 *> (ui_features[i]->data);
		else
		if (::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
			::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0) {
			external_host = (LV2_External_UI_Host *) ui_features[i]->data;
		}
	}

	if (qApp == NULL && samplv1_lv2ui_qapp_instance == NULL) {
		static int s_argc = 1;
		static const char *s_argv[] = { __func__, NULL };
		samplv1_lv2ui_qapp_instance = new QApplication(s_argc, (char **) s_argv);
	}
	samplv1_lv2ui_qapp_refcount++;

	samplv1_lv2ui_external_widget *pExtWidget = new samplv1_lv2ui_external_widget;
	pExtWidget->external.run  = samplv1_lv2ui_external_run;
	pExtWidget->external.show = samplv1_lv2ui_external_show;
	pExtWidget->external.hide = samplv1_lv2ui_external_hide;
	pExtWidget->widget = new samplv1widget_lv2(pSynth, controller, write_function);
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
		delete pExtWidget;
		if (--samplv1_lv2ui_qapp_refcount == 0 && samplv1_lv2ui_qapp_instance) {
			delete samplv1_lv2ui_qapp_instance;
			samplv1_lv2ui_qapp_instance = NULL;
		}
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


// end of samplv1_lv2ui.cpp
