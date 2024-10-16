/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include "fileprogressdialog.h"
#include "../debugdialog.h"
#include "../processeventblocker.h"

#include <QVBoxLayout>
#include <QTextStream>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLocale>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCloseEvent>
#include <QApplication>
#include <QSplashScreen>
#include <QScreen>

/////////////////////////////////////

FileProgressDialog::FileProgressDialog(const QString & title, int initialMaximum, QWidget * parent) : QDialog(parent),
    m_incValueMod(2)
{
	QSplashScreen *splash = nullptr;
	Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
		splash = qobject_cast<QSplashScreen *>(widget);
		if (splash != nullptr) {
			break;
		}
	}

	init(title, initialMaximum);
	setModal(splash == nullptr);			// OS X Lion doesn't seem to like modal dialogs during splash time

	show();
	if (splash != nullptr) {
		QRect sr = splash->geometry();
		QRect r = this->geometry();
		QRect fr = this->frameGeometry();
		int offset = sr.bottom() - r.top() + (fr.height() - r.height());
		r.adjust(0, offset, 0, offset);
		this->setGeometry(r);
	}
	ProcessEventBlocker::processEvents();
}

FileProgressDialog::FileProgressDialog(QWidget *parent) : QDialog(parent)
{
	init(QObject::tr("File Progress..."), 0);
}


FileProgressDialog::~FileProgressDialog() {
	m_timer.stop();
}

void FileProgressDialog::init(const QString & title, int initialMaximum)
{
	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	flags ^= Qt::WindowCloseButtonHint;
	flags ^= Qt::WindowSystemMenuHint;
	flags ^= Qt::WindowContextHelpButtonHint;
	//flags ^= Qt::WindowTitleHint;
	setWindowFlags(flags);

	this->setWindowTitle(title);

	auto * vLayout = new QVBoxLayout(this);

	m_message = new QLabel(this);
	m_message->setMinimumWidth(300);
	vLayout->addWidget(m_message);

	m_progressBar = new QProgressBar(this);
	m_progressBar->setMinimum(0);
	m_progressBar->setMaximum(initialMaximum);
	vLayout->addWidget(m_progressBar);

	//QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
	//buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	//buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	//connect(buttonBox, SIGNAL(rejected()), this, SLOT(sendCancel()));
	//vLayout->addWidget(buttonBox);

	this->setLayout(vLayout);
}

void FileProgressDialog::setMinimum(int minimum) {
	m_progressBar->setMinimum(minimum);
}

void FileProgressDialog::setMaximum(int maximum) {
	m_progressBar->setMaximum(maximum);
	//DebugDialog::debug(QString("set maximum:%1").arg(maximum));
}

void FileProgressDialog::addMaximum(int maximum) {
	if (maximum != 0) {
		m_progressBar->setMaximum(m_progressBar->maximum() + maximum);
		//DebugDialog::debug(QString("set maximum:%1").arg(maximum));
	}
}

void FileProgressDialog::setValue(int value) {
	m_progressBar->setValue(value);
	ProcessEventBlocker::processEvents();
}

void FileProgressDialog::incValue() {
	m_progressBar->setValue(m_progressBar->value() + 1);
	if (m_progressBar->value() % m_incValueMod == 0) {
		ProcessEventBlocker::processEvents();
	}
}

int FileProgressDialog::value() {
	return m_progressBar->value();
}

void FileProgressDialog::sendCancel() {
	Q_EMIT cancel();
}

void FileProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}

void FileProgressDialog::setMessage(const QString & message) {
	m_message->setText(message);
	ProcessEventBlocker::processEvents();
}

void FileProgressDialog::setBinLoadingCount(int count)
{
	m_binLoadingCount = count;
	m_binLoadingIndex = -1;
	m_binLoadingStart = value();
}

void FileProgressDialog::setBinLoadingChunk(int chunk)
{
	m_binLoadingChunk = chunk;
}

void FileProgressDialog::loadingInstancesSlot(class ModelBase *, QDomElement & instances)
{
	m_binLoadingValue = m_binLoadingStart + (++m_binLoadingIndex * m_binLoadingChunk / (double) m_binLoadingCount);
	setValue(m_binLoadingValue);

	int count = instances.childNodes().count();

	if (count == 0) {
		// Set a small increment to show some progress
		m_binLoadingInc = m_binLoadingChunk / (double) (m_binLoadingCount * 3);
	} else {
		// Original calculation
		m_binLoadingInc = m_binLoadingChunk / (double) (m_binLoadingCount * 3 * count);
	}
}

void FileProgressDialog::loadingInstanceSlot(class ModelBase *, QDomElement & instance)
{
	Q_UNUSED(instance);
	//QString text;
	//QTextStream textStream(&text);
	//instance.save(textStream, 0);
	//DebugDialog::debug(QString("loading %1").arg(text));
	//DebugDialog::debug(QString("loading %1").arg(instance.attribute("path")));
	settingItemSlot();
}

void FileProgressDialog::settingItemSlot()
{
	m_binLoadingValue += m_binLoadingInc;
	if ((int) m_binLoadingValue > value()) {
		setValue(m_binLoadingValue);
	}
}

void FileProgressDialog::resizeEvent(QResizeEvent * event)
{
	QDialog::resizeEvent(event);
	QRect scr = QApplication::primaryScreen()->geometry();
	move( scr.center() - rect().center() );
}

void FileProgressDialog::setIncValueMod(int mod) {
	m_incValueMod = mod;
}

void FileProgressDialog::setIndeterminate() {
	m_progressBar->setRange(0, 0);
	m_timer.setSingleShot(false);
	m_timer.setInterval(1000);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateIndeterminate()));
	m_timer.start();
}

void FileProgressDialog::updateIndeterminate() {
	if (m_progressBar != nullptr) {
		m_progressBar->setValue(0);
	}
}
