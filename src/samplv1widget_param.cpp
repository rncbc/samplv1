// samplv1widget_param.cpp
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

#include "samplv1widget_param.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>

#include <QGridLayout>
#include <QMouseEvent>

#include <math.h>


// Integer value round.
inline int iroundf(float x) { return int(x < 0.0f ? x - 0.5f : x + 0.5f); }


//-------------------------------------------------------------------------
// samplv1widget_param_style - Custom widget style.
//

#include <QProxyStyle>
#include <QPainter>
#include <QIcon>


class samplv1widget_param_style : public QProxyStyle
{
public:

	// Constructor.
	samplv1widget_param_style() : QProxyStyle()
	{
		m_icon.addPixmap(
			QPixmap(":/images/ledOff.png"), QIcon::Normal, QIcon::Off);
		m_icon.addPixmap(
			QPixmap(":/images/ledOn.png"), QIcon::Normal, QIcon::On);
	}


	// Hints override.
	int styleHint(StyleHint hint, const QStyleOption *option,
		const QWidget *widget, QStyleHintReturn *retdata) const
	{
		if (hint == QStyle::SH_UnderlineShortcut)
			return 0;
		else
			return QProxyStyle::styleHint(hint, option, widget, retdata);
	}

	// Paint job.
	void drawPrimitive(PrimitiveElement element,
		const QStyleOption *option,
		QPainter *painter, const QWidget *widget) const
	{
		if (element == PE_IndicatorRadioButton ||
			element == PE_IndicatorCheckBox) {
			const QRect& rect = option->rect;
			if (option->state & State_Enabled) {
				if (option->state & State_On)
					m_icon.paint(painter, rect,
						Qt::AlignCenter, QIcon::Normal, QIcon::On);
				else
			//	if (option->state & State_Off)
					m_icon.paint(painter, rect,
						Qt::AlignCenter, QIcon::Normal, QIcon::Off);
			} else {
				m_icon.paint(painter, rect,
					Qt::AlignCenter, QIcon::Disabled, QIcon::Off);
			}
		}
		else
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}

	// Spiced up text margins
	void drawItemText(QPainter *painter, const QRect& rectangle,
		int alignment, const QPalette& palette, bool enabled,
		const QString& text, QPalette::ColorRole textRole) const
	{
		QRect rect = rectangle;
		rect.setLeft(rect.left() - 4);
		rect.setRight(rect.right() + 4);
		QProxyStyle::drawItemText(painter, rect,
			alignment, palette, enabled, text, textRole);
	}

	static void addRef ()
		{ if (++g_iRefCount == 1) g_pStyle = new samplv1widget_param_style(); }

	static void releaseRef ()
		{ if (--g_iRefCount == 0) { delete g_pStyle; g_pStyle = nullptr; } }

	static samplv1widget_param_style *getRef ()
		{ return g_pStyle; }

private:

	QIcon m_icon;

	static samplv1widget_param_style *g_pStyle;
	static unsigned int g_iRefCount;
};


samplv1widget_param_style *samplv1widget_param_style::g_pStyle = nullptr;
unsigned int samplv1widget_param_style::g_iRefCount = 0;


//-------------------------------------------------------------------------
// samplv1widget_param - Custom composite widget.
//

// Constructor.
samplv1widget_param::samplv1widget_param ( QWidget *pParent ) : QWidget(pParent)
{
	const QFont& font = QWidget::font();
	const QFont font2(font.family(), font.pointSize() - 2);
	QWidget::setFont(font2);

	m_fValue = 0.0f;

	m_fMinimum = 0.0f;
	m_fMaximum = 1.0f;

	m_fScale = 1.0f;

	resetDefaultValue();

	QWidget::setMaximumSize(QSize(52, 72));

	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setContentsMargins(0, 0, 0, 0);
	pGridLayout->setSpacing(0);
	QWidget::setLayout(pGridLayout);
}


// Accessors.
void samplv1widget_param::setText ( const QString& sText )
{
	setValue(sText.toFloat());
}


