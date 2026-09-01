#include "tasks.h"
#include "project_gen_emitter.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen)

template <class ARRAY> void RequireUnique(const ARRAY & values, CString::View property_name)
{
	REFLEX_LOOP(index, values.GetSize())
	{
		for (UInt previous = 0; previous < index; ++previous)
			Require(values[previous] != values[index], property_name, "duplicate value");
	}
}

Array<PathDesc> GetPaths(const Data::PropertySet & values, CString::View name)
{
	Array<PathDesc> result;
	for (auto & value : GetCStrings(values, MakeKey32(name))) result.Push(DecodePath(value));
	return result;
}

REFLEX_NOINLINE Array<Variable> GetVariables(const ValidatedPropertySet & values, Key32 property, const Data::KeyMap & keymap)
{
	Array<const ValidatedPropertySet *> scopes;
	for (auto scope = &values; scope; scope = scope->parent ? scope->parent.Adr() : nullptr) scopes.Push(scope);

	Array<Variable> result;
	UInt index = scopes.GetSize();
	if (index)
	{
		if (scopes[index - 1]->type == kPropertySetTypeProject)
		{
			auto project = Cast<Project>(scopes[index - 1]);
			result = project->GetDocumentVariables(property);
			--index;
		}
	}
	for (; index; --index)
	{
		result = DecodeVariables(*scopes[index - 1], property, keymap, result);
	}
	if (property == kNullKey) for (auto & variable : GetPersistentVariables()) SetVariable(result, variable.name, variable.value);
	for (auto & variable : result) Require(IsVariableName(variable.name), "invalid variable name", variable.name);
	return result;
}

REFLEX_NOINLINE UInt ParseEnumImpl(const Data::PropertySet & node, CString::View property_name, ArrayView<CString::View> enum_strings, Key32 default_value)
{
	auto value = Data::GetKey32(node, MakeKey32(property_name), default_value);
	REFLEX_LOOP(idx, enum_strings.size) if (MakeKey32(enum_strings[idx]) == value.value) return idx;
	ThrowError(property_name, "invalid value");
	return {};
}

template <class ENUM> ENUM ParseEnum(const Data::PropertySet & node, CString::View property_name, ArrayView<CString::View> enum_strings, Key32 default_value)
{
	return ENUM(ParseEnumImpl(node, property_name, enum_strings, default_value));
}

CString GetDriverName(const ValidatedPropertySet & driver)
{
	auto result = Data::GetKey(*driver.keymap, driver.id);
	Require(True(result), "structural object", "names must not be empty");
	return result;
}

void ResolveTargetDependencies(Project & project)
{
	auto project_targets = project.GetTargets();
	for (auto & target : project_targets) target->ResolveDependencies();

	Array<Reference<Target>> visited_targets;
	Array<UInt8> states;
	auto acquire_state = [&visited_targets, &states](const Reference<Target> & target)
	{
		REFLEX_LOOP(index, visited_targets.GetSize()) if (visited_targets[index].Adr() == target.Adr()) return Idx(index);
		visited_targets.Push(target);
		states.Push(0);
		return Idx(states.GetSize() - 1);
	};
	auto validate_acyclic = [&states, &acquire_state](const Reference<Target> & target, auto && validate_acyclic) -> void
	{
		auto index = acquire_state(target).value;
		Require(states[index] != 1, "dependency cycle at", target->GetName());
		if (states[index] == 2) return;
		states[index] = 1;
		for (auto & dependency : target->GetDependencies()) validate_acyclic(dependency, validate_acyclic);
		target->ResolveIncludeDirectories();
		states[index] = 2;
	};
	for (auto & target : project_targets) validate_acyclic(target, validate_acyclic);
}

void DecodeProjectDocument(Project & project, Array<Reference<Project>> & projects, WString::View path, const Data::PropertySet & options)
{
	auto filename = File::ResolveRelativePath(File::CorrectStrokes(path));
	auto blob = File::Open(filename);
	Require(True(blob), "file not found", EncodeUTF8(filename));

	// Decode generically first to discover this document's ordered include graph.
	// Each referenced document retains its own root, keymap, source path, and targets.
	auto source = Data::DecodePropertySet(Data::kPropertySheetFormat, blob, options);
	if (auto error = Data::GetError(source)) ThrowError(ToCString(error.value.a), error.value.c);
	auto variables = DecodeVariables(source, kNullKey, Data::GetKeyMap(source));
	for (auto & variable : GetPersistentVariables()) SetVariable(variables, variable.name, variable.value);
	auto folder = File::SplitFilename(filename).a;

	auto AcquireIncludes = [&project, &projects, &variables, folder](ArrayView<CString> paths)
	{
		for (auto & path : paths)
		{
			auto expanded = EvaluateVariableExpressions(ToWString(path), variables, kVariableSyntaxProject, {});
			auto include_path = File::ResolveIncludePath(folder, File::CorrectStrokes(expanded));
			auto included = Project::Acquire(projects, include_path, project.keymap.Adr());
			project.AddInclude(std::move(included));
		}
	};
	AcquireIncludes(GetCStrings(source, kProjectInclude));
	AcquireIncludes(GetCStrings(source, kImport));

	g_project_format->Decode(project, File::Open(filename), options);
	if (auto error = Data::GetError(project)) ThrowError(ToCString(error.value.a), error.value.c);
}

REFLEX_END_INTERNAL

REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, PathGroup);
REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, Project);
REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, Package);
REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, Target);

