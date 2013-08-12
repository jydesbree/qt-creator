/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "analyzersettings.h"

#include "analyzermanager.h"
#include "analyzerrunconfigwidget.h"
#include "ianalyzertool.h"
#include "analyzerplugin.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QSettings>

using namespace Analyzer::Internal;

static const char useGlobalC[] = "Analyzer.Project.UseGlobal";

namespace Analyzer {

ISettingsAspect *ISettingsAspect::clone() const
{
    ISettingsAspect *other = create();
    QVariantMap data;
    toMap(data);
    other->fromMap(data);
    return other;
}

AnalyzerRunConfigurationAspect::AnalyzerRunConfigurationAspect(ISettingsAspect *projectSettings,
    ISettingsAspect *globalSettings)
{
    m_useGlobalSettings = true;
    m_projectSettings = projectSettings;
    m_globalSettings = globalSettings;
}

ISettingsAspect *AnalyzerRunConfigurationAspect::currentSettings() const
{
   return m_useGlobalSettings ? m_globalSettings : m_projectSettings;
}

AnalyzerRunConfigurationAspect::~AnalyzerRunConfigurationAspect()
{
    delete m_projectSettings;
}

void AnalyzerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_projectSettings->fromMap(map);
    m_useGlobalSettings = map.value(QLatin1String(useGlobalC), true).toBool();
}

void AnalyzerRunConfigurationAspect::toMap(QVariantMap &map) const
{
    m_projectSettings->toMap(map);
    map.insert(QLatin1String(useGlobalC), m_useGlobalSettings);
}

AnalyzerRunConfigurationAspect *AnalyzerRunConfigurationAspect::clone(
        ProjectExplorer::RunConfiguration *parent) const
{
    Q_UNUSED(parent)
    AnalyzerRunConfigurationAspect *other
            = new AnalyzerRunConfigurationAspect(m_projectSettings->clone(), m_globalSettings);
    other->m_useGlobalSettings = m_useGlobalSettings;
    return other;
}

void AnalyzerRunConfigurationAspect::setUsingGlobalSettings(bool value)
{
    m_useGlobalSettings = value;
}

void AnalyzerRunConfigurationAspect::resetCustomToGlobalSettings()
{
    ISettingsAspect *global = globalSettings();
    QTC_ASSERT(global, return);
    QVariantMap map;
    global->toMap(map);
    m_projectSettings->fromMap(map);
}

ProjectExplorer::RunConfigWidget *AnalyzerRunConfigurationAspect::createConfigurationWidget()
{
    return new AnalyzerRunConfigWidget(this);
}

} // namespace Analyzer
