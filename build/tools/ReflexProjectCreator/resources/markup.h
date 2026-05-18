#module "Data > Format"

GLX::Object ParseMarkup(String path, GLX::Style styles)
{
	using GLX;

	typedef Data::PropertySet PropertySet;

	auto data = Data::DecodePropertySet(Data::kReflexMarkupFormat, File::Open(path));
	
	Object Recurse(String folder, PropertySet data, Style styles, String text)
	{
		Object obj = Init(new, styles, text);
		
		obj.SetFlow(kFlowY);
		
		obj.EnableAutoFit(false, true);

		foreach (i : Data::GetPropertySetArray(data, ''))
		{
			Key32 type = i#type;
			
			Style style = styles[type];
		
			auto child = Recurse(folder, i, style, Data::GetCString(i, 'value'));
			
			AddInline(obj, child);
		}

		return obj;		
	}	
	
	return Recurse(File::SplitFilename(path).a, data, styles, "");	
}
