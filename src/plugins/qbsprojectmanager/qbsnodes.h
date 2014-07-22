/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef QBSNODES_H
#define QBSNODES_H

#include <projectexplorer/projectnodes.h>

#include <qbs.h>

#include <QIcon>

namespace QbsProjectManager {
namespace Internal {

class FileTreeNode;
class QbsProject;
class QbsProjectFile;

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

class QbsFileNode : public ProjectExplorer::FileNode
{
    Q_OBJECT
public:
    QbsFileNode(const QString &filePath, const ProjectExplorer::FileType fileType, bool generated,
                int line);

    QString displayName() const;
};

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

class QbsGroupNode;

class QbsBaseProjectNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

public:
    explicit QbsBaseProjectNode(const QString &path);

    bool showInSimpleTree() const;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;

    bool canAddSubProject(const QString &proFilePath) const;

    bool addSubProjects(const QStringList &proFilePaths);

    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    bool deleteFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

private:
    friend class QbsGroupNode;
};

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

class QbsGroupNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    QbsGroupNode(const qbs::GroupData *grp, const QString &productPath);

    bool isEnabled() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    void updateQbsGroupData(const qbs::GroupData *grp, const QString &productPath,
                            bool productWasEnabled, bool productIsEnabled);

    const qbs::GroupData *qbsGroupData() const { return m_qbsGroupData; }

    QString productPath() const;

    static void setupFiles(QbsBaseProjectNode *root, const QStringList &files,
                           const QString &productPath, bool updateExisting);

private:
    static void setupFolder(ProjectExplorer::FolderNode *folder,
                            const FileTreeNode *subFileTree, const QString &baseDir, bool updateExisting);
    const qbs::GroupData *m_qbsGroupData;
    QString m_productPath;

    static QIcon m_groupIcon;
};

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

class QbsProductNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    explicit QbsProductNode(const qbs::ProductData &prd);

    bool isEnabled() const;
    bool showInSimpleTree() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);

    void setQbsProductData(const qbs::ProductData prd);
    const qbs::ProductData qbsProductData() const { return m_qbsProductData; }

    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const;

private:
    QbsGroupNode *findGroupNode(const QString &name);

    qbs::ProductData m_qbsProductData;
    static QIcon m_productIcon;
};

// ---------------------------------------------------------------------------
// QbsProjectNode:
// ---------------------------------------------------------------------------

class QbsProjectNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    explicit QbsProjectNode(const QString &path);
    ~QbsProjectNode();

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);

    virtual QbsProject *project() const;
    const qbs::Project qbsProject() const;
    const qbs::ProjectData qbsProjectData() const;

    bool showInSimpleTree() const;

protected:
    void update(const qbs::ProjectData &prjData);

private:
    void ctor();

    QbsProductNode *findProductNode(const QString &name);
    QbsProjectNode *findProjectNode(const QString &name);

    static QIcon m_projectIcon;
};

class QbsRootProjectNode : public QbsProjectNode
{
    Q_OBJECT

public:
    explicit QbsRootProjectNode(QbsProject *project);

    using QbsProjectNode::update;
    void update();

    QbsProject *project() const { return m_project; }

private:
    QbsProject * const m_project;
};


} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSNODES_H
