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

#include "symbolgroup.h"
#include "stringutils.h"

#include <set>
#include <algorithm>
#include <iterator>

typedef std::vector<int>::size_type VectorIndexType;
typedef std::vector<std::string> StringVector;

enum { debug = 0 };
const char rootNameC[] = "local";

// ------------- SymbolGroup
SymbolGroup::SymbolGroup(IDebugSymbolGroup2 *sg,
                         const SymbolParameterVector &vec,
                         ULONG threadId,
                         unsigned frame) :
    m_symbolGroup(sg),
    m_threadId(threadId),
    m_frame(frame),
    m_root(0)
{
    m_root = SymbolGroupNode::create(this, rootNameC, vec);
}

SymbolGroup::~SymbolGroup()
{
    m_symbolGroup->Release();
    delete m_root;
}

static inline bool getSymbolCount(CIDebugSymbolGroup *symbolGroup,
                                  ULONG *count,
                                  std::string *errorMessage)
{
    const HRESULT hr = symbolGroup->GetNumberSymbols(count);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetNumberSymbols", hr);
        return false;
    }
    return true;
}

bool SymbolGroup::getSymbolParameters(CIDebugSymbolGroup *symbolGroup,
                                      SymbolParameterVector *vec,
                                      std::string *errorMessage)
{
    ULONG count;
    return getSymbolCount(symbolGroup, &count, errorMessage)
            && getSymbolParameters(symbolGroup, 0, count, vec, errorMessage);
}

bool SymbolGroup::getSymbolParameters(CIDebugSymbolGroup *symbolGroup,
                                      unsigned long start,
                                      unsigned long count,
                                      SymbolParameterVector *vec,
                                      std::string *errorMessage)
{
    if (!count) {
        vec->clear();
        return true;
    }
    // Trim the count to the maximum count available. When expanding elements
    // and passing SubElements as count, SubElements might be an estimate that
    // is too large and triggers errors.
    ULONG totalCount;
    if (!getSymbolCount(symbolGroup, &totalCount, errorMessage))
        return false;
    if (start >= totalCount) {
        std::ostringstream str;
        str << "SymbolGroup::getSymbolParameters: Start parameter "
            << start << " beyond total " << totalCount << '.';
        *errorMessage = str.str();
        return  false;
    }
    if (start + count > totalCount)
        count = totalCount - start;
    // Get parameters.
    vec->resize(count);
    const HRESULT hr = symbolGroup->GetSymbolParameters(start, count, &(*vec->begin()));
    if (FAILED(hr)) {
        std::ostringstream str;
        str << "SymbolGroup::getSymbolParameters failed for index=" << start << ", count=" << count
            << ": " << msgDebugEngineComFailed("GetSymbolParameters", hr);
        *errorMessage = str.str();
        return false;
    }
    return true;
}

SymbolGroup *SymbolGroup::create(CIDebugControl *control, CIDebugSymbols *debugSymbols,
                                 ULONG threadId, unsigned frame,
                                 std::string *errorMessage)
{
    errorMessage->clear();

    ULONG obtainedFrameCount = 0;
    const ULONG frameCount = frame + 1;

    DEBUG_STACK_FRAME *frames = new DEBUG_STACK_FRAME[frameCount];
    IDebugSymbolGroup2 *idebugSymbols = 0;
    bool success = false;
    SymbolParameterVector parameters;

    // Obtain symbol group at stack frame.
    do {
        HRESULT hr = control->GetStackTrace(0, 0, 0, frames, frameCount, &obtainedFrameCount);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetStackTrace", hr);
            break;
        }
        if (obtainedFrameCount < frameCount ) {
            std::ostringstream str;
            str << "Unable to obtain frame " << frame << " (" << obtainedFrameCount << ").";
            *errorMessage = str.str();
            break;
        }
        hr = debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, NULL, &idebugSymbols);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetScopeSymbolGroup2", hr);
            break;
        }
        hr = debugSymbols->SetScope(0, frames + frame, NULL, 0);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("SetScope", hr);
            break;
        }
        // refresh with current frame
        hr = debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, idebugSymbols, &idebugSymbols);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetScopeSymbolGroup2", hr);
            break;
        }
        if (!SymbolGroup::getSymbolParameters(idebugSymbols, &parameters, errorMessage))
            break;

        success = true;
    } while (false);
    delete [] frames;
    if (!success) {
        if (idebugSymbols)
            idebugSymbols->Release();
        return 0;
    }
    return new SymbolGroup(idebugSymbols, parameters, threadId, frame);
}