QString samplv1widget_param::text (void) const
{
	return QString::number(value());
}


void samplv1widget_param::setValue ( float fValue )
{
	QPalette pal;

	if (m_iDefaultValue == 0) {
		m_fDefaultValue = fValue;
		m_iDefaultValue++;
	}
	else
	if (QWidget::isEnabled()
		&& ::fabsf(fValue - m_fDefaultValue) > 0.0001f) {
		pal.setColor(QPalette::Base,
			(pal.window().color().value() < 0x7f
				? QColor(Qt::darkYellow).darker()
				: QColor(Qt::yellow).lighter()));
	}

	QWidget::setPalette(pal);

	if (::fabsf(fValue - m_fValue) > 0.0001f) {
		m_fValue = fValue;
		emit valueChanged(m_fValue);
	}
}


float samplv1widget_param::value (void) const
{
	return m_fValue;
}


QString samplv1widget_param::valueText (void) const
{
	return QString::number(value());
}


void samplv1widget_param::setMaximum ( float fMaximum )
{
	m_fMaximum = fMaximum;
}

float samplv1widget_param::maximum (void) const
{
	return m_fMaximum;
}


void samplv1widget_param::setMinimum ( float fMinimum )
{
	m_fMinimum = fMinimum;
}

float samplv1widget_param::minimum (void) const
{
	return m_fMinimum;
}


void samplv1widget_param::resetDefaultValue (void)
{
	m_fDefaultValue = 0.0f;
	m_iDefaultValue = 0;
}


bool samplv1widget_param::isDefaultValue (void) const
{
	return (m_iDefaultValue > 0);
}


void samplv1widget_param::setDefaultValue ( float fDefaultValue )
{
	m_fDefaultValue = fDefaultValue;
	m_iDefaultValue++;
}


float samplv1widget_param::defaultValue (void) const
{
	return m_fDefaultValue;
}


// Mouse behavior event handler.
void samplv1widget_param::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::MiddleButton) {
		if (m_iDefaultValue < 1) {
			m_fDefaultValue = 0.5f * (maximum() + minimum());
			m_iDefaultValue++;
		}
		setValue(m_fDefaultValue);
	}

	QWidget::mousePressEvent(pMouseEvent);
}


// Scale multiplier accessors.
void samplv1widget_param::setScale ( float fScale )
{
	m_fScale = fScale;
}


float samplv1widget_param::scale (void) const
{
	return m_fScale;
}


// Scale/value converters.
float samplv1widget_param::scaleFromValue ( float fValue ) const
{
	return (m_fScale * fValue);
}


float samplv1widget_param::valueFromScale ( float fScale ) const
{
	return (fScale / m_fScale);
}


//-------------------------------------------------------------------------
// samplv1widget_dial - A better QDial widget.

samplv1widget_dial::DialMode
samplv1widget_dial::g_dialMode = samplv1widget_dial::DefaultMode;

// Set knob dial mode behavior.
void samplv1widget_dial::setDialMode ( DialMode dialMode )
	{ g_dialMode = dialMode; }

samplv1widget_dial::DialMode samplv1widget_dial::dialMode (void)
	{ return g_dialMode; }


// Constructor.
samplv1widget_dial::samplv1widget_dial ( QWidget *pParent )
	: QDial(pParent), m_bMousePressed(false), m_fLastDragValue(0.0f)
{
}


// Mouse angle determination.
float samplv1widget_dial::mouseAngle ( const QPoint& pos )
{
	const float dx = pos.x() - (width() >> 1);
	const float dy = (height() >> 1) - pos.y();
	return 180.0f * ::atan2f(dx, dy) / float(M_PI);
}


// Alternate mouse behavior event handlers.
void samplv1widget_dial::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (g_dialMode == DefaultMode) {
		QDial::mousePressEvent(pMouseEvent);
	} else if (pMouseEvent->button() == Qt::LeftButton) {
		m_bMousePressed = true;
		m_posMouse = pMouseEvent->pos();
		m_fLastDragValue = float(value());
		emit sliderPressed();
	}
}


