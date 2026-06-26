#pragma once

#include "../defines.h"




//
//JsonTokeniser

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct JsonTokeniser : public Detail::Tokeniser
{
	struct ArrayImpl;

	struct ObjectImpl;

	struct State	//for whole sheet
	{
		State(KeyMap & keymap, UInt8 flags);

		const TRef <KeyMap> keymap;

		const ValueTypeHandler * token2handler[kNumValueType];
	};

	enum Expect
	{
		kExpectKey,
		kExpectString,
		kExpectValue,
		kExpectComma,
		kExpectColon,
	};

	JsonTokeniser(State & state, Expect init);

	Reference <Object> CreateValue(UInt line, ValueType type, const CString::View & word);

	virtual void OnComment(UInt line, const CString::View & comment) override {}

	CString::View ConsumeSign(const CString::View & word);


	State & state;

	Expect m_expect;

	const char * m_psign = 0;
};

struct JsonTokeniser::ArrayImpl : public JsonTokeniser
{
	ArrayImpl(State & state);

	virtual TRef <Tokeniser> OnPush(UInt line, const char & bracket) override;

	virtual void OnPop(UInt line, const char & bracket, Tokeniser & child) override;

	virtual void OnValue(UInt line, ValueType type, const CString::View & word) override;

	virtual void OnSymbol(UInt line, const char & symbol) override;

	Detail::PropertySheetInterface::ObjectWithType ConvertArray(UInt line);


	Array < Reference <PropertySet> > m_pending_propertysets;

	ValueType m_pending_values_type = kNumValueType;

	Array <CString::View> m_pending_values;
};

struct JsonTokeniser::ObjectImpl : public JsonTokeniser
{
	ObjectImpl(State & state, TRef <PropertySet> dynamic);

	void CommitValue();

	virtual TRef <Tokeniser> OnPush(UInt line, const char & bracket) override;

	virtual void OnPop(UInt line, const char & bracket, Detail::Tokeniser & child) override;

	virtual void OnValue(UInt line, ValueType type, const CString::View & word) override;

	virtual void OnSymbol(UInt line, const char & symbol) override;


	Reference <PropertySet> dynamic;

	Key32 m_key;

	Reference <Object> m_value;
};

REFLEX_END_INTERNAL
