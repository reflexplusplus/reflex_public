#pragma once

#include "common.h"




//
//declarations

namespace ReflexCLI
{
	struct Variable
	{
		CString name;
		WString value;
	};

	struct XmlAttribute
	{
		CString name;
		CString value;
	};

	class XmlWriter
	{
	public:
		XmlWriter();
		XmlWriter(CString::View name, CString::View public_id, CString::View system_id);
		void Element(CString::View name, CString::View text, bool allow_empty = false);
		void WriteBoolElement(CString::View name, bool value, Optional<bool> default_value = false);
		void EmptyElement(CString::View name, ArrayView<XmlAttribute> attributes = {});
		Data::Archive::View GetOutput() const { return m_output; }

	private:
		friend class XmlScope;
		Data::Archive m_output;
		UInt m_indent = 0;
		CString Attributes(ArrayView<XmlAttribute> attributes);
		void Indent();
	};

	class XmlScope
	{
	public:
		REFLEX_NONCOPYABLE(XmlScope);
		XmlScope(XmlWriter &, CString::View, ArrayView<XmlAttribute> = {});
		~XmlScope();

	private:
		XmlWriter & m_writer;
		CString m_name;
	};

	enum VariableSyntax : UInt8 { kVariableSyntaxTemplate, kVariableSyntaxProject };

	ArrayView <CString> GetCStrings(const Data::PropertySet & properties, Key32 id);
	ArrayView <Key32> GetKeys(const Data::PropertySet & node, Key32 id);

	CString VariableReference(CString::View name, VariableSyntax syntax);
	bool IsVariableName(CString::View name);
	const Variable * FindVariable(ArrayView<Variable> variables, CString::View name);
	void SetVariable(Array<Variable> & variables, CString::View name, WString value);
	Array<Variable> DecodeVariables(const Data::PropertySet & values, Key32 property, const Data::KeyMap & keymap, ArrayView<Variable> inherited_variables = {});
	Array<Variable> GetPersistentVariables();
	WString EvaluateVariableExpressions(WString::View source, ArrayView<Variable> variables, VariableSyntax syntax, ArrayView <Key32> deferred_variables, System::Platform platform = System::kNumPlatform);
	CString EscapeXml(CString::View text);
	WString PlatformPath(const WString & path, System::Platform platform);
	Data::PropertySet OpenTemplateCfg(WString::View template_folder);
	void EncodeTemplate(const TemplateDefinition & tmpl, Data::PropertySet & config);
	bool SaveGeneratedFile(const WString & path, Data::Archive::View data);
	bool GetFilePermissions(WString::View path, UInt32 & permissions);
	bool SetFilePermissions(WString::View path, UInt32 permissions);

	struct StringCompare
	{
		static bool eq(CString::View a, CString::View b);
	};


	void Install(CString::View version, const Array <CString::View> & platforms, const WString & path, bool test, System::FileHandle & std_out);

	void ListVersions(System::FileHandle & std_out);

	void GetVersion(System::FileHandle & std_out);

	void SetPath(const WString & path, System::FileHandle & std_out);

	void Doc(const Data::PropertySet & args, System::FileHandle & std_out);

	void DocHelp(System::FileHandle & std_out);


	enum BuildPlatform : UInt8
	{
		kBuildPlatformWindows,
		kBuildPlatformMacOS,
		kBuildPlatformIOS,
		kBuildPlatformAndroid,
		kBuildPlatformLinux,
		kBuildPlatformCMake
	};


	WString CreateProject(const TemplateDefinition & tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString::View> targets, const WString & destination, System::FileHandle & std_in, System::FileHandle & std_out, const Function <bool(const WString&)> & overwrite);

	void GenerateProject(const WString & path, ArrayView <CString::View> platforms_filter, System::FileHandle & std_out);


	void BuildResources(const WString::View & filename, Float & progress);
	
	void BuildPlist(const Data::PropertySet & args, System::FileHandle & std_out);


	extern Output output;


	inline const auto & kColourDim = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourBrightBlack];

	inline const auto & kColourDefault = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourDefault];


	inline void PrintCommandWithDescription(System::FileHandle & std_out, CString::View name, CString::View description)
	{
		File::WriteLine(std_out, Join(kColourDefault, name, ' ', kColourDim, description, kColourDefault));
	}


	constexpr Key32 kInclude = "include";

	constexpr Key32 kPersistentVariables = K32("variables");

	constexpr CString::View kBuildPlatforms[] = { "windows", "macos", "ios", "android", "linux", "cmake" };

}