void samplv1widget_dial::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	if (g_dialMode == DefaultMode) {
		QDial::mouseMoveEvent(pMouseEvent);
		return;
	}

	if (!m_bMousePressed)
		return;

	const QPoint& pos = pMouseEvent->pos();
	const int dx = pos.x() - m_posMouse.x();
	const int dy = pos.y() - m_posMouse.y();
	float fAngleDelta =  mouseAngle(pos) - mouseAngle(m_posMouse);
	int iNewValue = value();

	switch (g_dialMode)	{
	case LinearMode:
		iNewValue = int(m_fLastDragValue) + dx - dy;
		break;
	case AngularMode:
	default:
		// Forget about the drag origin to be robust on full rotations
		if (fAngleDelta > +180.0f) fAngleDelta -= 360.0f;
		else
		if (fAngleDelta < -180.0f) fAngleDelta += 360.0f;
		m_fLastDragValue += float(maximum() - minimum()) * fAngleDelta / 270.0f;
		if (m_fLastDragValue > float(maximum()))
			m_fLastDragValue = float(maximum());
		else
		if (m_fLastDragValue < float(minimum()))
			m_fLastDragValue = float(minimum());
		m_posMouse = pos;
		iNewValue = int(m_fLastDragValue + 0.5f);
		break;
	}

	setValue(iNewValue);
	update();

	emit sliderMoved(value());
}


void samplv1widget_dial::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	if (g_dialMode == DefaultMode
		&& pMouseEvent->button() != Qt::MiddleButton) {
		QDial::mouseReleaseEvent(pMouseEvent);
	} else if (m_bMousePressed) {
		m_bMousePressed = false;
	}
}


//-------------------------------------------------------------------------
// samplv1widget_knob - Custom knob/dial widget.
//

// Constructor.
samplv1widget_knob::samplv1widget_knob ( QWidget *pParent ) : samplv1widget_param(pParent)
{
	m_pLabel = new QLabel();
	m_pLabel->setAlignment(Qt::AlignCenter);

	m_pDial = new samplv1widget_dial();
	m_pDial->setNotchesVisible(true);
	m_pDial->setMaximumSize(QSize(48, 48));

	QGridLayout *pGridLayout
		= static_cast<QGridLayout *> (samplv1widget_param::layout());
	pGridLayout->addWidget(m_pLabel, 0, 0, 1, 3);
	pGridLayout->addWidget(m_pDial,  1, 0, 1, 3);
	pGridLayout->setAlignment(m_pDial, Qt::AlignVCenter | Qt::AlignHCenter);

	QObject::connect(m_pDial,
		SIGNAL(valueChanged(int)),
		SLOT(dialValueChanged(int)));
}


void samplv1widget_knob::setText ( const QString& sText )
{
	m_pLabel->setText(sText);
}


QString samplv1widget_knob::text (void) const
{
	return m_pLabel->text();
}


void samplv1widget_knob::setValue ( float fValue )
{
	const bool bDialBlock = m_pDial->blockSignals(true);
	samplv1widget_param::setValue(fValue);
	m_pDial->setValue(scaleFromValue(fValue));
	m_pDial->blockSignals(bDialBlock);
}


void samplv1widget_knob::setMaximum ( float fMaximum )
{
	samplv1widget_param::setMaximum(fMaximum);
	m_pDial->setMaximum(scaleFromValue(fMaximum));
}


void samplv1widget_knob::setMinimum ( float fMinimum )
{
	samplv1widget_param::setMinimum(fMinimum);
	m_pDial->setMinimum(scaleFromValue(fMinimum));
}


// Scale-step accessors.
void samplv1widget_knob::setSingleStep ( float fSingleStep )
{
	m_pDial->setSingleStep(scaleFromValue(fSingleStep));
}


float samplv1widget_knob::singleStep (void) const
{
	return valueFromScale(m_pDial->singleStep());
}


// Dial change slot.
void samplv1widget_knob::dialValueChanged ( int iDialValue )
{
	setValue(valueFromScale(iDialValue));
}


