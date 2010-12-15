/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SYMBOLGROUP_H
#define SYMBOLGROUP_H

#include "common.h"
#include "symbolgroupnode.h"

// Thin wrapper around a symbol group storing a tree of expanded symbols rooted on
// a fake "locals" root element.
// Provides a find() method based on inames ("locals.this.i1.data") that retrieves
// that index based on the current expansion state.

class SymbolGroup {
public:
    typedef std::vector<DEBUG_SYMBOL_PARAMETERS> SymbolParameterVector;

private:
    SymbolGroup(const SymbolGroup &);
    SymbolGroup &operator=(const SymbolGroup &);

    explicit SymbolGroup(CIDebugSymbolGroup *,
                         const SymbolParameterVector &vec,
                         ULONG threadId, unsigned frame);

public:
    typedef AbstractSymbolGroupNode::AbstractSymbolGroupNodePtrVector AbstractSymbolGroupNodePtrVector;

    static SymbolGroup *create(CIDebugControl *control,
                               CIDebugSymbols *,
                               ULONG threadId,
                               unsigned frame,
                               std::string *errorMessage);
    ~SymbolGroup();

    // Dump all
    std::string dump(const SymbolGroupValueContext &ctx,
                     const DumpParameters &p = DumpParameters()) const;
    // Expand node and dump
    std::string dump(const std::string &iname, const SymbolGroupValueContext &ctx,
                     const DumpParameters &p, std::string *errorMessage);
    std::string debug(const std::string &iname = std::string(), unsigned verbosity = 0) const;

    unsigned frame() const { return m_frame; }
    ULONG threadId() const { return m_threadId; }
    SymbolGroupNode *root() { return m_root; }
    const SymbolGroupNode *root() const { return m_root; }
    AbstractSymbolGroupNode *find(const std::string &iname) const;

    // Expand a single node "locals.A.B" requiring that "locals.A.B" is already visible
    // (think mkdir without -p).
    bool expand(const std::string &node, std::string *errorMessage);
    bool expandRunComplexDumpers(const std::string &node, const SymbolGroupValueContext &ctx, std::string *errorMessage);
    // Expand a node list "locals.i1,locals.i2", expanding all nested child nodes
    // (think mkdir -p).
    unsigned expandList(const std::vector<std::string> &nodes, std::string *errorMessage);
    unsigned expandListRunComplexDumpers(const std::vector<std::string> &nodes,
                                         const SymbolGroupValueContext &ctx,
                                         std::string *errorMessage);

    // Mark uninitialized (top level only)
    void markUninitialized(const std::vector<std::string> &nodes);

    // Cast an (unexpanded) node
    bool typeCast(const std::string &iname, const std::string &desiredType, std::string *errorMessage);
    // Add a symbol by name expression
    SymbolGroupNode *addSymbol(const std::string &name, // Expression like 'myarray[1]'
                               const std::string &iname, // Desired iname, defaults to name
                               std::string *errorMessage);

    bool accept(SymbolGroupNodeVisitor &visitor) const;

    // Assign a value by iname
    bool assign(const std::string &node,
                const std::string &value,
                std::string *errorMessage);

    CIDebugSymbolGroup *debugSymbolGroup() const { return m_symbolGroup; }

    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    unsigned long start,
                                    unsigned long count,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

private:
    inline AbstractSymbolGroupNode *findI(const std::string &iname) const;
    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

    CIDebugSymbolGroup * const m_symbolGroup;
    const unsigned m_frame;
    const ULONG m_threadId;
    SymbolGroupNode *m_root;
};

#endif // SYMBOLGROUP_H
