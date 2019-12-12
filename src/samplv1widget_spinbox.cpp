// samplv1widget_spinbox.cpp
//
/****************************************************************************
   Copyright (C) 2012-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1widget_spinbox.h"

#include <QLineEdit>


#include <math.h>


//----------------------------------------------------------------------------
// samplv1widget_spinbox -- A time-formatted spin-box widget.

// Constructor.
samplv1widget_spinbox::samplv1widget_spinbox ( QWidget *parent )
	: QAbstractSpinBox(parent), m_srate(44100.0f), m_format(Frames),
		m_value(0), m_minimum(0), m_maximum(0), m_changed(0)
{
	QObject::connect(this,
		SIGNAL(editingFinished()),
		SLOT(editingFinishedSlot()));
	QObject::connect(QAbstractSpinBox::lineEdit(),
		SIGNAL(textChanged(const QString&)),
		SLOT(valueChangedSlot(const QString&)));
}


// Mark that we got actual value.
void samplv1widget_spinbox::showEvent ( QShowEvent */*event*/ )
{
	QLineEdit *line_edit = QAbstractSpinBox::lineEdit();
	const bool block_signals = line_edit->blockSignals(true);
	line_edit->setText(textFromValue(m_value));
	QAbstractSpinBox::interpretText();
	line_edit->blockSignals(block_signals);
}


// Time-scale accessors.
void samplv1widget_spinbox::setSampleRate ( float srate )
{
	m_srate = srate;

	updateText();
}

float samplv1widget_spinbox::sampleRate (void) const
{
	return m_srate;
}


// Display-format accessors.
void samplv1widget_spinbox::setFormat ( Format format )
{
	m_format = format;

	updateText();
}

samplv1widget_spinbox::Format samplv1widget_spinbox::format (void) const
{
	return m_format;
}


// Nominal value (in frames) accessors.
void samplv1widget_spinbox::setValue ( uint32_t value )
{
	if (updateValue(value, true))
		updateText();
}

uint32_t samplv1widget_spinbox::value (void) const
{
	return m_value;
}


// Minimum value (in frames) accessors.
void samplv1widget_spinbox::setMinimum ( uint32_t minimum )
{
	m_minimum = minimum;
}

uint32_t samplv1widget_spinbox::minimum (void) const
{
	return m_minimum;
}


// Maximum value (in frames) accessors.
void samplv1widget_spinbox::setMaximum ( uint32_t maximum )
{
	m_maximum = maximum;
}

uint32_t samplv1widget_spinbox::maximum (void) const
{
	return m_maximum;
}


// Inherited/override methods.
QValidator::State samplv1widget_spinbox::validate (
	QString& text, int& pos ) const
{
	if (pos == 0)
		return QValidator::Acceptable;

	const QChar& ch = text.at(pos - 1);
	switch (m_format) {
	case Time:
		if (ch == ':' || ch == '.')
			return QValidator::Acceptable;
		// Fall thru...
	case Frames:
		if (ch.isDigit())
			return QValidator::Acceptable;
		// Fall thru...
	default:
		break;
	}

	return QValidator::Invalid;
}


void samplv1widget_spinbox::fixup ( QString& text ) const
{
	text = textFromValue(m_value);
}


void samplv1widget_spinbox::stepBy ( int steps )
{
	QLineEdit *line_edit = QAbstractSpinBox::lineEdit();
	const int cursor_pos = line_edit->cursorPosition();
	
	if (m_format == Time) {
		const QString& text = line_edit->text();
		const int pos = text.section(':', 0, 0).length() + 1;
		if (cursor_pos < pos)
			steps *= int(3600.0f * m_srate);
		else
		if (cursor_pos < pos + text.section(':', 1, 1).length() + 1)
			steps *= int(60.0f * m_srate);
		else
		if (cursor_pos < text.section('.', 0, 0).length() + 1)
			steps *= int(m_srate);
		else
			steps *= int(0.001f * m_srate);
	}

	long value = long(m_value) + steps;
	if (value < 0)
		value = 0;
	setValue(value);

	line_edit->setCursorPosition(cursor_pos);
}


