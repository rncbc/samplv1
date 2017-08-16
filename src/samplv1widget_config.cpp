// samplv1widget_config.cpp
//
/****************************************************************************
   Copyright (C) 2012-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1widget_config.h"
#include "samplv1widget_param.h"

#include "samplv1_ui.h"

#include "samplv1_controls.h"
#include "samplv1_programs.h"


#include <QPushButton>
#include <QMessageBox>

#include <QMenu>

#include <QStyleFactory>


//----------------------------------------------------------------------------
// samplv1widget_config -- UI wrapper form.

// ctor.
samplv1widget_config::samplv1widget_config (
	samplv1_ui *pSamplUi, QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags), m_pSamplUi(pSamplUi)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Custom style themes...
	//m_ui.CustomStyleThemeComboBox->clear();
	//m_ui.CustomStyleThemeComboBox->addItem(tr("(default)"));
	m_ui.CustomStyleThemeComboBox->addItems(QStyleFactory::keys());

	// Setup options...
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig && m_pSamplUi) {
		const bool bPlugin = m_pSamplUi->isPlugin();
		m_ui.ProgramsPreviewCheckBox->setChecked(pConfig->bProgramsPreview);
		m_ui.UseNativeDialogsCheckBox->setChecked(pConfig->bUseNativeDialogs);
		m_ui.KnobDialModeComboBox->setCurrentIndex(pConfig->iKnobDialMode);
		m_ui.KnobEditModeComboBox->setCurrentIndex(pConfig->iKnobEditMode);
		int iCustomStyleTheme = 0;
		if (!pConfig->sCustomStyleTheme.isEmpty())
			iCustomStyleTheme = m_ui.CustomStyleThemeComboBox->findText(
				pConfig->sCustomStyleTheme);
		m_ui.CustomStyleThemeComboBox->setCurrentIndex(iCustomStyleTheme);
		m_ui.CustomStyleThemeTextLabel->setEnabled(!bPlugin);
		m_ui.CustomStyleThemeComboBox->setEnabled(!bPlugin);
		// Load controllers database...
		samplv1_controls *pControls = m_pSamplUi->controls();
		if (pControls) {
			m_ui.ControlsTreeWidget->loadControls(pControls);
			m_ui.ControlsEnabledCheckBox->setEnabled(bPlugin);
			m_ui.ControlsEnabledCheckBox->setChecked(pControls->enabled());
		}
		// Load programs database...
		samplv1_programs *pPrograms = m_pSamplUi->programs();
		if (pPrograms) {
			m_ui.ProgramsTreeWidget->loadPrograms(pPrograms);
			m_ui.ProgramsEnabledCheckBox->setEnabled(bPlugin);
			m_ui.ProgramsPreviewCheckBox->setEnabled(!bPlugin);
			m_ui.ProgramsEnabledCheckBox->setChecked(pPrograms->enabled());
		}
	}

	// Signal/slots connections...
	QObject::connect(m_ui.ControlsAddItemToolButton,
		SIGNAL(clicked()),
		SLOT(controlsAddItem()));
	QObject::connect(m_ui.ControlsEditToolButton,
		SIGNAL(clicked()),
		SLOT(controlsEditItem()));
	QObject::connect(m_ui.ControlsDeleteToolButton,
		SIGNAL(clicked()),
		SLOT(controlsDeleteItem()));

	QObject::connect(m_ui.ControlsTreeWidget,
		SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		SLOT(controlsCurrentChanged()));
	QObject::connect(m_ui.ControlsTreeWidget,
		SIGNAL(itemChanged(QTreeWidgetItem *, int)),
		SLOT(controlsChanged()));

	QObject::connect(m_ui.ControlsEnabledCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(controlsEnabled(bool)));

	QObject::connect(m_ui.ProgramsAddBankToolButton,
		SIGNAL(clicked()),
		SLOT(programsAddBankItem()));
	QObject::connect(m_ui.ProgramsAddItemToolButton,
		SIGNAL(clicked()),
		SLOT(programsAddItem()));
	QObject::connect(m_ui.ProgramsEditToolButton,
		SIGNAL(clicked()),
		SLOT(programsEditItem()));
	QObject::connect(m_ui.ProgramsDeleteToolButton,
		SIGNAL(clicked()),
		SLOT(programsDeleteItem()));

	QObject::connect(m_ui.ProgramsTreeWidget,
		SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		SLOT(programsCurrentChanged()));
	QObject::connect(m_ui.ProgramsTreeWidget,
		SIGNAL(itemChanged(QTreeWidgetItem *, int)),
		SLOT(programsChanged()));
	QObject::connect(m_ui.ProgramsTreeWidget,
		SIGNAL(itemActivated(QTreeWidgetItem *, int)),
		SLOT(programsActivated()));

	QObject::connect(m_ui.ProgramsEnabledCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(programsEnabled(bool)));

	// Custom context menu...
	m_ui.ControlsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.ProgramsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(m_ui.ControlsTreeWidget,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(controlsContextMenuRequested(const QPoint&)));
	QObject::connect(m_ui.ProgramsTreeWidget,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(programsContextMenuRequested(const QPoint&)));

	// Options slots...
	QObject::connect(m_ui.ProgramsPreviewCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.UseNativeDialogsCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.KnobDialModeComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.KnobEditModeComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.CustomStyleThemeComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));

	// Dialog commands...
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(rejected()),
		SLOT(reject()));

	// Dialog dirty flags.
	m_iDirtyControls = 0;
	m_iDirtyPrograms = 0;
	m_iDirtyOptions  = 0;

	// Done.
	stabilize();
}


// dtor.
samplv1widget_config::~samplv1widget_config (void)
{
}


samplv1_ui *samplv1widget_config::ui_instance (void) const
{
	return m_pSamplUi;
}


// controllers command slots.
void samplv1widget_config::controlsAddItem (void)
{
	m_ui.ControlsTreeWidget->addControlItem();

	controlsChanged();
}


void samplv1widget_config::controlsEditItem (void)
{
	QTreeWidgetItem *pItem = m_ui.ControlsTreeWidget->currentItem();
	if (pItem)
		m_ui.ControlsTreeWidget->editItem(pItem, 0);

	controlsChanged();
}


void samplv1widget_config::controlsDeleteItem (void)
{
	QTreeWidgetItem *pItem = m_ui.ControlsTreeWidget->currentItem();
	if (pItem)
		delete pItem;

	controlsChanged();
}


// controllers janitorial slots.
void samplv1widget_config::controlsCurrentChanged (void)
{
	stabilize();
}


void samplv1widget_config::controlsContextMenuRequested ( const QPoint& pos )
{
	QTreeWidgetItem *pItem = m_ui.ControlsTreeWidget->currentItem();

	QMenu menu(this);
	QAction *pAction;

	bool bEnabled = (m_pSamplUi && m_pSamplUi->controls() != NULL);

	pAction = menu.addAction(QIcon(":/images/samplv1_preset.png"),
		tr("&Add Controller"), this, SLOT(controlsAddItem()));
	pAction->setEnabled(bEnabled);

	menu.addSeparator();

	bEnabled = bEnabled && (pItem != NULL);

	pAction = menu.addAction(QIcon(":/images/presetEdit.png"),
		tr("&Edit"), this, SLOT(controlsEditItem()));
	pAction->setEnabled(bEnabled);

	menu.addSeparator();

	pAction = menu.addAction(QIcon(":/images/presetDelete.png"),
		tr("&Delete"), this, SLOT(controlsDeleteItem()));
	pAction->setEnabled(bEnabled);

	menu.exec(m_ui.ControlsTreeWidget->mapToGlobal(pos));
}


void samplv1widget_config::controlsEnabled ( bool bOn )
{
	if (m_pSamplUi) {
		samplv1_controls *pControls = m_pSamplUi->controls();
		if (pControls && m_pSamplUi->isPlugin())
			pControls->enabled(bOn);
	}

	controlsChanged();
}


void samplv1widget_config::controlsChanged (void)
{
	++m_iDirtyControls;

	stabilize();
}


// programs command slots.
void samplv1widget_config::programsAddBankItem (void)
{
	m_ui.ProgramsTreeWidget->addBankItem();

	programsChanged();
}


void samplv1widget_config::programsAddItem (void)
{
	m_ui.ProgramsTreeWidget->addProgramItem();

	programsChanged();
}


void samplv1widget_config::programsEditItem (void)
{
	QTreeWidgetItem *pItem = m_ui.ProgramsTreeWidget->currentItem();
	if (pItem)
		m_ui.ProgramsTreeWidget->editItem(pItem, 1);

	programsChanged();
}


void samplv1widget_config::programsDeleteItem (void)
{
	QTreeWidgetItem *pItem = m_ui.ProgramsTreeWidget->currentItem();
	if (pItem)
		delete pItem;

	programsChanged();
}


// programs janitor slots.
void samplv1widget_config::programsCurrentChanged (void)
{
	stabilize();
}


void samplv1widget_config::programsContextMenuRequested ( const QPoint& pos )
{
	QTreeWidgetItem *pItem = m_ui.ProgramsTreeWidget->currentItem();

	QMenu menu(this);
	QAction *pAction;

	bool bEnabled = (m_pSamplUi && m_pSamplUi->programs() != NULL);

	pAction = menu.addAction(QIcon(":/images/presetBank.png"),
		tr("Add &Bank"), this, SLOT(programsAddBankItem()));
	pAction->setEnabled(bEnabled);

	pAction = menu.addAction(QIcon(":/images/samplv1_preset.png"),
		tr("&Add Program"), this, SLOT(programsAddItem()));
	pAction->setEnabled(bEnabled);

	menu.addSeparator();

	bEnabled = bEnabled && (pItem != NULL);

	pAction = menu.addAction(QIcon(":/images/presetEdit.png"),
		tr("&Edit"), this, SLOT(programsEditItem()));
	pAction->setEnabled(bEnabled);

	menu.addSeparator();

	pAction = menu.addAction(QIcon(":/images/presetDelete.png"),
		tr("&Delete"), this, SLOT(programsDeleteItem()));
	pAction->setEnabled(bEnabled);

	menu.exec(m_ui.ProgramsTreeWidget->mapToGlobal(pos));
}


void samplv1widget_config::programsEnabled ( bool bOn )
{
	if (m_pSamplUi) {
		samplv1_programs *pPrograms = m_pSamplUi->programs();
		if (pPrograms && m_pSamplUi->isPlugin())
			pPrograms->enabled(bOn);
	}

	programsChanged();
}


void samplv1widget_config::programsChanged (void)
{
	++m_iDirtyPrograms;

	stabilize();
}


void samplv1widget_config::programsActivated (void)
{
	if (m_pSamplUi) {
		samplv1_programs *pPrograms = m_pSamplUi->programs();
		if (m_ui.ProgramsPreviewCheckBox->isChecked() && pPrograms)
			m_ui.ProgramsTreeWidget->selectProgram(pPrograms);
	}

	stabilize();
}


// options slot.
void samplv1widget_config::optionsChanged (void)
{
	++m_iDirtyOptions;

	stabilize();
}


// stabilizer.
void samplv1widget_config::stabilize (void)
{
	QTreeWidgetItem *pItem = m_ui.ControlsTreeWidget->currentItem();
	bool bEnabled = (m_pSamplUi && m_pSamplUi->controls() != NULL);
	m_ui.ControlsAddItemToolButton->setEnabled(bEnabled);
	bEnabled = bEnabled && (pItem != NULL);
	m_ui.ControlsEditToolButton->setEnabled(bEnabled);
	m_ui.ControlsDeleteToolButton->setEnabled(bEnabled);

	pItem = m_ui.ProgramsTreeWidget->currentItem();
	bEnabled = (m_pSamplUi && m_pSamplUi->programs() != NULL);
	m_ui.ProgramsPreviewCheckBox->setEnabled(
		bEnabled && m_ui.ProgramsEnabledCheckBox->isChecked());
	m_ui.ProgramsAddBankToolButton->setEnabled(bEnabled);
	m_ui.ProgramsAddItemToolButton->setEnabled(bEnabled);
	bEnabled = bEnabled && (pItem != NULL);
	m_ui.ProgramsEditToolButton->setEnabled(bEnabled);
	m_ui.ProgramsDeleteToolButton->setEnabled(bEnabled);

	const bool bValid
		= (m_iDirtyControls > 0 || m_iDirtyPrograms > 0 || m_iDirtyOptions > 0);
	m_ui.DialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(bValid);
}


// dialog slots.
void samplv1widget_config::accept (void)
{
	samplv1_config *pConfig = samplv1_config::getInstance();

	if (m_iDirtyControls > 0 && pConfig && m_pSamplUi) {
		// Save controls...
		samplv1_controls *pControls = m_pSamplUi->controls();
		if (pControls) {
			m_ui.ControlsTreeWidget->saveControls(pControls);
			pConfig->saveControls(pControls);
			// Reset dirty flag.
			m_iDirtyControls = 0;
		}
	}

	if (m_iDirtyPrograms > 0 && pConfig && m_pSamplUi) {
		// Save programs...
		samplv1_programs *pPrograms = m_pSamplUi->programs();
		if (pPrograms) {
			m_ui.ProgramsTreeWidget->savePrograms(pPrograms);
			pConfig->savePrograms(pPrograms);
			// Reset dirty flag.
			m_iDirtyPrograms = 0;
		}
	}

	if (m_iDirtyOptions > 0 && pConfig) {
		// Save options...
		pConfig->bProgramsPreview = m_ui.ProgramsPreviewCheckBox->isChecked();
		pConfig->bUseNativeDialogs = m_ui.UseNativeDialogsCheckBox->isChecked();
		pConfig->bDontUseNativeDialogs = !pConfig->bUseNativeDialogs;
		pConfig->iKnobDialMode = m_ui.KnobDialModeComboBox->currentIndex();
		samplv1widget_dial::setDialMode(
			samplv1widget_dial::DialMode(pConfig->iKnobDialMode));
		pConfig->iKnobEditMode = m_ui.KnobEditModeComboBox->currentIndex();
		samplv1widget_edit::setEditMode(
			samplv1widget_edit::EditMode(pConfig->iKnobEditMode));
		const QString sOldCustomStyleTheme = pConfig->sCustomStyleTheme;
		if (m_ui.CustomStyleThemeComboBox->currentIndex() > 0)
			pConfig->sCustomStyleTheme = m_ui.CustomStyleThemeComboBox->currentText();
		else
			pConfig->sCustomStyleTheme.clear();
		int iNeedRestart = 0;
 		if (pConfig->sCustomStyleTheme != sOldCustomStyleTheme) {
			if (pConfig->sCustomStyleTheme.isEmpty()) {
				++iNeedRestart;
			} else {
				QApplication::setStyle(
					QStyleFactory::create(pConfig->sCustomStyleTheme));
			}
 		}
		// Show restart message if needed...
 		if (iNeedRestart > 0) {
			QMessageBox::information(this,
				tr("Information") + " - " SAMPLV1_TITLE,
				tr("Some settings may be only effective\n"
				"next time you start this application."));
		}
		// Reset dirty flag.
		m_iDirtyOptions = 0;
	}

	// Just go with dialog acceptance.
	QDialog::accept();
}


void samplv1widget_config::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtyControls > 0 || m_iDirtyPrograms > 0 || m_iDirtyOptions > 0) {
		QMessageBox::StandardButtons buttons
			= QMessageBox::Discard | QMessageBox::Cancel;
		if (m_ui.DialogButtonBox->button(QDialogButtonBox::Ok)->isEnabled())
			buttons |= QMessageBox::Apply;
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " SAMPLV1_TITLE,
			tr("Some settings have been changed.\n\n"
			"Do you want to apply the changes?"),
			buttons)) {
		case QMessageBox::Apply:
			accept();
			return;
		case QMessageBox::Discard:
			break;
		default:    // Cancel.
			bReject = false;
		}
	}

	if (bReject)
		QDialog::reject();
}


// end of samplv1widget_config.cpp
