#include "../../../include/reflex_ext/async/await.h"




void Reflex::Async::Detail::AttachAwait(Data::PropertySet & object, Key32 id, TRef <Task> task, decltype (&CreatePeriodicClock) create_clock, const Function <void(bool ok, Reflex::Object & result)> & callback)
{
	SetAbstractProperty(object, id, create_clock(0.25f, [&object, object_t = object.object_t, id, task = AutoRelease(task), callback]()
	{
		auto status = task->GetStatus();

		if (status != Task::kStatusPending)
		{
			Reflex::Detail::AbstractWeakRef weakref(object_t, Data::PropertySet::null, object);

			callback(status == Task::kStatusCompleted, task->GetResult());

			UnsetAbstractProperty(weakref.Load(), id);
		}
	}));
}