static inline std::string msgNotFound(const std::string &nodeName)
{
    std::ostringstream str;
    str << "Node '" << nodeName << "' not found.";
    return str.str();
}

std::string SymbolGroup::dump(const SymbolGroupValueContext &ctx,
                              const DumpParameters &p) const
{
    std::ostringstream str;
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
    if (p.humanReadable())
        str << '\n';
    str << '[';
    accept(visitor);
    str << ']';
    return str.str();
}

// Dump a node, potentially expand
std::string SymbolGroup::dump(const std::string &iname,
                              const SymbolGroupValueContext &ctx,
                              const DumpParameters &p,
                              std::string *errorMessage)
{
    AbstractSymbolGroupNode *const aNode = find(iname);
    if (!aNode) {
        *errorMessage = msgNotFound(iname);
        return std::string();
    }

    // Real nodes: Expand and complex dumpers
    if (SymbolGroupNode *node = aNode->asSymbolGroupNode()) {
        if (node->isExpanded()) { // Mark expand request by watch model
            node->clearFlags(SymbolGroupNode::ExpandedByDumper);
        } else {
            if (node->canExpand() && !node->expand(errorMessage))
                return false;
        }
        // After expansion, run the complex dumpers
        if (p.dumpFlags & DumpParameters::DumpComplexDumpers)
            node->runComplexDumpers(ctx);
    }

    std::ostringstream str;
    if (p.humanReadable())
        str << '\n';
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
    str << '[';
    aNode->accept(visitor, SymbolGroupNodeVisitor::parentIname(iname), 0, 0);
    str << ']';
    return str.str();
}

std::string SymbolGroup::debug(const std::string &iname, unsigned verbosity) const
{
    std::ostringstream str;
    str << '\n';
    DebugSymbolGroupNodeVisitor visitor(str, verbosity);
    if (iname.empty()) {
        accept(visitor);
    } else {
        if (AbstractSymbolGroupNode *const node = find(iname)) {
            node->accept(visitor, SymbolGroupNodeVisitor::parentIname(iname), 0, 0);
        } else {
            str << msgNotFound(iname);
        }
    }
    return str.str();
}

/* expandList: Expand a list of inames with a 'mkdir -p'-like behaviour, that is,
 * expand all sub-paths. The list of inames has thus to be reordered to expand the
 * parent items first, for example "locals.this.i1.data,locals.this.i2" --->:
 * "locals, locals.this, locals.this.i1, locals.this.i2, locals.this.i1.data".
 * This is done here by creating a set of name parts keyed by level and name
 * (thus purging duplicates). */

typedef std::pair<unsigned, std::string> InamePathEntry;

struct InamePathEntryLessThan : public std::binary_function<InamePathEntry, InamePathEntry, bool> {
    bool operator()(const InamePathEntry &i1, const InamePathEntry& i2) const
    {
        if (i1.first < i2.first)
            return true;
        if (i1.first != i2.first)
            return false;
        return i1.second < i2.second;
    }
};

typedef std::set<InamePathEntry, InamePathEntryLessThan> InamePathEntrySet;

static inline InamePathEntrySet expandEntrySet(const std::vector<std::string> &nodes)
{
    InamePathEntrySet pathEntries;
    const VectorIndexType nodeCount = nodes.size();
    for (VectorIndexType i= 0; i < nodeCount; i++) {
        const std::string &iname = nodes.at(i);
        std::string::size_type pos = 0;
        // Split a path 'local.foo' and insert (0,'local'), (1,'local.foo') (see above)
        for (unsigned level = 0; pos < iname.size(); level++) {
            std::string::size_type dotPos = iname.find(SymbolGroupNodeVisitor::iNamePathSeparator, pos);
            if (dotPos == std::string::npos)
                dotPos = iname.size();
            pathEntries.insert(InamePathEntry(level, iname.substr(0, dotPos)));
            pos = dotPos + 1;
        }
    }
    return pathEntries;
}