//-------------------------------------------------------------------------
// samplv1widget_edit - A better QDoubleSpinBox widget.

samplv1widget_edit::EditMode
samplv1widget_edit::g_editMode = samplv1widget_edit::DefaultMode;

// Set spin-box edit mode behavior.
void samplv1widget_edit::setEditMode ( EditMode editMode )
	{ g_editMode = editMode; }

samplv1widget_edit::EditMode samplv1widget_edit::editMode (void)
	{ return g_editMode; }


// Constructor.
samplv1widget_edit::samplv1widget_edit ( QWidget *pParent )
	: QDoubleSpinBox(pParent), m_iTextChanged(0)
{
	QObject::connect(QDoubleSpinBox::lineEdit(),
		SIGNAL(textChanged(const QString&)),
		SLOT(lineEditTextChanged(const QString&)));
	QObject::connect(this,
		SIGNAL(editingFinished()),
		SLOT(spinBoxEditingFinished()));
	QObject::connect(this,
		SIGNAL(valueChanged(double)),
		SLOT(spinBoxValueChanged(double)));
}


// Alternate value change behavior handlers.
void samplv1widget_edit::lineEditTextChanged ( const QString& )
{
	if (g_editMode == DeferredMode)
		++m_iTextChanged;
}


void samplv1widget_edit::spinBoxEditingFinished (void)
{
	if (g_editMode == DeferredMode) {
		m_iTextChanged = 0;
		emit valueChangedEx(QDoubleSpinBox::value());
	}
}


void samplv1widget_edit::spinBoxValueChanged ( double spinValue )
{
	if (g_editMode != DeferredMode || m_iTextChanged == 0)
		emit valueChangedEx(spinValue);
}


// Inherited/override methods.
QValidator::State samplv1widget_edit::validate ( QString& sText, int& iPos ) const
{
	const QValidator::State state
		= QDoubleSpinBox::validate(sText, iPos);

	if (state == QValidator::Acceptable
		&& g_editMode == DeferredMode
		&& m_iTextChanged == 0)
		return QValidator::Intermediate;

	return state;
}


//-------------------------------------------------------------------------
// samplv1widget_spin - Custom knob/spin-box widget.
//

// Constructor.
samplv1widget_spin::samplv1widget_spin ( QWidget *pParent )
	: samplv1widget_knob(pParent)
{
	m_pSpinBox = new samplv1widget_edit();
	m_pSpinBox->setAccelerated(true);
	m_pSpinBox->setAlignment(Qt::AlignCenter);

	const QFontMetrics fm(samplv1widget_knob::font());
	m_pSpinBox->setMaximumHeight(fm.height() + 6);

	QGridLayout *pGridLayout
		= static_cast<QGridLayout *> (samplv1widget_knob::layout());
	pGridLayout->addWidget(m_pSpinBox, 2, 1, 1, 1);

	setScale(100.0f);

	setMinimum(0.0f);
	setMaximum(1.0f);

	setDecimals(1);

	QObject::connect(m_pSpinBox,
		SIGNAL(valueChangedEx(double)),
		SLOT(spinBoxValueChanged(double)));
}


// Virtual accessors.
void samplv1widget_spin::setValue ( float fValue )
{
	const bool bSpinBlock = m_pSpinBox->blockSignals(true);
	samplv1widget_knob::setValue(fValue);
	m_pSpinBox->setValue(scaleFromValue(fValue));
	m_pSpinBox->blockSignals(bSpinBlock);
}


void samplv1widget_spin::setMaximum ( float fMaximum )
{
	m_pSpinBox->setMaximum(scaleFromValue(fMaximum));
	samplv1widget_knob::setMaximum(fMaximum);
}


void samplv1widget_spin::setMinimum ( float fMinimum )
{
	m_pSpinBox->setMinimum(scaleFromValue(fMinimum));
	samplv1widget_knob::setMinimum(fMinimum);
}


QString samplv1widget_spin::valueText (void) const
{
	return QString::number(m_pSpinBox->value(), 'f', 1);
}


