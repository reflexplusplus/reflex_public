
//TODO

REFLEX_NS(Reflex::GLXVM)

inline void BindData(VM::Compiler::State & compilestate, Key32 glx, VM::TypeRef dynamic_t, VM::TypeRef fsize_t, VM::TypeRef rect_t)
{
	auto bindings = compilestate.bindings;

	auto void_t = bindings->void_t;

	auto int32_t = bindings->int32_t;

	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;

	auto string_t = bindings->string_t;

	auto string_array_t = bindings->GetTypeByRTTID(K32("Array@String"));

	auto float32_array_t = bindings->GetTypeByRTTID(K32("Array@Float32"));


	auto text_t = VM::RegisterObject<GLX::Text>(bindings, glx, "Text");

	VM::AddConstructor(bindings, text_t, { string_t }, [](VM::Context & context)
	{
		VM_POP1(VM::String&);

		auto text = REFLEX_CREATE(GLX::Text, arg.GetView(), false);

		VM_RTN(text);
	});

	VM::AddConstructor(bindings, text_t, { string_array_t }, [](VM::Context & context)
	{
		VM_POP1(VM::ObjectArray&);

		auto text = REFLEX_CREATE(GLX::Text, true);

		if (auto lines = arg.GetView<VM::String>())
		{
			WString temp;

			for (auto & i : lines)
			{
				temp.Append(i->GetView());

				temp.Push(10);
			}

			temp.Pop();

			text->SetValue(temp);
		}

		VM_RTN(text);
	});

	VM::AddMethod(bindings, "GetValue", string_t, { text_t }, [](VM::Context & context)
	{
		VM_POP1(GLX::Text&);

		VM_RTN(New<VM::String>(arg.GetView()));
	});




	//colour

	VM::InstantiateObjectOf<GLX::Colour>(compilestate);



	//grid

	compilestate.InstantiateTemplateType(VM::kFn, { string_t, float32_t });

	//auto grid_t = VM::RegisterObject<GLX::Grid>(bindings, glx, "Grid");

	//AddConstructor(bindings, grid_t, { bool_t, float32_t }, [](VM::Context & context)
	//{
	//	VM_POP(bool,Float32);

	//	auto grid = REFLEX_CREATE(GLX::Grid, args.a);

	//	grid->SetUnit(args.b, [](Float32) { return WString(); });

	//	VM_RTN(grid);
	//});

	auto stringtizer_fn = bindings->GetTypeByRTTID(K32("Fn@(String,Float32)"));

	VM::AddFunction(bindings, glx, "SetViewPortGridStringifier", void_t, {dynamic_t, key32_t, stringtizer_fn}, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,Key32,VM::Detail::FnObject&);

		GLX::SetViewPortGridStringifier(args.a, args.b, [fn = AutoRelease(args.c)](Float32 value) -> WString
		{
			auto string = AutoRelease(VM::CallReturningObject<VM::String>(fn->context, fn, value));

			return string->GetView();
		});
		//auto grid = REFLEX_CREATE(GLX::Grid, args.a);

		//grid->SetUnit(args.b, [fnref = Reference<VM::Detail::FnObject>(args.c)](Float32 value) -> WString
		//{
		//	auto & fn = *fnref;

		//	auto string = AutoRelease(VM::CallReturningObject<VM::String>(fn.context, fn, value));

		//	return string->GetView();
		//});
	});



	//bargraph data

	auto block_t = VM::RegisterObject<Data::ArrayOfFloat32Property>(bindings, glx, "Values");

	AddConstructor(bindings, block_t, { int32_t }, [](VM::Context & context)
	{
		VM_POP1(UInt32);

		auto block = REFLEX_CREATE(Data::ArrayOfFloat32Property, UInt16(arg));

		block->value.Wipe();

		VM_RTN(block);
	});

	AddFunction(bindings, glx, "Bind", void_t, { block_t, float32_array_t }, [](VM::Context & context)
	{
		VM_POP(Data::ArrayOfFloat32Property&,VM::ValueArray&);

		args.b.SetData(args.a, ToRegion(args.a.value));
	});
}

REFLEX_END

//REFLEX_SET_TRAIT(Reflex::GLXVM::FaceDesc, IsSingleThreadExclusive);