Reflex::Reference<ReflexCLI::ProjectGen::Project> ReflexCLI::ProjectGen::Project::Acquire(Array<Reference<Project>> & projects, WString::View cfg_path, const Data::KeyMap * shared_keymap)
{
	auto filename = ResolveAbsolutePathCase(File::ResolveRelativePath(File::CorrectStrokes(cfg_path)));

	for (auto & project : projects)
	{
		if (project->m_cfg_path == filename)
		{
			Require(project->m_initialized, "circular include", EncodeUTF8(filename));
			return project;
		}
	}

	Reference<Data::KeyMap> created_keymap;
	if (!shared_keymap)
	{
		created_keymap = Make<Data::KeyMap>();
		shared_keymap = created_keymap.Adr();
	}
	auto project = Make<Project>(*shared_keymap);
	project->m_cfg_path = filename;
	project->m_root = File::SplitFilename(filename).a;
	projects.Push(project);
	auto options = Data::GetPropertySet(Bootstrap::global->prefs, kPersistentVariables);
	DecodeProjectDocument(*project, projects, filename, options);
	project->Initialize(filename);
	project->m_initialized = true;

	return project;
}

void ReflexCLI::ProjectGen::Project::AddInclude(Reference<Project> project)
{
	for (auto & existing : m_includes) if (existing.Adr() == project.Adr()) return;
	m_includes.Push(std::move(project));
}

const ReflexCLI::ProjectGen::ValidatedPropertySet * ReflexCLI::ProjectGen::Project::FindTemplate(Key32 id) const
{
	for (auto & item : Iterate<ValidatedPropertySet>())
	{
		auto candidate = Cast<ValidatedPropertySet>(item.value);
		if (candidate->type == kPropertySetTypeTemplate && candidate->id == id && candidate->IsEnabled()) return candidate.Adr();
	}
	for (auto & included : m_includes) if (auto match = included->FindTemplate(id)) return match;
	return nullptr;
}

Reflex::Reference<ReflexCLI::ProjectGen::Target> ReflexCLI::ProjectGen::Project::FindTarget(CString::View name) const
{
	for (auto & target : m_targets) if (target->GetName() == name) return target;
	for (auto & included : m_includes) if (auto match = included->FindTarget(name)) return match;
	return {};
}

bool ReflexCLI::ProjectGen::Project::FindDependency(CString::View name, Reference<Target> & target, Reference<Package> & package) const
{
	for (auto & candidate : m_targets) if (candidate->GetName() == name)
	{
		target = candidate;
		return true;
	}
	for (auto & candidate : m_packages) if (candidate->GetName() == name)
	{
		package = candidate;
		return true;
	}
	for (auto & included : m_includes) if (included->FindDependency(name, target, package)) return true;
	return false;
}

Reflex::Array<ReflexCLI::Variable> ReflexCLI::ProjectGen::Project::GetDocumentVariables(Key32 property) const
{
	Array<Variable> result;
	for (UInt index = m_includes.GetSize(); index; --index)
	{
		for (auto & variable : m_includes[index - 1]->GetDocumentVariables(property)) SetVariable(result, variable.name, variable.value);
	}
	for (auto & item : Iterate())
	{
		if (item.key.id != property) continue;
		auto node = DynamicCast<Data::PropertySet>(item.value);
		if (!node) continue;
		for (auto & [address, value] : node->Iterate())
		{
			if (address.type_id == GetTypeID<Data::KeyMap>()) continue;
			auto name = Data::GetKey(*keymap, address.id);
			Require(address.type_id == GetTypeID<Data::CStringProperty>(), name, "expected a string");
			SetVariable(result, name, ToWString(Cast<Data::CStringProperty>(value)->value));
		}
		break;
	}
	return result;
}

bool ReflexCLI::ProjectGen::Project::HasDocumentProperty(Key32 id) const
{
	for (auto & item : Iterate()) if (item.key.id == id) return true;
	for (auto & included : m_includes) if (included->HasDocumentProperty(id)) return true;
	return false;
}

void ReflexCLI::ProjectGen::Project::OnQueryProperty(Address address, Object * & object) const
{
	auto fallback = object;
	Data::PropertySet::OnQueryProperty(address, object);
	if (object != fallback) return;
	for (auto & item : Iterate()) if (item.key.id == address.id) return;
	for (auto & included : m_includes)
	{
		object = included->QueryProperty(address, fallback);
		if (object != fallback || included->HasDocumentProperty(address.id)) return;
	}
}

void ReflexCLI::ProjectGen::Project::Initialize(WString::View source_cfg_path)
{
	auto & values = *this;
	m_cfg_path = source_cfg_path;
	m_root = File::SplitFilename(m_cfg_path).a;
	m_format = UInt32(Data::GetInt32(values, kFormat));
	Require(m_format == 1, "format", "unsupported format version");

	if (auto value = Data::GetCString(values, kGeneratedDirectory))
	{
		constexpr char disallow[] = { L'~', ':', '/', '\\', '.' };
		for (auto i : disallow) Require(!Search(value, i), kGeneratedDirectory, "must name an immediate sub-folder");
		m_generated_directory = File::CorrectTrailingStroke(DecodeUTF8(value));
	}

	auto keymap = Data::AcquireKeyMap(values);

	auto project_variables = GetVariables(values, kNullKey, *keymap);

	for (auto & item : values.Iterate())
	{
		if (item.key == MakeAddress<Data::CStringProperty>(MakeKey32(kProjectName)))
		{
			m_name = EncodeUTF8(EvaluateVariableExpressions(ToWString(Cast<Data::CStringProperty>(item.value)->value), project_variables, kVariableSyntaxProject, {}));
			break;
		}
	}

	auto add_target = [this](ValidatedPropertySet & source, bool library)
	{
		if (!source.IsEnabled()) return;
		m_targets.Push(Make<Target>(*this, source, library));
	};
	for (auto & item : values.Iterate<ValidatedPropertySet>())
	{
		auto source = Cast<ValidatedPropertySet>(item.value);
		if (source->type == kPropertySetTypeTarget) add_target(*source, false);
		else if (source->type == kPropertySetTypeLibrary) add_target(*source, true);
		else if (source->type == kPropertySetTypePackage) m_packages.Push(Make<Package>(*this, *source));
	}
	Array<CString> configuration_names;
	for (auto & target : m_targets)
	{
		for (auto & platform : target->GetTargetPlatforms())
		{
			for (auto & configuration : platform->GetTargetConfigurations())
			{
				if (!Search(configuration_names, configuration->GetName())) configuration_names.Push(configuration->GetName());
			}
		}
	}
	if (m_targets)
	{
		Require(True(configuration_names), "project", "has no configurations");
		auto default_configuration = Data::GetKey32(values, MakeKey32(kDefaultConfiguration));
		if (IsSet(default_configuration))
		{
			m_default_configuration = Data::GetKey(*keymap, default_configuration);
			Require(True(Search(configuration_names, m_default_configuration)), "project", "invalid default_configuration");
		}
		else if (auto release = Search<CaseInsensitive>(configuration_names, kRelease))
		{
			m_default_configuration = configuration_names[release.value];
		}
		else
		{
			m_default_configuration = configuration_names.GetFirst();
		}
		for (auto & target : m_targets)
		{
			for (auto & platform : target->GetTargetPlatforms())
			{
				Require(platform->FindConfiguration(m_default_configuration), target->GetName(), Join("missing default configuration ", m_default_configuration));
			}
		}
	}

	UInt library_count = 0;
	for (auto & target : m_targets) library_count += target->IsLibrary();
	m_library_only = library_count == m_targets.GetSize();
	if (!m_library_only) Require(True(m_name), kProjectName, "a project name is required");

	ResolveTargetDependencies(*this);
}

