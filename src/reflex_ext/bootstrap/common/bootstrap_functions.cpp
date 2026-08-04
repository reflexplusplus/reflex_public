#include "reflex_ext/bootstrap/common/functions.h"




//

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

constexpr Key32 kcommand = K32("command");

bool IsKey(const CString::View & value)
{
	return Left<true>(value, 2) == "--";
}

void SetValue(Data::PropertySet & args, Key32 key, CString::View token, bool typed, UInt8 flags)
{
	if (typed)
	{
		auto itr = token;

		if (itr && itr.GetFirst() == '-') itr = Nudge(itr);

		bool contains_period = false;

		auto number = Data::Detail::ExtractWhile<Data::Detail::IsNumericOrPeriod>(itr, contains_period);

		if (number && !itr)
		{
			if (contains_period)
			{
				if (flags & Data::kJsonFormatOptionFloat64)
				{
					SetFloat64(args, key, ToFloat64(token));
				}
				else
				{
					SetFloat32(args, key, ToFloat32(token));
				}
			}
			else if (flags & Data::kJsonFormatOptionInt64)
			{
				SetInt64(args, key, ToInt64(token));
			}
			else
			{
				SetInt32(args, key, ToInt32(token));
			}

			return;
		}
		else if (CaseInsensitive::eq(token, "true"))
		{
			SetBool(args, key, true);

			return;
		}
		else if (CaseInsensitive::eq(token, "false"))
		{
			SetBool(args, key, false);

			return;
		}
	}

	SetCString(args, key, token);
}

REFLEX_END_INTERNAL

Reflex::Data::PropertySet Reflex::Bootstrap::ParseCmdlineArgs(ArrayView <CString::View> cmdline, bool typed, UInt8 flags)
{
	Data::PropertySet args;

	auto keymap = Data::AcquireKeyMap(args);

	if (cmdline && !IsKey(cmdline.GetFirst()))
	{
		SetCString(args, kcommand, cmdline.GetFirst());

		cmdline = Mid(cmdline, 1);
	}

	CString::View key;

	Key32 value_key = Data::RegisterKey(keymap, "value");

	for (auto token : cmdline)
	{
		if (key)
		{
			if (IsKey(token))
			{
				SetBool(args, key, true);

				key = Mid(token, 2);

				Data::RegisterKey(keymap, key);

				continue;
			}

			SetValue(args, key, token, typed, flags);

			key = {};
		}
		else if (IsKey(token))
		{
			key = Mid(token, 2);

			Data::RegisterKey(keymap, key);
		}
		else
		{
			SetValue(args, value_key, token, typed, flags);

			value_key.value++;
		}
	}

	if (key) SetBool(args, key, true);

	return args;
}
