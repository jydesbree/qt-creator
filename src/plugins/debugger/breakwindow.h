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

#ifndef DEBUGGER_BREAKWINDOW_H
#define DEBUGGER_BREAKWINDOW_H

#include "breakhandler.h"
#include <utils/basetreeview.h>

namespace Debugger {
namespace Internal {

class BreakTreeView : public Utils::BaseTreeView
{
    Q_OBJECT

public:
    BreakTreeView();

    static void editBreakpoint(Breakpoint bp, QWidget *parent);

private:
    void rowActivated(const QModelIndex &index);
    void contextMenuEvent(QContextMenuEvent *ev);
    void keyPressEvent(QKeyEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *ev);

    void showAddressColumn(bool on);
    void deleteBreakpoints(const Breakpoints &bps);
    void deleteAllBreakpoints();
    void addBreakpoint();
    void editBreakpoints(const Breakpoints &bps);
    void associateBreakpoint(const Breakpoints &bps, int thread);
    void setBreakpointsEnabled(const Breakpoints &bps, bool enabled);
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_BREAKWINDOW