ReflexCLI::ProjectGen::Package::Package(Project & project, ValidatedPropertySet & source)
	: CompiledObject(GetDriverName(source))
	, project(project)
{
	if (auto dependencies = source.QueryProperty<AppendableKeys>(MakeKey32(kDependencies)))
	{
		m_dependency_names = MakeArray(dependencies->values, [keymap = source.keymap](Key32 id) -> CString
		{
			return Data::GetKey(keymap, id);
		});
	}
	Require(True(m_dependency_names), kDependencies, Join(GetName(), ": package has no dependencies"));
	RequireUnique(m_dependency_names, kDependencies);
}

ReflexCLI::ProjectGen::Target::Target(Project & project, ValidatedPropertySet & source, bool library)
	: CompiledObject(GetDriverName(source))
	, project(project)
	, m_library(library)
	, m_source(source)
{
	for (auto & item : m_source->Iterate<PlatformPropertySet>())
	{
		auto platform = Cast<PlatformPropertySet>(item.value);
		if (platform->IsEnabled()) m_platforms.Push(Make<TargetPlatform>(*this, platform));
	}
	Require(True(m_platforms), GetName(), "has no platforms");

}

ReflexCLI::ProjectGen::TargetPlatform::TargetPlatform(Target & target, PlatformPropertySet & values)
	: CompiledObject(kBuildPlatforms[values.GetPlatform()])
	, target(target)
	, m_platform(values.GetPlatform())
	, m_source(values)
{
	Array<Key32> dependency_ids;
	for (auto & item : values.Iterate<ValidatedPropertySet>())
	{
		auto configuration = Cast<ValidatedPropertySet>(item.value);
		if (configuration->type != kPropertySetTypeConfiguration) continue;
		if (!configuration->IsEnabled()) continue;

		Array<Key32> configuration_dependencies;
		if (auto dependencies = configuration->QueryProperty<AppendableKeys>(MakeKey32(kDependencies)))
			configuration_dependencies.Append(dependencies->values);
		if (m_configurations)
		{
			bool identical = dependency_ids.GetSize() == configuration_dependencies.GetSize();
			REFLEX_LOOP(index, dependency_ids.GetSize()) if (identical && dependency_ids[index] != configuration_dependencies[index]) identical = false;
			Require(identical, kDependencies, Join(target.GetName(), ':', GetName(), ": must be identical across configurations"));
		}
		else
		{
			dependency_ids.Append(configuration_dependencies);
			for (auto id : dependency_ids)
			{
				auto dependency_name = Data::GetKey(*values.keymap, id);
				m_dependency_names.Push(dependency_name);
			}
			RequireUnique(m_dependency_names, kDependencies);
		}

		m_configurations.Push(Make<TargetConfiguration>(*this, configuration));
	}
	Require(True(m_configurations), Join(target.GetName(), ':', GetName()), "has no configurations");
}

void ReflexCLI::ProjectGen::Target::ResolveDependencies()
{
	auto expand_dependency = [](Project & scope, CString::View dependency_name, Array<const Package *> & active_packages, Array<Reference<Target>> & result, auto && expand_dependency) -> void
	{
		Reference<Target> target;
		Reference<Package> package;
		Require(scope.FindDependency(dependency_name, target, package), kDependencies, Join(scope.GetName(), ": dependency not found ", dependency_name));
		if (target)
		{
			for (auto & existing : result) if (existing.Adr() == target.Adr()) return;
			result.Push(std::move(target));
			return;
		}
		Require(!Search(active_packages, package.Adr()), kDependencies, Join("package dependency cycle at ", package->GetName()));
		active_packages.Push(package.Adr());
		for (auto & member : package->GetDependencyNames()) expand_dependency(*package->project, member, active_packages, result, expand_dependency);
		active_packages.Pop();
	};

	auto validate_dependency_compatibility = [this](const TargetPlatform & platform, const Target & dependency)
	{
		auto dependency_platform = dependency.FindPlatform(platform.GetPlatform());
		Require(dependency_platform, Join(GetName(), " -> ", dependency.GetName()), Join("target does not support ", platform.GetName()));

		for (auto & config : platform.GetTargetConfigurations())
		{
			auto dependency_config = dependency_platform->FindConfiguration(config->GetName());
			auto desc = Join(GetName(), " -> ", dependency.GetName());
			Require(dependency_config, desc, Join("missing configuration ", config->GetName()));
			for (auto architecture : config->GetArchitectures())
			{
				Require(True(Search(dependency_config->GetArchitectures(), architecture)), desc, Join("missing architecture ", kArchitectureNames[architecture], " for ", config->GetName()));
			}
		}
	};

	for (auto & platform : m_platforms)
	{
		for (auto & dependency_name : platform->m_dependency_names)
		{
			Array<const Package *> active_packages;
			Array<Reference<Target>> dependencies;
			expand_dependency(*project, dependency_name, active_packages, dependencies, expand_dependency);
			for (auto & dependency : dependencies)
			{
				Require(dependency.Adr() != this, kDependencies, Join(GetName(), ": target cannot depend on itself"));
				validate_dependency_compatibility(*platform, *dependency);
				bool platform_exists = false;
				for (auto & existing : platform->m_dependencies) if (existing.Adr() == dependency.Adr()) platform_exists = true;
				if (!platform_exists) platform->m_dependencies.Push(dependency);
				bool exists = false;
				for (auto & existing : m_dependencies) if (existing.Adr() == dependency.Adr()) exists = true;
				if (!exists) m_dependencies.Push(dependency);
			}
		}
		platform->m_dependency_names.Clear();
	}
}

