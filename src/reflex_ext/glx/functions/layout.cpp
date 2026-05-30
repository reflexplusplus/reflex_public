#include "../../../../include/reflex_ext/glx/functions/layout.h"





void Reflex::GLX::SetOnAlign(Object & object, const Function <void(Object & object, bool isresponsive, Float & contenth, Detail::LayoutModel::AlignFn std_align)> & callback)
{
	SetFunctionProperty(object, "SetOnAlign", callback);

	object.SetLayoutModel([](Object & object) -> TRef <Detail::LayoutModel>
	{
		struct StandardLayoutWithOnAlign : public Detail::StandardLayout
		{
			StandardLayoutWithOnAlign(const Function <void(GLX::Object & object, bool isresponsive, Float & contenth, Detail::LayoutModel::AlignFn std_align)> & callback)
				: m_callback(callback)
			{
			}

			Pair <AccommodateFn, AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
			{
				auto [accommodatefn, alignfn] = Detail::StandardLayout::OnRebuild(object, layout_flags);

				m_std_align = alignfn;

				alignfn = [](GLX::Object & object, bool isresponsive, Float & contenth)
				{
					auto self = Cast<StandardLayoutWithOnAlign>(object.GetLayoutModel());

					self->m_callback(object, isresponsive, contenth, self->m_std_align);
				};

				return { accommodatefn, alignfn };
			}

			AlignFn m_std_align;
			
			Function <void(GLX::Object & object, bool isresponsive, Float & contenth, GLX::Detail::LayoutModel::AlignFn std_align)> m_callback;
		};

		if (auto func = QueryFunctionProperty<void(Object & object, bool isresponsive, Float & contenth, Detail::LayoutModel::AlignFn std_align)>(object, "SetOnAlign"))
		{
			return New<StandardLayoutWithOnAlign>(func->value);
		}
		else
		{
			return kStandardLayout(object);
		}
	});
}
