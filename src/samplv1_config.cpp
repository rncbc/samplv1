// samplv1_config.cpp
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

#include "samplv1_config.h"
#include "samplv1_programs.h"

#include <QFileInfo>


//-------------------------------------------------------------------------
// samplv1_config - Prototype settings structure (pseudo-singleton).
//

// Singleton instance accessor (static).
samplv1_config *samplv1_config::g_pSettings = NULL;

samplv1_config *samplv1_config::getInstance (void)
{
	return g_pSettings;
}


// Constructor.
samplv1_config::samplv1_config (void)
	: QSettings(SAMPLV1_DOMAIN, SAMPLV1_TITLE)
{
	g_pSettings = this;

	load();
}


// Default destructor.
samplv1_config::~samplv1_config (void)
{
#if 0
	// DO NOT save config here:
	// prevent multi-instance clash...
	save();
#endif
	g_pSettings = NULL;
}


// Preset utility methods.
QString samplv1_config::presetGroup (void) const
{
	return "/Presets/";
}


QString samplv1_config::presetFile ( const QString& sPreset )
{
	QSettings::beginGroup(presetGroup());
	const QString sPresetFile(QSettings::value(sPreset).toString());
	QSettings::endGroup();
	return sPresetFile;
}


void samplv1_config::setPresetFile (
	const QString& sPreset, const QString& sPresetFile )
{
	QSettings::beginGroup(presetGroup());
	QSettings::setValue(sPreset, sPresetFile);
	QSettings::endGroup();
}


void samplv1_config::removePreset ( const QString& sPreset )
{
	QSettings::beginGroup(presetGroup());
	const QString& sPresetFile = QSettings::value(sPreset).toString();
	if (QFileInfo(sPresetFile).exists())
		QFile(sPresetFile).remove();
	QSettings::remove(sPreset);
	QSettings::endGroup();
}


QStringList samplv1_config::presetList (void)
{
	QStringList list;
	QSettings::beginGroup(presetGroup());
	QStringListIterator iter(QSettings::childKeys());
	while (iter.hasNext()) {
		const QString& sPreset = iter.next();
		if (QFileInfo(QSettings::value(sPreset).toString()).exists())
			list.append(sPreset);
	}
	QSettings::endGroup();
	return list;
}


// Programs utility methods.
QString samplv1_config::programsGroup (void) const
{
	return "/Programs";
}

QString samplv1_config::bankPrefix (void) const
{
	return "/Bank_";
}


void samplv1_config::loadPrograms ( samplv1_programs *pPrograms )
{
	pPrograms->clear_banks();

	QSettings::beginGroup(programsGroup());

	const QStringList& bank_keys = QSettings::childKeys();
	QStringListIterator bank_iter(bank_keys);
	while (bank_iter.hasNext()) {
		const QString& bank_key = bank_iter.next();
		uint16_t bank_id = bank_key.toInt();
		const QString& bank_name
			= QSettings::value(bank_key).toString();
		samplv1_programs::Bank *pBank = pPrograms->add_bank(bank_id, bank_name);
		QSettings::beginGroup(bankPrefix() + bank_key);
		const QStringList& prog_keys = QSettings::childKeys();
		QStringListIterator prog_iter(prog_keys);
		while (prog_iter.hasNext()) {
			const QString& prog_key = prog_iter.next();
			uint16_t prog_id = prog_key.toInt();
			const QString& prog_name
				= QSettings::value(prog_key).toString();
			pBank->add_prog(prog_id, prog_name);
		}
		QSettings::endGroup();
	}

	QSettings::endGroup();
}


void samplv1_config::savePrograms ( samplv1_programs *pPrograms )
{
	clearPrograms();

	QSettings::beginGroup(programsGroup());

	const samplv1_programs::Banks& banks = pPrograms->banks();
	samplv1_programs::Banks::ConstIterator bank_iter = banks.constBegin();
	const samplv1_programs::Banks::ConstIterator& bank_end = banks.constEnd();
	for ( ; bank_iter != bank_end; ++bank_iter) {
		samplv1_programs::Bank *pBank = bank_iter.value();
		const QString& bank_key = QString::number(pBank->id());
		const QString& bank_name = pBank->name();
		QSettings::setValue(bank_key, bank_name);
		QSettings::beginGroup(bankPrefix() + bank_key);
		const samplv1_programs::Progs& progs = pBank->progs();
		samplv1_programs::Progs::ConstIterator prog_iter = progs.constBegin();
		const samplv1_programs::Progs::ConstIterator& prog_end = progs.constEnd();
		for ( ; prog_iter != prog_end; ++prog_iter) {
			samplv1_programs::Prog *pProg = prog_iter.value();
			const QString& prog_key = QString::number(pProg->id());
			const QString& prog_name = pProg->name();
			QSettings::setValue(prog_key, prog_name);
		}
		QSettings::endGroup();
	}

	QSettings::endGroup();
	QSettings::sync();
}


void samplv1_config::clearPrograms (void)
{
	QSettings::beginGroup(programsGroup());

	const QStringList& bank_keys = QSettings::childKeys();
	QStringListIterator bank_iter(bank_keys);
	while (bank_iter.hasNext()) {
		const QString& bank_key = bank_iter.next();
		QSettings::beginGroup(bankPrefix() + bank_key);
		const QStringList& prog_keys = QSettings::childKeys();
		QStringListIterator prog_iter(prog_keys);
		while (prog_iter.hasNext()) {
			const QString& prog_key = prog_iter.next();
			QSettings::remove(prog_key);
		}
		QSettings::endGroup();
		QSettings::remove(bank_key);
	}

	QSettings::endGroup();
}


// Explicit I/O methods.
void samplv1_config::load (void)
{
	QSettings::beginGroup("/Default");
	sPreset = QSettings::value("/Preset").toString();
	sPresetDir = QSettings::value("/PresetDir").toString();
	sSampleDir = QSettings::value("/SampleDir").toString();
	QSettings::endGroup();

	QSettings::beginGroup("/Dialogs");
	bProgramsPreview = QSettings::value("/ProgramsPreview", false).toBool();
	bUseNativeDialogs = QSettings::value("/UseNativeDialogs", true).toBool();
	// Run-time special non-persistent options.
	bDontUseNativeDialogs = !bUseNativeDialogs;
	QSettings::endGroup();
}


void samplv1_config::save (void)
{
	QSettings::beginGroup("/Program");
	QSettings::setValue("/Version", SAMPLV1_VERSION);
	QSettings::endGroup();

	QSettings::beginGroup("/Default");
	QSettings::setValue("/Preset", sPreset);
	QSettings::setValue("/PresetDir", sPresetDir);
	QSettings::setValue("/SampleDir", sSampleDir);
	QSettings::endGroup();

	QSettings::beginGroup("/Dialogs");
	QSettings::setValue("/ProgramsPreview", bProgramsPreview);
	QSettings::setValue("/UseNativeDialogs", bUseNativeDialogs);
	QSettings::endGroup();

	QSettings::sync();
}


// end of samplv1_config.cpp