void ReflexCLI::ProjectGen::Target::ResolveIncludeDirectories()
{
	for (auto & platform : m_platforms)
	{
		for (auto & configuration : platform->GetTargetConfigurations())
		{
			Array<const TargetConfiguration *> dependencies;
			for (auto & dependency : platform->GetDependencies())
			{
				auto dependency_platform = dependency->FindPlatform(platform->GetPlatform());
				REFLEX_ASSERT(dependency_platform);
				auto dependency_configuration = dependency_platform->FindConfiguration(configuration->GetName());
				REFLEX_ASSERT(dependency_configuration);
				dependencies.Push(dependency_configuration);
			}
			configuration->ResolveIncludeDirectories(dependencies);
		}
	}
}

Reflex::ArrayView<Reflex::Reference<ReflexCLI::ProjectGen::Target>> ReflexCLI::ProjectGen::Target::GetDependencies(BuildPlatform platform) const
{
	if (auto target_platform = FindPlatform(platform)) return target_platform->GetDependencies();
	return {};
}

Reflex::Array<ReflexCLI::ProjectGen::PlatformTarget> ReflexCLI::ProjectGen::CollectTargetPlatforms(Project & root, BuildPlatform platform, Array<const TargetPlatform *> & generated_platforms)
{
	Array<PlatformTarget> result;
	Array<Target *> visited;
	auto visit = [&result, &visited, &root, &generated_platforms, platform, cmake = platform == kBuildPlatformCMake](Target & target, auto && visit) -> void
	{
		if (Search(visited, &target)) return;
		visited.Push(&target);
		auto target_platform = cmake ? nullptr : target.FindPlatform(platform);
		auto dependencies = cmake ? target.GetDependencies() : target.GetDependencies(platform);
		result.Push({ Reference<Target>(target), target_platform, target_platform && !target.IsLibrary() && GetOutputType(*target_platform) != kOutputType_static_library ? GetLinkDependencies(target, platform) : Array<Reference<Target>>() });
		if (target.project.Adr() == &root && !target.IsLibrary() && target_platform) generated_platforms.Push(target_platform);
		for (auto & dependency : dependencies) visit(*dependency, visit);
	};
	for (auto & target : root.GetTargets()) if (!target->IsLibrary() && (platform == kBuildPlatformCMake || target->FindPlatform(platform))) visit(*target, visit);
	return result;
}

Reflex::TRef<ReflexCLI::ProjectGen::PathGroup> ReflexCLI::ProjectGen::AcquirePathGroup(PathGroup & parent, CString::View value)
{
	if (value)
	{
		for (auto & i : parent) if (i.name == value) return i;
		auto child = New<PathGroup>();
		child->name = value;
		child->Attach(parent);
		return child;
	}

	return parent;
}

Reflex::WString ReflexCLI::ProjectGen::ExpandVariables(WString::View value, ArrayView <Variable> variables, System::Platform platform)
{
	constexpr Key32 kDeferredVariables[] = { "Architecture", "PlatformVariant" };
	return EvaluateVariableExpressions(value, variables, kVariableSyntaxProject, { kDeferredVariables }, platform);
}

ReflexCLI::ProjectGen::PathDesc ReflexCLI::ProjectGen::DecodePath(CString::View value)
{
	PathDesc result = { value };
	if (value)
	{
		result.path = File::CorrectStrokes(ToWString(value));
		result.is_absolute = System::IsAbsolutePath(result.path) || value.GetFirst() == '$';
	}
	return result;
}

Reflex::WString ReflexCLI::ProjectGen::ResolvePath(WString::View root, const PathDesc & path)
{
	return path.is_absolute ? path.path : Join(root, path.path);
}

Reflex::Array<ReflexCLI::ProjectGen::PathDesc> ReflexCLI::ProjectGen::FlattenPaths(const PathGroup & root)
{
	Array<PathDesc> result;
	auto append = [&result](const PathGroup & group, auto && append) -> void
	{
		for (auto & path : group.paths) result.Push(path.a);
		for (auto & child : group) append(child, append);
	};
	append(root, append);
	return result;
}

ReflexCLI::ProjectGen::FileGroups ReflexCLI::ProjectGen::GetFiles(const TargetConfiguration & config)
{
	return { config.GetPaths(false, kSources), config.GetPaths(false, kHeaders), config.GetPaths(false, kOtherFiles) };
}

bool ReflexCLI::ProjectGen::ComparePath(const PathDesc & a, const PathDesc & b)
{
	return a.is_absolute == b.is_absolute && a.path == b.path;
}