QAbstractSpinBox::StepEnabled samplv1widget_spinbox::stepEnabled (void) const
{
	StepEnabled flags = StepUpEnabled;
	if (value() > 0)
		flags |= StepDownEnabled;
	return flags;
}


// Value/text format converter utilities.
uint32_t samplv1widget_spinbox::valueFromText (
	const QString& text, Format format, float srate )
{
	if (format == Frames)
		return text.toULong();

	// Time frame code in hh:mm:ss.zzz ...
	const uint32_t hh = text.section(':', 0, 0).toULong();
	const uint32_t mm = text.section(':', 1, 1).toULong() + 60 * hh;
	const float secs = text.section(':', 2).toFloat() + float(60 * mm);
	return ::lrintf(secs * srate);
}

QString samplv1widget_spinbox::textFromValue (
	uint32_t value, Format format, float srate )
{
	if (format == Frames)
		return QString::number(value);

	// Time frame code in hh:mm:ss.zzz ...
	float secs = float(value) / srate;

	uint32_t hh = 0, mm = 0, ss = 0;
	if (secs >= 3600.0f) {
		hh = uint32_t(secs / 3600.0f);
		secs -= float(hh) * 3600.0f;
	}
	if (secs >= 60.0f) {
		mm = uint32_t(secs / 60.0f);
		secs -= float(mm) * 60.0f;
	}
	if (secs >= 0.0f) {
		ss = uint32_t(secs);
		secs -= float(ss);
	}

	const uint32_t zzz = uint32_t(secs * 1000.0f);
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
	return QString().sprintf("%02u:%02u:%02u.%03u", hh, mm, ss, zzz);
#else
	return QString::asprintf("%02u:%02u:%02u.%03u", hh, mm, ss, zzz);
#endif
}


// Value/text format converters.
uint32_t samplv1widget_spinbox::valueFromText ( const QString& text ) const
{
	return valueFromText(text, m_format, m_srate);
}

QString samplv1widget_spinbox::textFromValue ( uint32_t value ) const
{
	return textFromValue(value, m_format, m_srate);
}


// Common value setler.
bool samplv1widget_spinbox::updateValue ( uint32_t value, bool notify )
{
	if (value < m_minimum)
		value = m_minimum;
	if (value > m_maximum && m_maximum > m_minimum)
		value = m_maximum;

	if (m_value != value) {
		m_value  = value;
		++m_changed;
	}

	const int changed = m_changed;

	if (notify && m_changed > 0) {
		emit valueChanged(m_value);
		m_changed = 0;
	}

	return (changed > 0);
}


void samplv1widget_spinbox::updateText (void)
{
	if (QAbstractSpinBox::isVisible()) {
		QLineEdit *line_edit = QAbstractSpinBox::lineEdit();
		const bool block_signals = line_edit->blockSignals(true);
		const int cursor_pos = line_edit->cursorPosition();
		line_edit->setText(textFromValue(m_value));
	//	QAbstractSpinBox::interpretText();
		line_edit->setCursorPosition(cursor_pos);
		line_edit->blockSignals(block_signals);
	}
}


// Pseudo-fixup slot.
void samplv1widget_spinbox::editingFinishedSlot (void)
{
	if (m_changed > 0) {
		// Kind of final fixup.
		if (updateValue(valueFromText(QAbstractSpinBox::text()), true)) {
			// Rephrase text display...
			updateText();
		}
	}
}


// Textual value change notification.
void samplv1widget_spinbox::valueChangedSlot ( const QString& text )
{
	// Kind of interim fixup.
	if (updateValue(valueFromText(text), false)) {
		// Just forward this one...
		emit valueChanged(text);
	}
}


// end of samplv1widget_spinbox.cpp
