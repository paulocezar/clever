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

#ifndef CLEVER_VECTOR_H
#define CLEVER_VECTOR_H

#include <sstream>
#include "types/type.h"
#include "compiler/clever.h"
#include "compiler/value.h"
#include "compiler/scope.h"
#include "types/arrayvalue.h"
#include "types/arrayiterator.h"

#define CLEVER_RETURN_ARRAY(x) retval->setDataValue(new ArrayValue(x))
#define CLEVER_GET_ARRAY(x)    static_cast<ArrayValue*>((x)->getDataValue())->m_array
#define CLEVER_ARG_ARRAY(x)    CLEVER_GET_ARRAY(args->at((x)))
#define CLEVER_TPL_ARRAY(x)    CLEVER_GET_ARRAY_TEMPLATE->getTemplatedType((x))

namespace clever {

class Array : public TemplatedType {
public:
	Array()
		: TemplatedType(CSTRING("Array"), CLEVER_OBJECT) {
		addArg(NULL);
		
		Type* array_iter = new ArrayIterator;
		g_scope.pushType(CSTRING("ArrayIterator"), array_iter);
		array_iter->init();
	}

	Array(const CString* name, const Type* arg_type) :
		TemplatedType(name, CLEVER_OBJECT) {
			addArg(arg_type);
	}

 	const std::string* checkTemplateArgs(const TemplateArgs& args) const {
		if (args.size() != 1) {
			std::ostringstream oss;
			sprintf(oss, "Wrong number of template arguments given. "
				"`%S' requires 1 argument and %l was given.",
				this->getName(), args.size()
			);

			return new std::string(oss.str());
		}

		return NULL;
	}

	virtual const Type* getTemplatedType(const Type* type_arg) const {
		std::string name = getName()->str() + "<"
						   + type_arg->getName()->str() + ">";

		const CString* cname = CSTRING(name);
		const Type* type = g_scope.getType(cname);

		if (type == NULL) {
			Type* ntype = new Array(cname, type_arg);
			g_scope.pushType(cname, ntype);
			ntype->init();

			return ntype;
		}

		return type;
	}

	virtual const Type* getTemplatedType(const TemplateArgs& arg) const {
		return getTemplatedType(arg.at(0));
	}

	/**
	 * Virtual methods
	 */
	void init();
	DataValue* allocateValue() const;
	DataValue* copy(const Value*, bool) const;

	void destructor(Value* value) const {
		ValueVector* vec = CLEVER_GET_ARRAY(value);

		size_t sz = vec->size();
		for (size_t i = 0; i < sz; ++i) {
			vec->at(i)->delRef();
		}
	}

	/**
	 * Type methods
	 */
	static CLEVER_METHOD(push);
	static CLEVER_METHOD(pop);
	static CLEVER_METHOD(size);
	static CLEVER_METHOD(isEmpty);
	static CLEVER_METHOD(clear);
	static CLEVER_METHOD(at);
	static CLEVER_METHOD(set);
	static CLEVER_METHOD(resize);
	static CLEVER_METHOD(slice);
	static CLEVER_METHOD(toString);
	static CLEVER_METHOD(do_assign);
	static CLEVER_METHOD(find);
	static CLEVER_METHOD(do_copy);
	static CLEVER_METHOD(do_deepcopy);
	static CLEVER_METHOD(constructor);
	static CLEVER_METHOD(begin);
	static CLEVER_METHOD(end);
private:
	DISALLOW_COPY_AND_ASSIGN(Array);
};

} // clever

#endif // CLEVER_VECTOR_H
