



void Reflex::GLXVM::AttachCircular(Data::PropertySet & object, VM::Context & context)
{
	object.SetProperty(kNullKey, REFLEX_CREATE(ObjectOf<VM::Detail::Circular>, context, object));
}