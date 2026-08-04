#pragma once

#include "[require].h"




//
//

#define CONST_STRING(symbol, def) constexpr CString::View symbol = def;

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

CONST_STRING(kErrorPathNotFound, "path not found");
CONST_STRING(kErrorResourceHandlerNotFound, "resource handler not found");

CONST_STRING(kErrorTemplateUnknownSymbol, "unknown symbol '[0]'");
CONST_STRING(kErrorTemplateExpectedToken, "expected [0]");
CONST_STRING(kErrorInvalidFunctionDeclaration, "invalid function declaration");
CONST_STRING(kErrorTemplateInvalidKeyword, "'[0]' keyword is invalid here");
CONST_STRING(kErrorInvalidFunctionLocation, "function cannot be declared here");
CONST_STRING(kErrorInvalidNumberSuffix, "invalid number suffix");

CONST_STRING(kErrorExpectedPreprocessorCommand, "expected preprocessor command");
CONST_STRING(kErrorExpectedNewOrNull, "expected new or null");
CONST_STRING(kErrorExpectedBooleanExpression, "expected boolean expression");
CONST_STRING(kErrorExpectedConstant, "expected constant");
CONST_STRING(kErrorExpectedType, "expected type");
CONST_STRING(kErrorExpectedTemplateType, "expected template typename");
CONST_STRING(kErrorExpectedStatement, "expected statement");
CONST_STRING(kErrorExpectedNamespace, "expected namespace");
CONST_STRING(kErrorExpectedBrackets, "expected brackets");
CONST_STRING(kErrorExpectedSemiColon, "expected semicolon");
CONST_STRING(kErrorExpectedColon, "expected colon");
CONST_STRING(kErrorExpectedAssignmentOperator, "expected assignment operator");
CONST_STRING(kErrorExpectedReturn, "expected return");
CONST_STRING(kErrorExpectedBreak, "expected break");
CONST_STRING(kErrorExpectedName, "expected name");
CONST_STRING(kErrorExpectedMemberOrMethod, "expected member or method");
CONST_STRING(kErrorExpectedVariable, "expected variable");
CONST_STRING(kErrorExpectedAnonFunction, "expected anonymous function definition");
CONST_STRING(kErrorExpectedLocalVariable, "expected local variable");
CONST_STRING(kErrorNonAssignableValue, "non-assignable value");
CONST_STRING(kErrorInternalError, "internal error");

CONST_STRING(kErrorDuplicateSymbol, "duplicate symbol");

CONST_STRING(kErrorTypeMismatch, "type mismatch");
CONST_STRING(kErrorCanNotCast, "can not convert [0] to [1]");

CONST_STRING(kErrorNonNullableType, "non-nullable type");
CONST_STRING(kErrorMatchingConstructorNotFound, "matching constructor not found");
CONST_STRING(kErrorNonIterableType, "non-iterable type");

CONST_STRING(kErrorStructsCanNotContainObjects, "structs can not contain objects");
CONST_STRING(kErrorStructsCanNotHaveMethods, "structs can not have methods");
CONST_STRING(kErrorMemberIsNotMtCompatible, "[0] is not (mt) compatible");
CONST_STRING(kErrorInvalidSize, "struct too large");

CONST_STRING(kErrorCanNotAssignTemporary, "can not assign temporary");
CONST_STRING(kErrorCanNotPassTemporaryByRef, "can not pass temporary by reference");
CONST_STRING(kErrorCanNotDeduceType, "could not deduce type");

CONST_STRING(kErrorOperatorNotFound, "operator not found");

inline const CString::View kDefaultErrors[] = { "syntax error", kErrorTypeMismatch };

template <bool TYPE>
struct ErrorType
{
	ErrorType(const Source & src, const CString::View & error = kDefaultErrors[TYPE]) : src(src), error(error) { }

	void Throw() const
	{
		throw(*this);
	}

	static void Throw(const Source & src, const CString::View & error = kDefaultErrors[TYPE]) { ErrorType self(src, error); self.Throw(); }

	Source src;

	CString::View error;
};

typedef ErrorType <false> SyntaxError;

typedef ErrorType <true> TypeError;

template <class ERROR, class TYPE> REFLEX_INLINE static TYPE Assume(TYPE && action, const ERROR & error)
{
	if (!True(action)) error.Throw();

	return std::move(action);
}

REFLEX_END_INTERNAL
