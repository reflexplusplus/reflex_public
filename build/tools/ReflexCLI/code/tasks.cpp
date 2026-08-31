#include "tasks.h"

#if defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_LINUX) || defined(REFLEX_OS_IOS)
#include <sys/stat.h>
#endif

Reflex::ArrayView <Reflex::CString> ReflexCLI::GetCStrings(const Data::PropertySet & properties, Key32 id)
{
	if (auto strings = properties.QueryProperty<Data::ArrayOfCStringProperty>(id))
	{
		return strings->value;
	}
	else if (auto string = properties.QueryProperty<Data::CStringProperty>(id))
	{
		return { &string->value, 1 };
	}

	return {};
}

Reflex::ArrayView <Reflex::Key32> ReflexCLI::GetKeys(const Data::PropertySet & node, Key32 id)
{
	if (auto ptr = node.QueryProperty<Data::ArrayOfKey32Property>(id))
	{
		return ptr->value;
	}
	else if (auto ptr = node.QueryProperty<Data::Key32Property>(id))
	{
		return ToView(ptr->value);
	}

	return {};
}

ReflexCLI::XmlWriter::XmlWriter()
{
	Data::EncodeUTF8(m_output, L"<?xml version=\"1.0\" encoding=\"utf-8\"?>");
	m_output.Push('\n');
}

ReflexCLI::XmlWriter::XmlWriter(CString::View name, CString::View public_id, CString::View system_id)
	: XmlWriter()
{
	Data::EncodeUTF8(m_output, ToWString(Join("<!DOCTYPE ", name, " PUBLIC \"", public_id, "\" \"", system_id, "\">")));
	m_output.Push('\n');
}

void ReflexCLI::XmlWriter::Element(CString::View name, CString::View text, bool allow_empty)
{
	if (allow_empty || text)
	{
		Indent();
		Data::EncodeUTF8(m_output, ToWString(Join("<", name, ">", EscapeXml(text), "</", name, ">")));
		m_output.Push('\n');
	}
}

void ReflexCLI::XmlWriter::WriteBoolElement(CString::View name, bool value, Optional<bool> default_value)
{
	if (!default_value || value != default_value.value) Element(name, Reflex::Detail::kFalseTrue[value]);
}

void ReflexCLI::XmlWriter::EmptyElement(CString::View name, ArrayView<XmlAttribute> attributes)
{
	Indent();
	Data::EncodeUTF8(m_output, ToWString(Join("<", name, Attributes(attributes), " />")));
	m_output.Push('\n');
}

Reflex::CString ReflexCLI::XmlWriter::Attributes(ArrayView<XmlAttribute> attributes)
{
	CString result;
	for (auto & attribute : attributes) result.Append(Join(" ", attribute.name, "=\"", EscapeXml(attribute.value), "\""));
	return result;
}

void ReflexCLI::XmlWriter::Indent()
{
	REFLEX_LOOP(idx, m_indent)
	{
		m_output.Push(' ');
		m_output.Push(' ');
	}
}

ReflexCLI::XmlScope::XmlScope(XmlWriter & writer, CString::View name, ArrayView<XmlAttribute> attributes)
	: m_writer(writer),
	m_name(name)
{
	m_writer.Indent();
	Data::EncodeUTF8(m_writer.m_output, ToWString(Join("<", m_name, m_writer.Attributes(attributes), ">")));
	m_writer.m_output.Push('\n');
	++m_writer.m_indent;
}

ReflexCLI::XmlScope::~XmlScope()
{
	REFLEX_ASSERT(m_writer.m_indent);
	--m_writer.m_indent;
	m_writer.Indent();
	Data::EncodeUTF8(m_writer.m_output, ToWString(Join("</", m_name, ">")));
	m_writer.m_output.Push('\n');
}

Reflex::CString ReflexCLI::EscapeXml(CString::View text)
{
	CString result;

	for (auto c : text)
	{
		switch (c)
		{
		case '&': result.Append("&amp;"); break;
		case '<': result.Append("&lt;"); break;
		case '>': result.Append("&gt;"); break;
		case '"': result.Append("&quot;"); break;
		default: result.Push(c); break;
		}
	}

	return result;
}

bool ReflexCLI::GetFilePermissions(WString::View path, UInt32 & permissions)
{
#if defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_LINUX) || defined(REFLEX_OS_IOS)
	auto utf8 = Data::EncodeUTF8(path);
	utf8.Push(0);

	struct stat info = {};
	if (stat(Reinterpret<char>(utf8.GetData()), &info) != 0) return false;

	permissions = UInt32(info.st_mode) & 0777;
#else
	permissions = 0;
#endif
	return true;
}