bool ReflexCLI::ProjectGen::ComparePaths(ArrayView<PathDesc> a, ArrayView<PathDesc> b)
{
	if (a.size != b.size) return false;
	REFLEX_LOOP(idx, a.size) if (!ComparePath(a[idx], b[idx])) return false;
	return true;
}

Reflex::Reference<ReflexCLI::ProjectGen::PathGroup> ReflexCLI::ProjectGen::ConsolidatePathGroups(const TargetPlatform & platform)
{
	REFLEX_LOCAL(bool,FindPath)(const PathGroup & group, const PathDesc & path, CString group_path, Key32 & type, CString & found_group)
	{
		for (auto & item : group.paths)
		{
			if (!ComparePath(item.a, path)) continue;
			type = item.b;
			found_group = std::move(group_path);
			return true;
		}
		for (auto & child : group)
		{
			auto child_path = group_path ? Join(group_path, "/", child.name) : child.name;
			if (Call(child, path, std::move(child_path), type, found_group)) return true;
		}
		return false;
	}
	REFLEX_END

	REFLEX_LOCAL(void,Merge)(PathGroup & root, PathGroup & target, const PathGroup & source, Key32 type, CString group_path)
	{
		for (auto & item : source.paths)
		{
			Key32 existing_type;
			CString existing_group;
			if (FindPath::Call(root, item.a, {}, existing_type, existing_group))
			{
				Require(existing_type == type && existing_group == group_path, "conflicting presentation file groups", Join(EncodeUTF8(item.a.path)));
				continue;
			}
			target.paths.Push({ item.a, type });
		}
		for (auto & child : source)
		{
			auto child_path = group_path ? Join(group_path, "/", child.name) : child.name;
			Call(root, AcquirePathGroup(target, child.name), child, type, std::move(child_path));
		}
	}
	REFLEX_END

	auto result = Make<PathGroup>();
	for (auto & config : platform.GetTargetConfigurations())
	{
		auto [sources,headers,other] = GetFiles(config);
		Merge::Call(*result, *result, *sources, Key32(kSources), {});
		Merge::Call(*result, *result, *headers, Key32(kHeaders), {});
		Merge::Call(*result, *result, *other, Key32(kOtherFiles), {});
	}
	return result;
}

ReflexCLI::ProjectGen::TargetConfiguration::TargetConfiguration(TargetPlatform & target_platform, ValidatedPropertySet & values)
	: CompiledObject(GetDriverName(values))
	, platform(target_platform)
	, m_source(values)
{
	auto & target = *target_platform.target;
	REFLEX_ASSERT(target_platform.target.Adr() == &target);

	constexpr CString::View kReservedVariables[] = { "Platform", "Configuration", "TargetName", "OutputName", "Extension", "Architecture", "PlatformVariant" };
	m_variables = GetVariables(*m_source, kNullKey, *m_source->keymap);
	for (auto variable_name : kReservedVariables) Require(!FindVariable(m_variables, variable_name), "reserved variable", variable_name);

	auto output_name = Data::GetCString(*m_source, MakeKey32(kOutputName), target.project->GetName());
	auto output_extension = Data::GetCString(*m_source, MakeKey32(kOutputExtension));
	SetVariable(m_variables, "Configuration", ToWString(GetName()));
	SetVariable(m_variables, "Platform", ToWString(target_platform.GetName()));
	SetVariable(m_variables, "TargetName", ToWString(target.GetName()));
	SetVariable(m_variables, "OutputName", ToWString(output_name));
	SetVariable(m_variables, "Extension", ToWString(output_extension));

	Array <WString> resolved;
	for (auto & variable : m_variables)
	{
		resolved.Push(Expand(ToWString(VariableReference(variable.name, kVariableSyntaxProject))));
	}
	REFLEX_LOOP(i, m_variables.GetSize()) m_variables[i].value = std::move(resolved[i]);
}

Reflex::WString ReflexCLI::ProjectGen::TargetConfiguration::Expand(WString::View value, System::Platform emission_platform, bool allow_deferred) const
{
	auto result = ExpandVariables(value, m_variables, emission_platform);
	auto build_platform = platform->GetPlatform();
	if (build_platform != kBuildPlatformMacOS && build_platform != kBuildPlatformIOS)
	{
		Require(!Search(result, L"$(PlatformVariant)"), "unsupported variable", "PlatformVariant is only defined for Apple platforms");
	}
	if (!allow_deferred)
	{
		Require(!Search(result, L"$(Architecture)") && !Search(result, L"$(PlatformVariant)"), "unsupported variable", "build-time variables are not available in this context");
	}
	return result;
}

Reflex::CString ReflexCLI::ProjectGen::TargetConfiguration::Expand(CString::View value, System::Platform emission_platform, bool allow_deferred) const
{
	return EncodeUTF8(Expand(ToWString(value), emission_platform, allow_deferred));
}

Reflex::CString ReflexCLI::ProjectGen::TargetConfiguration::GetVariable(CString::View variable, CString::View fallback) const
{
	if (auto value = FindVariable(m_variables, variable)) return EncodeUTF8(value->value);
	return fallback;
}

Reflex::CString ReflexCLI::ProjectGen::TargetConfiguration::GetString(Key32 property, CString::View fallback) const
{
	return EncodeUTF8(Expand(ToWString(Data::GetCString(*m_source, property, fallback))));
}

Reflex::Array<Reflex::CString> ReflexCLI::ProjectGen::TargetConfiguration::GetStrings(CString::View property) const
{
	Key32 id = property;
	Array<CString> result;
	auto values = m_source->QueryProperty<Data::ArrayOfCStringProperty>(id);
	if (!values) return result;
	for (auto & value : values->value)
	{
		auto expanded = Expand(value);
		result.Push(std::move(expanded));
	}
	RequireUnique(result, property);
	return result;
}