// Internal widget slots.
void samplv1widget_spin::spinBoxValueChanged ( double spinValue )
{
	samplv1widget_knob::setValue(valueFromScale(float(spinValue)));
}


// Special value text (minimum)
void samplv1widget_spin::setSpecialValueText ( const QString& sText )
{
	m_pSpinBox->setSpecialValueText(sText);
}


QString samplv1widget_spin::specialValueText (void) const
{
	return m_pSpinBox->specialValueText();
}


bool samplv1widget_spin::isSpecialValue (void) const
{
	return (m_pSpinBox->minimum() >= m_pSpinBox->value());
}


// Decimal digits allowed.
void samplv1widget_spin::setDecimals ( int iDecimals )
{
	m_pSpinBox->setDecimals(iDecimals);
	m_pSpinBox->setSingleStep(::powf(10.0f, - float(iDecimals)));

	setSingleStep(0.1f);
}

int samplv1widget_spin::decimals (void) const
{
	return m_pSpinBox->decimals();
}


//-------------------------------------------------------------------------
// samplv1widget_combo - Custom knob/combo-box widget.
//

// Constructor.
samplv1widget_combo::samplv1widget_combo ( QWidget *pParent )
	: samplv1widget_knob(pParent)
{
	m_pComboBox = new QComboBox();

	const QFontMetrics fm(samplv1widget_knob::font());
	m_pComboBox->setMaximumHeight(fm.height() + 6);

	QGridLayout *pGridLayout
		= static_cast<QGridLayout *> (samplv1widget_knob::layout());
	pGridLayout->addWidget(m_pComboBox, 2, 0, 1, 3);

//	setScale(1.0f);

	QObject::connect(m_pComboBox,
		SIGNAL(activated(int)),
		SLOT(comboBoxValueChanged(int)));
}


// Virtual accessors.
void samplv1widget_combo::setValue ( float fValue )
{
	const bool bComboBlock = m_pComboBox->blockSignals(true);
	samplv1widget_knob::setValue(fValue);
	m_pComboBox->setCurrentIndex(iroundf(fValue));
	m_pComboBox->blockSignals(bComboBlock);
}


QString samplv1widget_combo::valueText (void) const
{
	return m_pComboBox->currentText();
}


// Special combo-box mode accessors.
void samplv1widget_combo::insertItems ( int iIndex, const QStringList& items )
{
	m_pComboBox->insertItems(iIndex, items);

	setMinimum(0.0f);

	const int iItemCount = m_pComboBox->count();
	if (iItemCount > 0)
		setMaximum(float(iItemCount - 1));
	else
		setMaximum(1.0f);

	setSingleStep(1.0f);
}


void samplv1widget_combo::clear (void)
{
	m_pComboBox->clear();

	setMinimum(0.0f);
	setMaximum(1.0f);

	setSingleStep(1.0f);
}


// Internal widget slots.
void samplv1widget_combo::comboBoxValueChanged ( int iComboValue )
{
	samplv1widget_knob::setValue(float(iComboValue));
}


// Reimplemented mouse-wheel stepping.
void samplv1widget_combo::wheelEvent ( QWheelEvent *pWheelEvent )
{
	const int delta
		= (pWheelEvent->angleDelta().y() / 120);
	if (delta) {
		float fValue = value() + float(delta);
		if (fValue < minimum())
			fValue = minimum();
		else
		if (fValue > maximum())
			fValue = maximum();
		setValue(fValue);
	}
}


//-------------------------------------------------------------------------
// samplv1widget_radio - Custom radio/button widget.
//

// Constructor.
samplv1widget_radio::samplv1widget_radio ( QWidget *pParent )
	: samplv1widget_param(pParent), m_group(this)
{
	samplv1widget_param_style::addRef();
#if 0
	samplv1widget_param::setStyleSheet(
	//	"QRadioButton::indicator { width: 16px; height: 16px; }"
		"QRadioButton::indicator::unchecked { image: url(:/images/ledOff.png); }"
		"QRadioButton::indicator::checked   { image: url(:/images/ledOn.png);  }"
	);
#endif

	QObject::connect(&m_group,
	#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
		SIGNAL(idClicked(int)),
	#else
		SIGNAL(buttonClicked(int)),
	#endif
		SLOT(radioGroupValueChanged(int)));
}


