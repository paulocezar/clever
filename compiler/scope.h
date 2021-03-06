/**
 * Clever programming language
 * Copyright (c) 2011-2012 Clever Team
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CLEVER_SYMBOLTABLE_H
#define CLEVER_SYMBOLTABLE_H

#ifdef CLEVER_MSVC
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
#include <vector>
#include "compiler/clever.h"
#include "compiler/cstring.h"
#include "compiler/refcounted.h"

#include "symbol.h"

namespace clever {

class Type;
class Value;
class Scope;

typedef std::tr1::unordered_map<const CString*, Symbol*> SymbolMap;
typedef SymbolMap::value_type ScopeEntry;
typedef std::vector<Scope*> ScopeVector;

class Scope {
public:
	// Constructor for non-global scopes
	explicit Scope(Scope* parent)
		: m_parent(parent), m_children(), m_symbols() {
		clever_assert_not_null(parent);
	}

	// Global scope constructor
	Scope() : m_parent(NULL), m_children(), m_symbols() {}

	~Scope() { clear(); }

	// Binds a Value* to an interned string.
	void pushValue(const CString* name, Value* value) {
		clever_assert_not_null(name);
		clever_assert_not_null(value);

		m_symbols.insert(ScopeEntry(name, new Symbol(name, value)));
	}

	// Binds a Type* to an interned string.
	void pushType(const CString* name, const Type* type) {
		clever_assert_not_null(name);
		clever_assert_not_null(type);

		m_symbols.insert(ScopeEntry(name, new Symbol(name, type)));
	}

	// Resolve a symbol name locally
	Symbol* getLocalSym(const CString* name);

	// Resolve a symbol name recursively
	Symbol* getSym(const CString* name);

	// Resolve a value-symbol name locally
	Value* getLocalValue(const CString* name);

	// Resolve a value-symbol name recursively
	Value* getValue(const CString* name);

	// Resolve a type-symbol name recursively
	const Type* getType(const CString* name);

	Scope* newChild() {
		Scope* s = new Scope(this);
		m_children.push_back(s);
		return s;
	}

	// This the first scope of top-level functions and classes.
	Scope* newOrphanedChild() {
		Scope *s = new Scope(this);
		s->m_orphaned = true;
		m_orphanedChildren.push_back(s);

		return s;
	}

	bool isOrphaned() const {
		return m_orphaned;
	}

	bool hasChildren() const { return m_children.size() != 0; }

	void clear();

	Scope* getParent() const { return m_parent; }
	ScopeVector& getChildren() { return m_children; }

	bool isGlobal() const { return m_parent == NULL; }

	SymbolMap& getSymbols() { return m_symbols; }

private:
	DISALLOW_COPY_AND_ASSIGN(Scope);

	Scope* m_parent;
	ScopeVector m_children;
	ScopeVector m_orphanedChildren;
	SymbolMap m_symbols;
	bool m_orphaned;
};

extern Scope g_scope;

} // clever

#include "scope-inl.h"

#endif // CLEVER_SYMBOLTABLE_H
