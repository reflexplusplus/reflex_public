#include "../../../include/reflex_ext/async/await.h"




void Reflex::Async::Detail::AttachAwait(Data::PropertySet & object, Key32 id, WillRetain <Task> task, decltype (&CreatePeriodicClock) create_clock, const Function <void(bool ok, Reflex::Object & result)> & callback)
{
	SetAbstractProperty(object, id, create_clock(0.25f, [&object, id, task = AutoRelease(task), callback]()
	{
		auto status = task->GetStatus();

		if (status != Task::kStatusPending)
		{
			REFLEX_IF_DEBUG(Reflex::Detail::WeakRef <Data::PropertySet> test(object));

			auto retain_clock = AutoRelease(QueryAbstractProperty(object, id));	//clock owns callback

			UnsetAbstractProperty(object, id);		//remove clock so that object can reattach an Await

			REFLEX_ASSERT(IsValid(test.Load()));	//OnXxxProperty is assumed to not destroy the owner

			callback(status == Task::kStatusCompleted, task->GetResult());
		}
	}));
}