bool ReflexCLI::SetFilePermissions(WString::View path, UInt32 permissions)
{
#if defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_LINUX) || defined(REFLEX_OS_IOS)
	auto utf8 = Data::EncodeUTF8(path);
	utf8.Push(0);
	return chmod(Reinterpret<char>(utf8.GetData()), mode_t(permissions & 0777)) == 0;
#else
	return true;
#endif
}

REFLEX_BEGIN_INTERNAL(ReflexCLI)

template <class T> T & AllowTemp(T && t) { return t; }

WString StripValue(WString value, char allowed_dash)
{
	auto idx = value.GetSize();
	while (idx--)
	{
		auto w = value[idx];
		if (w < 255)
		{
			auto c = char(w);
			if (!(Data::Detail::IsAlphaNumericCharacter(c) || c == allowed_dash))
				value.Remove(idx);
		}
	}

	//TODO this needs to be an error type, but different one so consumer can discard as non-fatal, perhaps std CLI error needs a Key32 so it can be intercepted
	if (value.Empty()) File::output.Warn(ToCString(value), "strip resulted in empty string");

	return value;
}

bool ApplyExpressionFunction(Key32 op, const WString & value, WString & result, System::Platform platform)
{
	switch (op.value)
	{
	case K32("strip"):
		result = StripValue(value, '_');
		return true;

	case K32("package_identifier_segment"):
		result = Lowercase(StripValue(Replace(value, L'_', L'-'), '-'));
		return true;

	case K32("generate_4cc"):
	{
		auto bytes = Data::Pack(MakeKey32(value));
		UInt8 output[2] = { UInt8(bytes[0] ^ bytes[1]), UInt8(bytes[2] ^ bytes[3]) };
		result = ToWString(Data::BytesToHex({ output, 2 }));
		return true;
	}

	case K32("constant"):
		result = std::move(value);
		return true;

	case K32("lowercase"):
		result = Lowercase(value);
		return true;

	case K32("path"):
		result = PlatformPath(value, platform);
		return true;

	case K32("quote"):
		result = Join(L'"', value, L'"');
		return true;

	case K32("get_reflex_path"):
		result = File::RemoveTrailingStroke(AllowTemp(GetReflexPath()));
		return true;

	case K32("get_reflex_cli_path"):
		Require(!value, "get_reflex_cli_path", "input must be empty");
		result = GetReflexExecutablePath(GetReflexPath());
		return true;

	default: 
		return false;
	}
}

WString EvaluateVariableExpressionsImpl(WString::View source, ArrayView<Variable> variables, VariableSyntax syntax, ArrayView <Key32> deferred_variables, Array<CString> & stack, System::Platform platform);

WString ResolveVariable(const Variable & variable, ArrayView<Variable> variables, VariableSyntax syntax, ArrayView <Key32> deferred_variables, Array<CString> & stack, System::Platform platform)
{
	Require(!Search(stack, variable.name), "cyclic variable", variable.name);
	stack.Push(variable.name);
	auto result = EvaluateVariableExpressionsImpl(variable.value, variables, syntax, deferred_variables, stack, platform);
	stack.Pop();
	return result;
}

