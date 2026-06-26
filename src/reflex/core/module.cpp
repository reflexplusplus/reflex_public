#include "../../../include/reflex/core/detail/module.h"




//
//impl

void Reflex::Detail::Module::Init()
{
	if (SetFiltered(m_is_initalised, true))
	{
		REFLEX_IF_DEBUG(System::DebugLog(false, m_name);)


		//members

		auto members = Cast<AbstractMember*>(REFLEX_STACKALLOC(m_num_member * sizeof(AbstractMember*)));

		auto member = m_first_member;

		while (member)
		{
			*members++ = member;

			member = member->m_next;
		}

		auto members_end = members - m_num_member;

		while (members != members_end)
		{
			(*--members)->OnInit();
		}


		//modules

		auto modules = Cast<Module*>(REFLEX_STACKALLOC(m_num_module * sizeof(Module*)));

		auto module = m_first_module;

		while (module)
		{
			*modules++ = module;

			module = module->m_next;
		}

		auto modules_end = modules - m_num_module;

		while (modules != modules_end)
		{
			(*--modules)->Init();
		}
	}
}

void Reflex::Detail::Module::Deinit()
{
	if (SetFiltered(m_is_initalised, false))
	{
		//modules

		auto module = RemoveConst(m_first_module);

		while (module)
		{
			module->Deinit();

			module = module->m_next;
		}


		//members

		auto member = m_first_member;

		while (member)
		{
			member->OnDeinit();

			member = member->m_next;
		}


		#if	(REFLEX_DEBUG)
		char print_buffer[32];

		print_buffer[0] = '/';

		RawStringCopy(m_name, print_buffer + 1, 31);

		System::DebugLog(false, print_buffer);
		#endif
	}
}
