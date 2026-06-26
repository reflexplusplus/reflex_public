#include "../../../../include/reflex/data/interfaces/history.h"




//
//history

REFLEX_SET_TRAIT(Data::History::ItemImpl, IsSingleThreadExclusive);

REFLEX_NOINLINE Reflex::TRef <Reflex::Data::History::ItemImpl> Reflex::Data::History::ItemImpl::Create(UInt size)
{
	auto item = Reflex::Detail::Constructor<ItemImpl>::CreateVariableSize(g_default_allocator, size);

	item->size = size;

	return item;
}

Reflex::Data::History::History(UInt capacity)
	: m_capacity(capacity),
	m_bytesize(0),
	m_editcount(0),
	m_offset(0),
	m_position(&m_newest)
{
	ItemImpl * items[] = { &m_oldest, &m_newest };

	for (auto & i : items)
	{
		i->size = 0;

		i->Attach(m_list);
	}
}

Reflex::Data::History::~History()
{
	auto itr = m_oldest.GetNext();

	while (itr != &m_newest)
	{
		auto current = itr;

		itr = itr->GetNext();

		current->Detach();
	}
}

void Reflex::Data::History::Reset()
{
	auto itr = m_oldest.GetNext();

	while (itr != &m_newest)
	{
		auto current = itr;

		itr = itr->GetNext();

		current->Detach();
	}

	m_oldest.Attach(m_list);

	m_newest.Attach(m_list);

	m_bytesize = 0;

	m_position = &m_newest;

	m_editcount = 0;

	m_offset = 0;
}

void Reflex::Data::History::Enumerate(UInt offset, UInt range, const Function <void(Archive::View item)> & callback)
{
	auto itr = m_newest.GetPrev();

	while (offset--) itr = itr->GetPrev();

	while (range--)
	{
		Archive::View stream = { itr->data, itr->size };

		callback(stream);
		
		itr = itr->GetPrev();
	}
}

void Reflex::Data::History::Undo()
{
	if (CanUndo())
	{
		auto pitem = m_position->GetPrev();

		Archive::View stream = { pitem->data, pitem->size };

		m_position = pitem;

		m_offset++;

		OnRestoreHistory(stream, false);
	}
}

void Reflex::Data::History::Redo()
{
	if (CanRedo())
	{
		auto pitem = m_position;

		Archive::View stream = { pitem->data, pitem->size };

		pitem = pitem->GetNext();

		m_position = pitem;

		m_offset--;

		OnRestoreHistory(stream, true);
	}
}

void Reflex::Data::History::Commit(const Archive::View & archive)
{
	REFLEX_ASSERT(archive);

	if (archive)
	{
		auto size = archive.size;

		auto item = ItemImpl::Create(size);

		MemCopy(archive.data, item->data, size);

		item->size = size;


		if (m_position != &m_newest)
		{
			for (auto itr = m_position; itr != &m_newest;)
			{
				auto pitem = itr;

				m_bytesize -= pitem->size;

				itr = itr->GetNext();

				pitem->Detach();
			}

			m_position = &m_newest;

			m_editcount -= m_offset;

			m_offset = 0;
		}


		item->InsertBefore(m_newest);

		m_bytesize += item->size;

		while (m_bytesize > m_capacity)
		{
			auto first = m_oldest.GetNext();

			m_bytesize -= first->size;

 			first->Detach();
		}

		m_position = &m_newest;

		m_editcount++;

		m_offset = 0;
	}
}

void Reflex::Data::History::Serialize(Archive & stream) const
{
	auto num_item = UInt16(m_list.GetNumItem() - 2);

	Data::Serialize(stream, m_editcount, m_offset, num_item);

	auto pitem = m_newest.GetPrev();

	REFLEX_LOOP(idx, num_item)
	{
		Archive::View view = { pitem->data, pitem->size };

		Data::Serialize(stream, view);

		pitem = pitem->GetPrev();
	}
}

void Reflex::Data::History::Deserialize(Archive::View & stream)
{
	Reset();

	Data::Deserialize(stream, m_editcount, m_offset);

	auto num_item = Data::Deserialize<UInt16>(stream);

	REFLEX_LOOP(idx, num_item)
	{
		auto size = Data::Deserialize<UInt32>(stream);

		auto pitem = ItemImpl::Create(size);

		pitem->InsertAfter(m_oldest);

		auto bytes = Data::ReadBytes(stream, size);

		MemCopy(bytes.data, pitem->data, size);

		m_bytesize += size;
	}

	m_position = &m_newest;

	REFLEX_LOOP(idx, m_offset)
	{
		m_position = m_position->GetPrev();
	}
}