Reflex::Array<Reflex::CString> ReflexCLI::ProjectGen::TargetConfiguration::GetDefinitions() const
{
	Array<CString> result;
	for (auto & definition : GetVariableMap(kDefines))
	{
		auto value = EncodeUTF8(definition.value);
		result.Push(value ? Join(definition.name, '=', value) : definition.name);
	}
	return result;
}

Reflex::Array<ReflexCLI::Variable> ReflexCLI::ProjectGen::TargetConfiguration::GetVariableMap(CString::View property) const
{
	auto result = GetVariables(*m_source, property, *m_source->keymap);
	for (auto & variable : result) variable.value = Expand(variable.value);
	return result;
}

Reflex::Reference<ReflexCLI::ProjectGen::PathGroup> ReflexCLI::ProjectGen::TargetConfiguration::GetPaths(bool folders, CString::View property) const
{
	if (property == kIncludeDirectories && m_include_directories) return m_include_directories;
	return GetDeclaredPaths(folders, property);
}

Reflex::Reference<ReflexCLI::ProjectGen::PathGroup> ReflexCLI::ProjectGen::TargetConfiguration::GetDeclaredPaths(bool folders, CString::View property) const
{
	REFLEX_LOCAL(void,AddWildcard)(const TargetConfiguration & self, Key32 property, PathGroup & group, const PathDesc & base, WString::View suffix, bool recursive)
	{
		auto folder = ResolvePath(self.platform->target->project->GetRoot(), base);
		Require(File::IsDirectory(folder), "wildcard folder not found", EncodeUTF8(base.path));
		auto [child_folders, files] = File::List(folder, true);
		for (auto & [file,unused] : files)
		{
			if (Right<true>(file, suffix.size) == suffix)
			{
				group.paths.Push({ { .source = base.source, .path = Join(base.path, file), .is_absolute = base.is_absolute }, property });
			}
		}
		if (recursive)
		{
			for (auto & [child_folder,unused] : child_folders)
			{
				auto child_path = base;
				child_path.path.Append(child_folder);
				Call(self, property, AcquirePathGroup(group, EncodeUTF8(File::RemoveTrailingStroke(child_folder))), child_path, suffix, true);
			}
		}
	}
	REFLEX_END

	REFLEX_LOCAL(void,AddPath)(const TargetConfiguration & self, bool folders, Key32 property, PathGroup & group, CString::View value)
	{
		auto path = DecodePath(self.Expand(value));
		path.source = value;
		auto wildcard = Search(path.path, ToView(L"/*"));

		if (folders || !wildcard)
		{
			group.paths.Push({ std::move(path), property });
		}
		else
		{
			auto pattern = Mid(path.path, wildcard.value + 1);
			bool recursive = pattern.size >= 3 && Left(pattern, 3) == ToView(L"**/");
			if (recursive) pattern = Mid(pattern, 3);
			Require(pattern && pattern.GetFirst() == '*' && !Search(Mid(pattern, 1), '*') && !Search(pattern, File::kStroke), "unsupported wildcard", EncodeUTF8(path.path));
			auto suffix = Mid(pattern, 1);
			path.path.SetSize(wildcard.value + 1);
			AddWildcard::Call(self, property, group, path, suffix, recursive);
		}
	}
	REFLEX_END

	REFLEX_LOCAL(void,Recurse)(const TargetConfiguration & self, bool folders, const Data::KeyMap & keymap, Key32 property, const Data::PropertySet & node, PathGroup & group)
	{
		for (auto & [adr, value] : node.Iterate())
		{
			auto child = AcquirePathGroup(group, Data::GetKey(keymap, adr.id));

			if (adr.type_id == GetTypeID<Data::PropertySet>())
			{
				if (child != group) Call(self, folders, keymap, property, Cast<Data::PropertySet>(value), *child);
			}
			else if (adr.type_id == GetTypeID<Data::ArrayOfCStringProperty>())
			{
				for (auto & i : Cast<Data::ArrayOfCStringProperty>(value)->value) AddPath::Call(self, folders, property, child, i);
			}
			else if (adr.type_id == GetTypeID<Data::CStringProperty>())
			{
				AddPath::Call(self, folders, property, child, Cast<Data::CStringProperty>(value)->value);
			}
		}
	};
	REFLEX_END

	auto result = Make<PathGroup>();
	Key32 id = property;
	if (auto source_group = Data::GetPropertySet(*m_source, id))
	{
		Recurse::Call(*this, folders, *m_source->keymap, property, source_group, result);
	}
	else
	{
		if (property == kIncludeDirectories || property == kPublicIncludeDirectories)
		{
			if (auto values = m_source->QueryProperty<AppendableStrings>(id))
				for (auto & source : values->values) AddPath::Call(*this, folders, property, result, source);
		}
		else if (auto values = m_source->QueryProperty<Data::ArrayOfCStringProperty>(id))
		{
			for (auto & source : values->value) AddPath::Call(*this, folders, property, result, source);
		}
	}

	return result;
}

const ReflexCLI::ProjectGen::PathGroup & ReflexCLI::ProjectGen::TargetConfiguration::GetPublicIncludeDirectories() const
{
	REFLEX_ASSERT(m_public_include_directories);
	return *m_public_include_directories;
}

