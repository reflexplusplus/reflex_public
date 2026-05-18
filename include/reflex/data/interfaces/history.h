#pragma once

#include "../types.h"




//
//Secondary API

namespace Reflex::Data
{

	class History;

}




//
//History

class Reflex::Data::History : public InterfaceOf <History>
{
public:

	//lifetime

	~History();



	//info

	bool CanUndo() const;

	bool CanRedo() const;

	UInt GetEditCount() const;

	UInt GetUndoOffset() const;	//0 is top, 1 first undo etc

	UInt GetByteSize() const;

	UInt GetCapacity() const;

	void Enumerate(UInt start, UInt range, const Function <void(Archive::View item)> & callback);



protected:

	//lifetime
	
	History(UInt capacity);



	//callbacks

	virtual void OnRestoreHistory(Archive::View stream, bool redo) = 0;



	//setup

	void SetCapacity(UInt bytes);



	//write

	void Commit(const Archive::View & archive);


	
	//traverse

	void Undo();

	void Redo();



	//persistence

	void Reset();

	void Deserialize(Archive::View & stream);

	void Serialize(Archive & stream) const;



private:

	struct ItemImpl : public Reflex::Item <ItemImpl>
	{
		static TRef <ItemImpl> Create(UInt size);

		using Item::Attach;
		using Item::Detach;
		using Item::InsertBefore;
		using Item::InsertAfter;

		UInt32 size;
		UInt8 data[4];
	};


	UInt32 m_capacity, m_bytesize;

	UInt16 m_editcount, m_offset;


	ItemImpl::List m_list;

	ItemImpl m_oldest, m_newest;

	ItemImpl * m_position;

};




//
//impl

inline void Reflex::Data::History::SetCapacity(UInt bytes)
{
	m_capacity = bytes;
}

inline Reflex::UInt Reflex::Data::History::GetCapacity() const
{
	return m_capacity;
}

inline bool Reflex::Data::History::CanUndo() const
{
	return m_position->GetPrev() != &m_oldest;
}

inline bool Reflex::Data::History::CanRedo() const
{
	return m_position != &m_newest;
}

inline Reflex::UInt Reflex::Data::History::GetEditCount() const
{
	REFLEX_ASSERT((m_list.GetNumItem() - 2) == m_editcount);

	return m_list.GetNumItem() - 2;
}

inline Reflex::UInt Reflex::Data::History::GetUndoOffset() const
{
	return m_offset;
}

inline Reflex::UInt Reflex::Data::History::GetByteSize() const
{
	return m_bytesize;
}