WString EvaluateVariableExpressionsImpl(WString::View source, ArrayView<Variable> variables, VariableSyntax syntax, ArrayView <Key32> deferred_variables, Array<CString> & stack, System::Platform platform)
{
	WString result = source;
	UInt cursor = 0;

	const Tuple <WString::View,char> kOpenClose[2] = { { L"_", L'_' }, { L"$(" , ')' } };

	auto open_close = kOpenClose[syntax == kVariableSyntaxProject];

	while (cursor < result.GetSize())
	{
		auto remaining = Mid(result, cursor);
		auto function_position = Search(remaining, L"$[");
		Idx variable_position = Search(remaining, open_close.a);

		if (!function_position && !variable_position) break;
		bool is_function = function_position && (!variable_position || function_position.value < variable_position.value);
		auto expression_start = cursor + (is_function ? function_position.value : variable_position.value);
		UInt expression_end = 0;

		WString replacement;

		if (is_function)
		{
			auto content_start = expression_start + 2;
			UInt end = content_start;
			UInt depth = 1;
			for (; end < result.GetSize(); ++end)
			{
				if (result[end] == '$' && end + 1 < result.GetSize() && result[end + 1] == '[') { ++depth; ++end; }
				else if (result[end] == ']' && !--depth) break;
			}
			Require(end < result.GetSize(), "unterminated function", source);

			auto content = Mid(result, content_start, end - content_start);
			auto separator = Search(content, ':');
			Require(True(separator), "function is missing ':'", source);
			auto name = Left(content, separator.value);
			Require(True(name), "function is missing operation", source);
			auto input = EvaluateVariableExpressionsImpl(Mid(content, separator.value + 1), variables, syntax, deferred_variables, stack, platform);

			WString pure_result;
			bool handled = ApplyExpressionFunction(MakeKey32(name), input, pure_result, platform);
			if (handled) replacement = pure_result;
			Require(handled, "unknown function", source);
			expression_end = end + 1;
		}
		else
		{
			UInt name_start = expression_start + open_close.a.size;
			auto end = Search(Mid(result, name_start), open_close.b);
			if (syntax == kVariableSyntaxTemplate && (!end || !end.value))
			{
				cursor = expression_start + 1;
				continue;
			}
			Require(True(end), "unterminated variable reference", source);
			expression_end = name_start + end.value + 1;
			auto name = ToCString(Mid(result, name_start, end.value));	//can assume variable name is ascii

			bool handled = false;
			if (auto variable = FindVariable(variables, name))
			{
				replacement = ResolveVariable(*variable, variables, syntax, deferred_variables, stack, platform);
				handled = true;
			}
			else if (Search(deferred_variables, MakeKey32(name)))
			{
				replacement = ToWString(VariableReference(name, syntax));
				handled = true;
			}
			Require(handled, "unknown variable", source);
		}

		result = Join(Left(result, expression_start), replacement, Mid(result, expression_end));
		cursor = expression_start + replacement.GetSize();
	}

	return result;
}

REFLEX_END_INTERNAL

Reflex::WString ReflexCLI::PlatformPath(const WString & path, System::Platform platform)
{
	return Replace(path, File::kStroke, platform == System::kPlatformWindows ? L'\\' : File::kStroke);
}

Reflex::WString ReflexCLI::ResolveAbsolutePathCase(WString::View path)
{
	REFLEX_ASSERT(!Search(path, L'\\'));	//must be pre-corrected, it expects a standard Reflex path

	if (System::IsAbsolutePath(path))
	{
		if (auto parts = Split(path, File::kStroke))
		{
			WString result;

			auto volumes = Make<System::DiskIterator>();

			bool removable;
			WString volume, display;

			while (volumes->GetNext(removable, volume, display))
			{
				auto root = File::RemoveTrailingStroke(volume);

				if (CaseInsensitive::eq(root, parts.GetFirst()))
				{
					result.Append(root);
					result.Push(File::kStroke);

					goto FoundDrive;
				}
			}

			return path;

			REFLEX_MARKER(FoundDrive);

			for (auto & i : Mid(parts, 1))
			{
				if (i)
				{
					auto directory = Make<System::DirectoryIterator>(result, true);

					System::DirectoryIterator::Item item;

					while (directory->GetNext(item))
					{
						if (CaseInsensitive::eq(item.filename, i))
						{
							result.Append(item.filename);
							if (item.is_directory) result.Push(File::kStroke);

							goto Next;
						}
					}

					return path;

					REFLEX_MARKER(Next);
				}
			}

			return result;
		}
	}

	return path;
}

bool ReflexCLI::SaveGeneratedFile(const WString & path, Data::Archive::View data)
{
	auto handle = Make<System::FileHandle>(path, System::FileHandle::kModeOverwrite);

	bool written = handle->Write(data.data, data.size) == data.size;
	bool flushed = handle->Flush(true);

	return written && flushed;
}

Reflex::CString ReflexCLI::VariableReference(CString::View name, VariableSyntax syntax)
{
	return syntax == kVariableSyntaxTemplate ? Join("_", name, "_") : Join("$(", name, ")");
}

const ReflexCLI::Variable * ReflexCLI::FindVariable(ArrayView<Variable> variables, CString::View name)
{
	return SearchValue<FieldCompare<&Variable::name>>(variables, name);
}

bool ReflexCLI::IsVariableName(CString::View name)
{
	if (name && Data::Detail::IsAlphaCharacter(name.GetFirst()))
	{
		for (auto c : Mid(name, 1))
		{
			if (!Data::Detail::IsAlphaNumericCharacter(c)) return false;
		}

		return true;
	}

	return false;
}

void ReflexCLI::SetVariable(Array<Variable> & variables, CString::View name, WString value)
{
	if (auto existing = FindVariable(variables, name))
	{
		RemoveConst(existing)->value = std::move(value);
	}
	else
	{
		variables.Push({ CString(name), std::move(value) });
	}
}