// Expand a node list "locals.i1,locals.i2"
unsigned SymbolGroup::expandList(const std::vector<std::string> &nodes, std::string *errorMessage)
{
    if (nodes.empty())
        return 0;
    // Create a set with a key <level, name>. Also required for 1 node (see above).
    const InamePathEntrySet pathEntries = expandEntrySet(nodes);
    // Now expand going by level.
    unsigned succeeded = 0;
    std::string nodeError;
    InamePathEntrySet::const_iterator cend = pathEntries.end();
    for (InamePathEntrySet::const_iterator it = pathEntries.begin(); it != cend; ++it)
        if (expand(it->second, &nodeError)) {
            succeeded++;
        } else {
            if (!errorMessage->empty())
                errorMessage->append(", ");
            errorMessage->append(nodeError);
        }
    return succeeded;
}

unsigned SymbolGroup::expandListRunComplexDumpers(const std::vector<std::string> &nodes,
                                     const SymbolGroupValueContext &ctx,
                                     std::string *errorMessage)
{
    if (nodes.empty())
        return 0;
    // Create a set with a key <level, name>. Also required for 1 node (see above).
    const InamePathEntrySet pathEntries = expandEntrySet(nodes);
    // Now expand going by level.
    unsigned succeeded = 0;
    std::string nodeError;
    InamePathEntrySet::const_iterator cend = pathEntries.end();
    for (InamePathEntrySet::const_iterator it = pathEntries.begin(); it != cend; ++it)
        if (expandRunComplexDumpers(it->second, ctx, &nodeError)) {
            succeeded++;
        } else {
            if (!errorMessage->empty())
                errorMessage->append(", ");
            errorMessage->append(nodeError);
        }
    return succeeded;
}

// Find a node for expansion, skipping reference nodes.
static inline SymbolGroupNode *
    findNodeForExpansion(const SymbolGroup *sg,
                         const std::string &nodeName,
                         std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = sg->find(nodeName);
    if (!aNode) {
        *errorMessage = msgNotFound(nodeName);
        return 0;
    }

    SymbolGroupNode *node = aNode->resolveReference()->asSymbolGroupNode();
    if (!node) {
        *errorMessage = "Node type error in expand: " + nodeName;
        return 0;
    }
    return node;
}

bool SymbolGroup::expand(const std::string &nodeName, std::string *errorMessage)
{
    if (SymbolGroupNode *node = findNodeForExpansion(this, nodeName, errorMessage))
        return node == m_root ? true : node->expand(errorMessage);
    return false;
}

bool SymbolGroup::expandRunComplexDumpers(const std::string &nodeName, const SymbolGroupValueContext &ctx, std::string *errorMessage)
{
    if (SymbolGroupNode *node = findNodeForExpansion(this, nodeName, errorMessage))
        return node == m_root ? true : node->expandRunComplexDumpers(ctx, errorMessage);
    return false;
}

// Cast an (unexpanded) node
bool SymbolGroup::typeCast(const std::string &iname, const std::string &desiredType, std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = find(iname);
    if (!aNode) {
        *errorMessage = msgNotFound(iname);
        return false;
    }
    if (aNode == m_root) {
        *errorMessage = "Cannot cast root node";
        return false;
    }
    SymbolGroupNode *node = aNode->asSymbolGroupNode();
    if (!node) {
        *errorMessage = "Node type error in typeCast: " + iname;
        return false;
    }
    return node->typeCast(desiredType, errorMessage);
}

SymbolGroupNode *SymbolGroup::addSymbol(const std::string &name, const std::string &iname, std::string *errorMessage)
{
    return m_root->addSymbolByName(name, iname, errorMessage);
}

