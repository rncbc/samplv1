// samplv1_lv2ui.cpp
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

#include "samplv1_lv2ui.h"
#include "samplv1_lv2.h"

#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"

#include <samplv1widget_lv2.h>

#include <QApplication>


//-------------------------------------------------------------------------
// samplv1_lv2ui - impl.
//

samplv1_lv2ui::samplv1_lv2ui ( samplv1_lv2 *pSampl,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: samplv1_ui(pSampl, true)
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

static LV2UI_Handle samplv1_lv2ui_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features )
{
	samplv1_lv2 *pSynth = nullptr;

	for (int i = 0; features && features[i]; ++i) {
		if (::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0) {
			pSynth = static_cast<samplv1_lv2 *> (features[i]->data);
			break;
		}
	}

	if (pSynth == nullptr)
		return nullptr;

	samplv1widget_lv2 *pWidget
		= new samplv1widget_lv2(pSynth, controller, write_function);
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


#ifdef CONFIG_LV2_UI_RESIZE

int samplv1_lv2ui_resize ( LV2UI_Handle ui, int width, int height )
{
	samplv1widget_lv2 *pWidget = static_cast<samplv1widget_lv2 *> (ui);
	if (pWidget) {
		pWidget->resize(width, height);
		return 0;
	} else {
		return 1;
	}
}

static const LV2UI_Resize samplv1_lv2ui_resize_interface =
{
	nullptr, // handle: host should use its own when calling ui_resize().
	samplv1_lv2ui_resize
};

#endif	// CONFIG_LV2_UI_RESIZE


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
#ifdef CONFIG_LV2_UI_RESIZE
	if (::strcmp(uri, LV2_UI__resize) == 0)
		return (void *) &samplv1_lv2ui_resize_interface;
	else
#endif
	return nullptr;
}


#ifdef CONFIG_LV2_UI_X11

static LV2UI_Handle samplv1_lv2ui_x11_instantiate (
	const LV2UI_Descriptor *, const char *, const char *,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *ui_features )
{
	WId winid, parent = 0;
	LV2UI_Resize *resize = nullptr;
	samplv1_lv2 *pSampl = nullptr;

	for (int i = 0; ui_features[i]; ++i) {
		if (::strcmp(ui_features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
			pSampl = static_cast<samplv1_lv2 *> (ui_features[i]->data);
		else
		if (::strcmp(ui_features[i]->URI, LV2_UI__parent) == 0)
			parent = (WId) ui_features[i]->data;
		else
		if (::strcmp(ui_features[i]->URI, LV2_UI__resize) == 0)
			resize = (LV2UI_Resize *) ui_features[i]->data;
	}

	if (pSampl == nullptr)
		return nullptr;
	if (!parent)
		return nullptr;

	samplv1widget_lv2 *pWidget
		= new samplv1widget_lv2(pSampl, controller, write_function);
	if (resize && resize->handle) {
		const QSize& hint = pWidget->sizeHint();
		resize->ui_resize(resize->handle, hint.width(), hint.height());
	}
	winid = pWidget->winId();
	pWidget->windowHandle()->setParent(QWindow::fromWinId(parent));
	pWidget->show();
	*widget = (LV2UI_Widget) winid;
	return pWidget;
}

#endif	// CONFIG_LV2_UI_X11


#ifdef CONFIG_LV2_UI_EXTERNAL

struct samplv1_lv2ui_external_widget
{
	LV2_External_UI_Widget external;
	LV2_External_UI_Host  *external_host;
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
			if (pExtWidget->external_host &&
				pExtWidget->external_host->plugin_human_id) {
				widget->setWindowTitle(QString::fromLocal8Bit(
					pExtWidget->external_host->plugin_human_id));
			}
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
	samplv1_lv2 *pSampl = nullptr;
	LV2_External_UI_Host *external_host = nullptr;

	for (int i = 0; ui_features[i] && !external_host; ++i) {
		if (::strcmp(ui_features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
			pSampl = static_cast<samplv1_lv2 *> (ui_features[i]->data);
		else
		if (::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
			::strcmp(ui_features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0) {
			external_host = (LV2_External_UI_Host *) ui_features[i]->data;
		}
	}

	samplv1_lv2ui_external_widget *pExtWidget = new samplv1_lv2ui_external_widget;
	pExtWidget->external.run  = samplv1_lv2ui_external_run;
	pExtWidget->external.show = samplv1_lv2ui_external_show;
	pExtWidget->external.hide = samplv1_lv2ui_external_hide;
	pExtWidget->external_host = external_host;
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
	return nullptr;
}

#endif	// CONFIG_LV2_UI_EXTERNAL


static const LV2UI_Descriptor samplv1_lv2ui_descriptor =
{
	SAMPLV1_LV2UI_URI,
	samplv1_lv2ui_instantiate,
	samplv1_lv2ui_cleanup,
	samplv1_lv2ui_port_event,
	samplv1_lv2ui_extension_data
};

#ifdef CONFIG_LV2_UI_X11
static const LV2UI_Descriptor samplv1_lv2ui_x11_descriptor =
{
	SAMPLV1_LV2UI_X11_URI,
	samplv1_lv2ui_x11_instantiate,
	samplv1_lv2ui_cleanup,
	samplv1_lv2ui_port_event,
	samplv1_lv2ui_extension_data
};
#endif	// CONFIG_LV2_UI_X11

#ifdef CONFIG_LV2_UI_EXTERNAL
static const LV2UI_Descriptor samplv1_lv2ui_external_descriptor =
{
	SAMPLV1_LV2UI_EXTERNAL_URI,
	samplv1_lv2ui_external_instantiate,
	samplv1_lv2ui_external_cleanup,
	samplv1_lv2ui_external_port_event,
	samplv1_lv2ui_external_extension_data
};
#endif	// CONFIG_LV2_UI_EXTERNAL


LV2_SYMBOL_EXPORT const LV2UI_Descriptor *lv2ui_descriptor ( uint32_t index )
{
	if (index == 0)
		return &samplv1_lv2ui_descriptor;
	else
#ifdef CONFIG_LV2_UI_X11
	if (index == 1)
		return &samplv1_lv2ui_x11_descriptor;
	else
#endif
#ifdef CONFIG_LV2_UI_EXTERNAL
	if (index == 2)
		return &samplv1_lv2ui_external_descriptor;
	else
#endif
	return nullptr;
}


// end of samplv1_lv2ui.cpp