Reflex::Array<ReflexCLI::Variable> ReflexCLI::DecodeVariables(const Data::PropertySet & values, Key32 property, const Data::KeyMap & keymap, ArrayView<Variable> inherited_variables)
{
	Array<Variable> result = inherited_variables;
	if (auto node = Data::GetPropertySet(values, property))
	{
		for (auto & [address, value] : node->Iterate())
		{
			if (address.type_id == GetTypeID<Data::KeyMap>()) continue;
			auto name = Data::GetKey(keymap, address.id);
			Require(address.type_id == GetTypeID<Data::CStringProperty>(), name, "expected a string");
			SetVariable(result, name, ToWString(Cast<Data::CStringProperty>(value)->value));
		}
	}
	return result;
}

Reflex::Array<ReflexCLI::Variable> ReflexCLI::GetPersistentVariables()
{
	Array<Variable> result;
	if (auto stored = Data::GetPropertySet(Bootstrap::global->prefs, kPersistentVariables))
	{
		auto keymap = Data::GetKeyMap(stored);
		for (auto & [address, value] : stored->Iterate())
		{
			if (address.type_id != GetTypeID<Data::KeyMap>())
			{
				auto name = Data::GetKey(keymap, address.id);
				if (address.type_id == GetTypeID<Data::CStringProperty>())
				{
					SetVariable(result, name, ToWString(Cast<Data::CStringProperty>(value)->value));
				}
				else if (address.type_id == GetTypeID<Data::BoolProperty>())
				{
					SetVariable(result, name, ToWString(Reflex::Detail::kFalseTrue[Cast<Data::BoolProperty>(value)->value]));
				}
			}
		}
	}
	return result;
}

Reflex::WString ReflexCLI::EvaluateVariableExpressions(WString::View source, ArrayView<Variable> variables, VariableSyntax syntax, ArrayView <Key32> deferred_variables, System::Platform platform)
{
	Array <CString> stack;
	return EvaluateVariableExpressionsImpl(source, variables, syntax, deferred_variables, stack, platform);
}

Reflex::Data::PropertySet ReflexCLI::OpenTemplateCfg(WString::View template_folder)
{
	auto decode = [](const WString & path)
	{
		Require(File::Exists(path), "template config does not exist", path);
		auto values = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(path));
		if (auto error = Data::GetError(values)) ThrowError(ToCString(error.value.a), error.value.c);
		return values;
	};

	auto install_path = Join(template_folder, L"install.cfg");
	auto source = decode(install_path);
	Data::PropertySet config;
	auto assimilate = [&config](const Data::PropertySet & values)
	{
		Data::Assimilate(Data::AcquireKeyMap(config), Data::GetKeyMap(values));
		for (auto & item : values.Iterate())
		{
			if (item.key.type_id == GetTypeID<Data::KeyMap>()) continue;
			config.SetProperty(item.key, item.value);
		}
	};
	for (auto & include : GetCStrings(source, kInclude))
	{
		auto path = File::ResolveIncludePath(template_folder, File::CorrectStrokes(ToWString(include)));
		auto inherited = decode(path);
		assimilate(inherited);
	}
	assimilate(source);

	auto project_path = Join(template_folder, L"project.cfg");
	Require(File::Exists(project_path), "template config does not exist", project_path);

	Data::SetWString(config, "folder", template_folder);

	return config;
}

void ReflexCLI::EncodeTemplate(const TemplateDefinition & tmpl, Data::PropertySet & config)
{
	REFLEX_ASSERT(tmpl.folder);
	Data::SetWString(config, "folder", tmpl.folder);
	Data::SetCString(config, "name", tmpl.name);
	Data::SetCString(config, "description", Data::Unpack<CString::View>(tmpl.description_utf8));
	Data::SetCStringArray(config, "platforms", tmpl.platforms);
	auto input = Data::AcquirePropertySet(config, "input");
	const Pair <const Array<TokenDefinition> &, Key32> groups[] = { { tmpl.paths, K32("paths") }, { tmpl.strings, K32("strings") } };
	for (auto & [src, id] : groups)
	{
		auto values = Data::AcquirePropertySetArray(input, id);
		for (auto & token : src)
		{
			auto value = Data::AddPropertySet(values);
			if (token.id) Data::SetCString(value, "id", token.id);
			Data::SetCString(value, "name", token.name);
			Data::SetCString(value, "token", token.token);
		}
	}
}