// Mark uninitialized (top level only)
void SymbolGroup::markUninitialized(const std::vector<std::string> &uniniNodes)
{
    if (m_root && !m_root->children().empty() && !uniniNodes.empty()) {
        const std::vector<std::string>::const_iterator unIniNodesBegin = uniniNodes.begin();
        const std::vector<std::string>::const_iterator unIniNodesEnd = uniniNodes.end();

        const AbstractSymbolGroupNodePtrVector::const_iterator childrenEnd = m_root->children().end();
        for (AbstractSymbolGroupNodePtrVector::const_iterator it = m_root->children().begin(); it != childrenEnd; ++it) {
            if (std::find(unIniNodesBegin, unIniNodesEnd, (*it)->absoluteFullIName()) != unIniNodesEnd)
                (*it)->addFlags(SymbolGroupNode::Uninitialized);
        }
    }
}

static inline std::string msgAssignError(const std::string &nodeName,
                                         const std::string &value,
                                         const std::string &why)
{
    std::ostringstream str;
    str << "Unable to assign '" << value << "' to '" << nodeName << "': " << why;
    return str.str();
}

bool SymbolGroup::assign(const std::string &nodeName, const std::string &value,
                         std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = find(nodeName);
    if (aNode == 0) {
        *errorMessage = msgAssignError(nodeName, value, "No such node");
        return false;
    }
    SymbolGroupNode *node = aNode->asSymbolGroupNode();
    if (node == 0) {
        *errorMessage = msgAssignError(nodeName, value, "Invalid node type");
        return false;
    }

    const HRESULT hr = m_symbolGroup->WriteSymbol(node->index(), const_cast<char *>(value.c_str()));
    if (FAILED(hr)) {
        *errorMessage = msgAssignError(nodeName, value, msgDebugEngineComFailed("WriteSymbol", hr));
        return false;
    }
    return true;
}

bool SymbolGroup::accept(SymbolGroupNodeVisitor &visitor) const
{
    if (!m_root || m_root->children().empty())
        return false;
    return m_root->accept(visitor, std::string(), 0, 0);
}

// Find  "locals.this.i1" and move index recursively
static AbstractSymbolGroupNode *findNodeRecursion(const std::vector<std::string> &iname,
                                                  unsigned depth,
                                                  std::vector<AbstractSymbolGroupNode *> nodes)
{
    typedef std::vector<AbstractSymbolGroupNode *>::const_iterator ConstIt;

    if (debug > 1) {
        DebugPrint() <<"findNodeRecursion: Looking for " << iname.back() << " (" << iname.size()
           << "),depth="  << depth << ",matching=" << iname.at(depth) << " in " << nodes.size();
    }

    if (nodes.empty())
        return 0;
    // Find the child that matches the iname part at depth
    const ConstIt cend = nodes.end();
    for (ConstIt it = nodes.begin(); it != cend; ++it) {
        AbstractSymbolGroupNode *c = *it;
        if (c->iName() == iname.at(depth)) {
            if (depth == iname.size() - 1) { // Complete iname matched->happy.
                return c;
            } else {
                // Sub-part of iname matched. Forward index and check children.
                return findNodeRecursion(iname, depth + 1, c->children());
            }
        }
    }
    return 0;
}

AbstractSymbolGroupNode *SymbolGroup::findI(const std::string &iname) const
{
    if (iname.empty())
        return 0;
    // Match the root element only: Shouldn't happen, still, all happy
    if (iname == m_root->name())
        return m_root;

    std::vector<std::string> inameTokens;
    split(iname, SymbolGroupNodeVisitor::iNamePathSeparator, std::back_inserter(inameTokens));

    // Must begin with root
    if (inameTokens.front() != m_root->name())
        return 0;

    // Start with index = 0 at root's children
    return findNodeRecursion(inameTokens, 1, m_root->children());
}

AbstractSymbolGroupNode *SymbolGroup::find(const std::string &iname) const
{
    AbstractSymbolGroupNode *rc = findI(iname);
    if (::debug > 1)
        DebugPrint() << "SymbolGroup::find " << iname << ' ' << rc;
    return rc;
}