// Destructor.
samplv1widget_radio::~samplv1widget_radio (void)
{
	samplv1widget_param_style::releaseRef();
}


// Virtual accessors.
void samplv1widget_radio::setValue ( float fValue )
{
	const int iRadioValue = iroundf(fValue);
	QRadioButton *pRadioButton
		= static_cast<QRadioButton *> (m_group.button(iRadioValue));
	if (pRadioButton) {
		const bool bRadioBlock = pRadioButton->blockSignals(true);
		samplv1widget_param::setValue(float(iRadioValue));
		pRadioButton->setChecked(true);
		pRadioButton->blockSignals(bRadioBlock);
	}
}


QString samplv1widget_radio::valueText (void) const
{
	QString sValueText;
	const int iRadioValue = iroundf(value());
	QRadioButton *pRadioButton
		= static_cast<QRadioButton *> (m_group.button(iRadioValue));
	if (pRadioButton)
		sValueText = pRadioButton->text();
	return sValueText;
}


// Special combo-box mode accessors.
void samplv1widget_radio::insertItems ( int iIndex, const QStringList& items )
{
	const QFont& font = samplv1widget_param::font();
	const QFont font1(font.family(), font.pointSize() - 1);

	QGridLayout *pGridLayout
		= static_cast<QGridLayout *> (samplv1widget_param::layout());
	const QString sToolTipMask(samplv1widget_param::toolTip() + ": %1");
	QStringListIterator iter(items);
	while (iter.hasNext()) {
		const QString& sValueText = iter.next();
		QRadioButton *pRadioButton = new QRadioButton(sValueText);
		pRadioButton->setStyle(samplv1widget_param_style::getRef());
		pRadioButton->setFont(font1);
		pRadioButton->setToolTip(sToolTipMask.arg(sValueText));
		pGridLayout->addWidget(pRadioButton, iIndex, 0);
		m_group.addButton(pRadioButton, iIndex);
		++iIndex;
	}

	setMinimum(0.0f);

	const QList<QAbstractButton *> list = m_group.buttons();
	const int iRadioCount = list.count();
	if (iRadioCount > 0)
		setMaximum(float(iRadioCount - 1));
	else
		setMaximum(1.0f);
}


void samplv1widget_radio::clear (void)
{
	const QList<QAbstractButton *> list = m_group.buttons();
	QListIterator<QAbstractButton *> iter(list);
	while (iter.hasNext()) {
		QRadioButton *pRadioButton
			= static_cast<QRadioButton *> (iter.next());
		if (pRadioButton)
			m_group.removeButton(pRadioButton);
	}

	setMinimum(0.0f);
	setMaximum(1.0f);
}


void samplv1widget_radio::radioGroupValueChanged ( int iRadioValue )
{
	samplv1widget_param::setValue(float(iRadioValue));
}


//-------------------------------------------------------------------------
// samplv1widget_check - Custom check-box widget.
//

// Constructor.
samplv1widget_check::samplv1widget_check ( QWidget *pParent )
	: samplv1widget_param(pParent)
{
	samplv1widget_param_style::addRef();
#if 0
	samplv1widget_param::setStyleSheet(
	//	"QCheckBox::indicator { width: 16px; height: 16px; }"
		"QCheckBox::indicator::unchecked { image: url(:/images/ledOff.png); }"
		"QCheckBox::indicator::checked   { image: url(:/images/ledOn.png);  }"
	);
#endif
	m_pCheckBox = new QCheckBox();
	m_pCheckBox->setStyle(samplv1widget_param_style::getRef());

	m_alignment = Qt::AlignHCenter | Qt::AlignVCenter;

	QGridLayout *pGridLayout
		= static_cast<QGridLayout *> (samplv1widget_param::layout());
	pGridLayout->addWidget(m_pCheckBox, 0, 0);
	pGridLayout->setAlignment(m_pCheckBox, m_alignment);

	samplv1widget_param::setMaximumSize(QSize(72, 72));

	QObject::connect(m_pCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(checkBoxValueChanged(bool)));
}


