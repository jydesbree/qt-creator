/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerstatewidget.h"

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTime>
#include <QDebug>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateWidget::QmlProfilerStateWidgetPrivate
{
    public:
    QmlProfilerStateWidgetPrivate(QmlProfilerStateWidget *qq) { Q_UNUSED(qq); }

    QLabel *text;
    QProgressBar *progressBar;

    QmlProfilerStateManager *m_profilerState;
    QmlProfilerModelManager *m_modelManager;
};

QmlProfilerStateWidget::QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
                                QmlProfilerModelManager *modelManager, QWidget *parent)
    : QFrame(parent), d(new QmlProfilerStateWidgetPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler State Display"));
    setFrameStyle(QFrame::StyledPanel);

    // UI elements
    QVBoxLayout *layout = new QVBoxLayout(this);
    resize(200,70);

    d->text = new QLabel(this);
    d->text->setAlignment(Qt::AlignCenter);
    setAutoFillBackground(true);
    layout->addWidget(d->text);

    d->progressBar = new QProgressBar(this);
    layout->addWidget(d->progressBar);
    d->progressBar->setRange(0, 0);
    d->progressBar->setVisible(false);

    setLayout(layout);

    // profiler state
    d->m_modelManager = modelManager;
    connect(d->m_modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    d->m_profilerState = stateManager;
    connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    connect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerStateWidget::updateDisplay);

    updateDisplay();
}

QmlProfilerStateWidget::~QmlProfilerStateWidget()
{
    delete d;
}

void QmlProfilerStateWidget::reposition()
{
    QWidget *parentWidget = qobject_cast<QWidget *>(parent());
    // positioning it at 2/3 height (it looks better)
    move(parentWidget->width()/2 - width()/2, parentWidget->height()/3 - height()/2);
}

void QmlProfilerStateWidget::showText(const QString &text, bool showProgress)
{
    setVisible(true);
    d->progressBar->setVisible(showProgress);
    d->text->setText(text);
    resize(300, 70);
    reposition();
}

void QmlProfilerStateWidget::updateDisplay()
{
    // When application is being profiled
    if (d->m_profilerState->serverRecording()) {
        showText(tr("Profiling application"));
        return;
    }

    QmlProfilerModelManager::State state = d->m_modelManager->state();
    if (state == QmlProfilerModelManager::Done || state == QmlProfilerModelManager::Empty) {
        // After profiling, there is an empty trace
        if (d->m_modelManager->traceTime()->duration() > 0 && d->m_modelManager->isEmpty()) {
            showText(tr("No QML events recorded"));
            return;
        }
    } else if (!d->m_modelManager->isEmpty()) {
        // When datamodel is acquiring or processing data
        if (state == QmlProfilerModelManager::ProcessingData) {
            showText(tr("Processing data"), true);
        } else if (d->m_profilerState->currentState() != QmlProfilerStateManager::Idle) {
            if (state == QmlProfilerModelManager::AcquiringData) {
                // we don't know how much more, so progress numbers are strange here
                showText(tr("Loading buffered data"));
            } else if (state == QmlProfilerModelManager::ClearingData) {
                // when starting a second recording from the same process without aggregation
                showText(tr("Clearing old trace"));
            }
        } else if (state == QmlProfilerModelManager::AcquiringData) {
            // Application died before all data could be read
            showText(tr("Loading offline data"));
        } else if (state == QmlProfilerModelManager::ClearingData) {
            showText(tr("Clearing old trace"));
        }
        return;
    } else if (state == QmlProfilerModelManager::AcquiringData) {
        showText(tr("Waiting for data"));
        return;
    }

    // There is a trace on view, hide this dialog
    d->progressBar->setVisible(false);
    setVisible(false);
}

} // namespace Internal
} // namespace QmlProfiler
