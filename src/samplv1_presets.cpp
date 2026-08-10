// samplv1_presets.cpp
//
/****************************************************************************
   Copyright (C) 2012-2026, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_presets.h"


//-------------------------------------------------------------------------
// samplv1_presets - Bank/presets database class (singleton).
//

// ctor.
samplv1_presets::samplv1_presets (void)
	: m_current_bank(nullptr), m_current_preset(nullptr)
{
}


// dtor.
samplv1_presets::~samplv1_presets (void)
{
	clear_banks();
	clear_presets();
}


// bank/preset managers
void samplv1_presets::Bank::add_preset ( const QString& preset_name )
{
	if (!m_preset_list.contains(preset_name))
		m_preset_list.append(preset_name);
}


void samplv1_presets::Bank::remove_preset ( const QString& preset_name )
{
	m_preset_list.removeAll(preset_name);
}


void samplv1_presets::Bank::clear_presets (void)
{
	m_preset_list.clear();
}


// bank managers
samplv1_presets::Bank *samplv1_presets::find_bank (
	const QString& bank_name ) const
{
	return m_banks.value(bank_name, nullptr);
}


samplv1_presets::Bank *samplv1_presets::find_preset_bank (
	const QString& preset_name ) const
{
	Banks::ConstIterator banks_iter = m_banks.constBegin();
	const Banks::ConstIterator& banks_end = m_banks.constEnd();
	for ( ; banks_iter != banks_end; ++banks_iter) {
		Bank *bank = banks_iter.value();
		if (bank->preset_list().contains(preset_name))
			return bank;
	}

	return nullptr;
}


samplv1_presets::Bank *samplv1_presets::add_bank (
	const QString& bank_name )
{
	Bank *bank = find_bank(bank_name);
	if (bank == nullptr) {
		bank = new Bank(bank_name);
		m_banks.insert(bank_name, bank);
		m_bank_list.append(bank_name);
	}

	return bank;
}


void samplv1_presets::remove_bank ( const QString& bank_name )
{
	Bank *bank = find_bank(bank_name);
	if (bank && m_current_bank == bank)
		m_current_bank = nullptr;
	if (bank && m_banks.remove(bank_name))
		delete bank;

	m_bank_list.removeAll(bank_name);
}


void samplv1_presets::clear_banks (void)
{
	m_current_bank = nullptr;

	qDeleteAll(m_banks);
	m_banks.clear();

	m_bank_list.clear();
}


// preset managers
samplv1_presets::Preset *samplv1_presets::find_preset (
	const QString& preset_name ) const
{
	return m_presets.value(preset_name, nullptr);
}


samplv1_presets::Preset *samplv1_presets::add_preset (
	const QString& preset_name )
{
	Preset *preset = find_preset(preset_name);
	if (preset == nullptr) {
		preset = new Preset(preset_name);
		m_presets.insert(preset_name, preset);
		if (!find_preset_bank(preset_name))
			m_preset_list.append(preset_name);
	}

	set_current_preset(preset_name);

	return preset;
}


void samplv1_presets::remove_preset ( const QString& preset_name )
{
	Preset *preset = m_presets.value(preset_name, nullptr);
	if (preset && m_current_preset == preset)
		m_current_preset = nullptr;
	if (preset && m_presets.remove(preset_name))
		delete preset;

	Bank *bank = find_preset_bank(preset_name);
	if (bank)
		bank->remove_preset(preset_name);
	else
		m_preset_list.removeAll(preset_name);
}


void samplv1_presets::clear_presets (void)
{
	qDeleteAll(m_presets);
	m_presets.clear();

	m_preset_list.clear();

	m_current_preset = nullptr;
}


// current bank/preset managers
void samplv1_presets::set_current_bank ( const QString& bank_name )
{
	m_current_bank = find_bank(bank_name);
}


void samplv1_presets::set_current_preset ( const QString& preset_name )
{
	m_current_preset = find_preset(preset_name);
}


// end of samplv1_presets.cpp