// Destructor.
samplv1widget_check::~samplv1widget_check (void)
{
	samplv1widget_param_style::releaseRef();
}


// Accessors.
void samplv1widget_check::setText ( const QString& sText )
{
	m_pCheckBox->setText(sText);
}


QString samplv1widget_check::text (void) const
{
	return m_pCheckBox->text();
}


void samplv1widget_check::setAlignment ( Qt::Alignment alignment )
{
	m_alignment = alignment;

	QLayout *pLayout = samplv1widget_param::layout();
	if (pLayout)
		pLayout->setAlignment(m_pCheckBox, m_alignment);
}


Qt::Alignment samplv1widget_check::alignment (void) const
{
	return m_alignment;
}


// Virtual accessors.
void samplv1widget_check::setValue ( float fValue )
{
	const bool bCheckValue = (fValue > 0.5f * (maximum() + minimum()));
	const bool bCheckBlock = m_pCheckBox->blockSignals(true);
	samplv1widget_param::setValue(bCheckValue ? maximum() : minimum());
	m_pCheckBox->setChecked(bCheckValue);
	m_pCheckBox->blockSignals(bCheckBlock);
}


void samplv1widget_check::checkBoxValueChanged ( bool bCheckValue )
{
	samplv1widget_param::setValue(bCheckValue ? maximum() : minimum());
}


//-------------------------------------------------------------------------
// samplv1widget_group - Custom checkable group-box widget.
//

// Constructor.
samplv1widget_group::samplv1widget_group ( QWidget *pParent )
	: QGroupBox(pParent)
{
	samplv1widget_param_style::addRef();
#if 0
	QGroupBox::setStyleSheet(
	//	"QGroupBox::indicator { width: 16px; height: 16px; }"
		"QGroupBox::indicator::unchecked { image: url(:/images/ledOff.png); }"
		"QGroupBox::indicator::checked   { image: url(:/images/ledOn.png);  }"
		);
#endif
	QGroupBox::setStyle(samplv1widget_param_style::getRef());

	m_pParam = new samplv1widget_param(this);
	m_pParam->setToolTip(QGroupBox::toolTip());
	m_pParam->setValue(0.5f); // HACK: half-way on.

	QObject::connect(m_pParam,
		 SIGNAL(valueChanged(float)),
		 SLOT(paramValueChanged(float)));

	QObject::connect(this,
		 SIGNAL(toggled(bool)),
		 SLOT(groupBoxValueChanged(bool)));
}


// Destructor.
samplv1widget_group::~samplv1widget_group (void)
{
	samplv1widget_param_style::releaseRef();

	delete m_pParam;
}


// Accessors.
void samplv1widget_group::setToolTip ( const QString& sToolTip )
{
	m_pParam->setToolTip(sToolTip);
}


QString samplv1widget_group::toolTip (void) const
{
	return m_pParam->toolTip();
}


samplv1widget_param *samplv1widget_group::param (void) const
{
	return m_pParam;
}


// Virtual accessors.
void samplv1widget_group::paramValueChanged ( float fValue )
{
	const float fMaximum = m_pParam->maximum();
	const float fMinimum = m_pParam->minimum();

	const bool bGroupValue = (fValue > 0.5f * (fMaximum + fMinimum));
	const bool bGroupBlock = QGroupBox::blockSignals(true);
	QGroupBox::setChecked(bGroupValue);
	QGroupBox::blockSignals(bGroupBlock);
}


void samplv1widget_group::groupBoxValueChanged ( bool bGroupValue )
{
	const float fMaximum = m_pParam->maximum();
	const float fMinimum = m_pParam->minimum();

	m_pParam->setValue(bGroupValue ? fMaximum : fMinimum);
}


// end of samplv1widget_param.cpp