void ReflexCLI::ProjectGen::TargetConfiguration::ResolveIncludeDirectories(ArrayView<const TargetConfiguration *> dependencies)
{
	auto includes = Make<PathGroup>();
	auto append = [](PathGroup & target, const PathGroup & source, WString::View source_root)
	{
		for (auto path : FlattenPaths(source))
		{
			if (source_root && !path.is_absolute)
			{
				path.path = ResolvePath(source_root, path);
				path.source = EncodeUTF8(path.path);
				path.is_absolute = true;
			}
			bool found = false;
			for (auto & existing : target.paths) if (ComparePath(existing.a, path)) found = true;
			if (!found) target.paths.Push({ std::move(path), MakeKey32(kIncludeDirectories) });
		}
	};

	auto declared_includes = GetDeclaredPaths(true, kIncludeDirectories);
	append(*includes, *declared_includes, {});

	auto has_public_includes = m_source->QueryProperty<AppendableStrings>(MakeKey32(kPublicIncludeDirectories));
	auto declared_public_includes = GetDeclaredPaths(true, kPublicIncludeDirectories);
	append(*includes, *declared_public_includes, {});
	auto public_includes = Make<PathGroup>();
	append(*public_includes, has_public_includes ? *declared_public_includes : *declared_includes, {});

	for (auto dependency : dependencies)
	{
		auto root = dependency->platform->target->project->GetRoot();
		append(*includes, dependency->GetPublicIncludeDirectories(), root);
		append(*public_includes, dependency->GetPublicIncludeDirectories(), root);
	}

	m_include_directories = includes;
	m_public_include_directories = public_includes;
}

Reflex::Float32 ReflexCLI::ProjectGen::TargetConfiguration::GetNumber(Key32 id, Float32 fallback) const
{
	if (auto value = m_source->QueryProperty<Data::Float32Property>(id))
	{
		return value->value;
	}
	else if (auto value = m_source->QueryProperty<Data::Int32Property>(id))
	{
		return Float32(value->value);
	}
	else
	{
		return fallback;
	}
}

Reflex::UInt ReflexCLI::ProjectGen::TargetConfiguration::GetEnumIndex(CString::View property, ArrayView<CString::View> names, UInt fallback) const
{
	auto fallback_key = fallback < names.size ? MakeKey32(names[fallback]) : kNullKey;
	return ParseEnumImpl(*m_source, property, names, fallback_key);
}

Reflex::Array<ReflexCLI::ProjectGen::Architecture> ReflexCLI::ProjectGen::TargetConfiguration::GetArchitectures() const
{
	Array<Architecture> result;
	auto values = m_source->QueryProperty<Data::ArrayOfKey32Property>(MakeKey32(kArchitectures));
	if (values) for (auto value : values->value)
	{
		Idx architecture;
		for (UInt i = 0; i < kArchitectureCount; ++i) if (MakeKey32(kArchitectureNames[i]) == value.value) architecture = i;
		Require(True(architecture), "architectures", "expected native, x86, x64, arm32, or arm64");
		result.Push(Architecture(architecture.value));
	}
	Require(!result.Empty(), "architectures", "undefined");
	return result;
}

Reflex::Array<ReflexCLI::ProjectGen::BuildActionDesc> ReflexCLI::ProjectGen::TargetConfiguration::GetBuildActions(BuildPhase phase, System::Platform platform) const
{
	Key32 id = kBuildPhases[phase];
	Array<BuildActionDesc> result;
	auto refs = m_source->QueryProperty<AppendablePropertySet>(id);
	if (refs)
	{
		REFLEX_LOOP(idx, refs->values.GetSize())
		{
			TRef value = refs->values[idx];
			BuildActionDesc action;
			auto default_name = Join(kBuildPhases[phase], '_', '#', ToCString(idx + 1));
			action.name = Data::GetCString(value, kName, default_name);
			for (auto i : Data::GetCStringArray(value, kCommand)) action.command.Push(ToWString(i));
			Require(True(action.command), kBuildPhases[phase], action.name);
			action.inputs = ProjectGen::GetPaths(value, kInputs);
			action.outputs = ProjectGen::GetPaths(value, kOutputs);
			action.always_run = Data::GetBool(value, kAlwaysRun, false);
			result.Push(std::move(action));
		}
	}
	auto allow_deferred = phase != kBuildPhasePostGenerate;
	for (auto & action : result)
	{
		action.name = Expand(action.name, platform, false);
		for (auto & command : action.command) command = Expand(command, platform, allow_deferred);
		for (auto ptr : {&action.inputs, &action.outputs})
		{
			for (auto & path : *ptr)
			{
				path.path = File::CorrectStrokes(Expand(path.path, platform, allow_deferred));
				path.is_absolute = System::IsAbsolutePath(path.path);
			}
		}
	}
	return result;
}

ReflexCLI::ProjectGen::OutputType ReflexCLI::ProjectGen::TargetConfiguration::GetProductType() const
{
	if (platform->target->IsLibrary()) return kOutputType_static_library;
	auto output_type = GetEnum<OutputType>(kOutputType, kOutputTypeNames, kOutputTypeCount);
	Require(output_type != kOutputTypeCount, kOutputType, "undefined");
	return output_type;
}

const ReflexCLI::ProjectGen::TargetPlatform * ReflexCLI::ProjectGen::Target::FindPlatform(BuildPlatform platform) const
{
	for (auto & candidate : GetTargetPlatforms()) if (candidate->GetPlatform() == platform) return candidate.Adr();
	return nullptr;
}

const ReflexCLI::ProjectGen::TargetConfiguration * ReflexCLI::ProjectGen::TargetPlatform::FindConfiguration(CString::View name) const
{
	for (auto & configuration : GetTargetConfigurations()) if (configuration->GetName() == name) return configuration.Adr();
	return nullptr;
}

Reflex::Pair <const ReflexCLI::ProjectGen::TargetConfiguration*> ReflexCLI::ProjectGen::FindDebugAndReleaseConfigurations(const TargetPlatform & platform)
{
	const TargetConfiguration * debug = nullptr;
	const TargetConfiguration * release = nullptr;

	for (auto & i : platform.GetTargetConfigurations())
	{
		if (CaseInsensitive::eq(i->GetName(), kDebug))
		{
			debug = i.Adr();
		}
		else if (CaseInsensitive::eq(i->GetName(), kRelease))
		{
			release = i.Adr();
		}
	}

	if (debug && release) return { debug, release };

	for (auto & i : platform.GetTargetConfigurations())
	{
		if (i->GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none) == kOptimization_none)
		{
			if (!debug) debug = i.Adr();
		}
		else
		{
			if (!release) release = i.Adr();
		}
	}

	return { debug, release };
}

