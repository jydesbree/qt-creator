/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOTOOLSMANAGER_H
#define AUTOTOOLSMANAGER_H

#include <projectexplorer/iprojectmanager.h>

namespace AutotoolsProjectManager {
namespace Internal {

/**
 * @brief Implementation of ProjectExplorer::IProjectManager interface.
 *
 * An autotools project is identified by the MIME type text/x-makefile.
 * The project is represented by an instance of ProjectExplorer::Project,
 * which gets created by AutotoolsManager::openProject().
 */
class AutotoolsManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    AutotoolsManager() {}

    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);
    QString mimeType() const;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSMANAGER_H