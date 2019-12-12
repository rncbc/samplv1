// samplv1widget_spinbox.h
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

#ifndef __samplv1widget_spinbox_h
#define __samplv1widget_spinbox_h

#include <QAbstractSpinBox>

#include <stdint.h>


//----------------------------------------------------------------------------
// samplv1widget_spinbox -- A time-formatted spin-box widget.

class samplv1widget_spinbox : public QAbstractSpinBox
{
    Q_OBJECT

public:

	// Constructor.
	samplv1widget_spinbox(QWidget *parent = 0);

	// Time-scale accessors.
	void setSampleRate(float srate);
	float sampleRate() const;

	// Display-format accessors.
	enum Format { Frames = 0, Time };

	void setFormat(Format format);
	Format format() const;

	// Nominal value (in frames) accessors.
	void setValue(uint32_t value);
	uint32_t value() const;

	// Minimum value (in frames) accessors.
	void setMinimum(uint32_t minimum);
	uint32_t minimum() const;

	// Maximum value (in frames) accessors.
	void setMaximum(uint32_t maximum);
	uint32_t maximum() const;

	// Value/text format converter utilities.
	static uint32_t valueFromText(
		const QString& text, Format format, float srate);
	static QString textFromValue(
		uint32_t value, Format format, float srate);

signals:

	// Common value change notification.
	void valueChanged(uint32_t);
	void valueChanged(const QString&);

protected:

	// Mark that we got actual value.
	void showEvent(QShowEvent *);

	// Inherited/override methods.
	QValidator::State validate(QString& text, int& pos) const;
	void fixup(QString& text) const;
	void stepBy(int steps);
	StepEnabled stepEnabled() const;

	// Value/text format converters.
	uint32_t valueFromText(const QString& text) const;
	QString textFromValue(uint32_t value) const;

	// Common value/text setlers.
	bool updateValue(uint32_t value, bool notify);
	void updateText();

protected slots:

	// Pseudo-fixup slot.
	void editingFinishedSlot();
	void valueChangedSlot(const QString&);

private:

	// Instance variables.
	float m_srate;

	Format m_format;

	uint32_t m_value;
	uint32_t m_minimum;
	uint32_t m_maximum;

	int m_changed;
};


#endif  // __samplv1widget_spinbox_h


// end of samplv1widget_spinbox.h