ReflexCLI::ProjectGen::OutputType ReflexCLI::ProjectGen::GetOutputType(const TargetPlatform & platform)
{
	return GetConfigurationInvariant(platform, kOutputType, [](const TargetConfiguration & configuration) { return configuration.GetProductType(); });
}

Reflex::Array<Reflex::Reference<ReflexCLI::ProjectGen::Target>> ReflexCLI::ProjectGen::GetLinkDependencies(const Target & target, BuildPlatform selected_platform)
{
	Array<Reference<Target>> result;
	Array<Reference<Target>> visited;
	auto collect = [&result, &visited, selected_platform](const Target & reference, auto && collect) -> void
	{
		for (auto & dependency : reference.GetDependencies(selected_platform))
		{
			auto dependency_platform = dependency->FindPlatform(selected_platform);
			REFLEX_ASSERT(dependency_platform);
			auto output_type = GetOutputType(*dependency_platform);
			if (output_type != kOutputType_static_library && output_type != kOutputType_dynamic_library) continue;
			bool was_visited = false;
			for (auto & existing : visited) if (existing.Adr() == dependency.Adr()) was_visited = true;
			if (was_visited) continue;
			visited.Push(dependency);
			if (output_type == kOutputType_static_library) collect(*dependency, collect);
			result.Push(dependency);
		}
	};
	collect(target, collect);
	Reverse(result);
	return result;
}

void ReflexCLI::GenerateProject(const WString & path, ArrayView <CString::View> platforms_filter, System::FileHandle & out)
{
	static constexpr decltype(&ProjectGen::GenerateVisualStudioProject) kGenerators[] =
	{ 
		&ProjectGen::GenerateVisualStudioProject, 
		&ProjectGen::GenerateXcodeProject,
		&ProjectGen::GenerateXcodeProject,
		&ProjectGen::GenerateAndroidProject,
		&ProjectGen::GenerateLinuxProject,
		&ProjectGen::GenerateCMakeProject
	};
	REFLEX_STATIC_ASSERT(GetArraySize(kGenerators) == GetArraySize(kBuildPlatforms));

	auto filename = File::ResolveRelativePath(File::CorrectStrokes(path));
	Array <Reference<ProjectGen::Project>> projects;
	auto root_project = ProjectGen::Project::Acquire(projects, filename);
	auto & project = *root_project;
	
	struct PostGenerateAction
	{
		ConstTRef <ProjectGen::Project> project;
		ProjectGen::BuildActionDesc action;
	};
	Array<PostGenerateAction> post_generate;
	Map <CString,Map<WString>> used_paths;

	auto collect_post_generate = [&post_generate, &used_paths](ArrayView <Reference <ProjectGen::TargetConfiguration>> configs)
	{
		for (auto & config : configs)
		{
			auto project = config->platform->target->project;
			for (auto & action : config->GetBuildActions(ProjectGen::kBuildPhasePostGenerate, System::kPlatform))
			{
				bool found = false;
				for (auto & existing : post_generate) if (existing.project == project && existing.action == action) found = true;
				if (!found) post_generate.Push({ project, std::move(action) });
			}

			for (auto & path : ProjectGen::FlattenPaths(config->GetPaths(true, ProjectGen::kIncludeDirectories)))
			{
				if (path.path) used_paths.Acquire(path.source).Set(ProjectGen::ResolvePath(project->GetRoot(), path));
			}
		}
	};

	Array<BuildPlatform> platforms;
	for (auto & target : project.GetTargets())
	{
		for (auto & target_platform : target->GetTargetPlatforms())
		{
			auto platform = target_platform->GetPlatform();
			if (!Search(platforms, platform)) platforms.Push(platform);
		}
	}
	Require(True(platforms), "project", "no platforms");
	
	if (Search(platforms_filter, kBuildPlatforms[kBuildPlatformCMake]) && !Search(platforms, kBuildPlatformCMake)) platforms.Push(kBuildPlatformCMake);

	for (auto platform : platforms)
	{
		if (Search(platforms_filter, kBuildPlatforms[platform]))
		{
			auto platform_name = kBuildPlatforms[platform];
			try
			{
				Array<const ProjectGen::TargetPlatform *> generated_platforms;
				bool root_generated = false;
				for (auto & generated_project : projects)
				{
					if (generated_project->IsLibraryOnly()) continue;
					auto generated_count = generated_platforms.GetSize();
					kGenerators[platform](*generated_project, platform, generated_platforms);
					if (generated_project.Adr() == root_project.Adr()) root_generated = generated_platforms.GetSize() != generated_count;
				}
				if (root_generated)
				{
					for (auto generated : generated_platforms) collect_post_generate(generated->GetTargetConfigurations());

					Bootstrap::CLI::Print(out, Bootstrap::CLI::kColourBrightBlack, Join("generated ", platform_name));
				}
			}
			catch (const CString & error)
			{
				Bootstrap::CLI::ThrowError(Join(platform_name, ": ", error));
			}
		}
	}

	auto previous = System::GetCurrentDirectory();

	for (auto & [project,action]: post_generate)
	{
		Require(System::SetCurrentDirectory(project->GetRoot()), ProjectGen::kBuildPhases[ProjectGen::kBuildPhasePostGenerate], "failed to set working directory");
		Require(RunCommand(action.command.GetFirst(), Mid(action.command, 1), &out), "post_generate failed", action.name);
	}

	for (auto & i : used_paths)
	{
		for (auto & path : i.value)
		{
			if (!File::Exists(path.key))
			{
				Bootstrap::CLI::Print(out, Bootstrap::CLI::kColourYellow, Join("path not found: ", i.key));
				break;
			}
		}
	}

	System::SetCurrentDirectory(previous);
}
