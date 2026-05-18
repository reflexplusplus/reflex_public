#pragma once

#include "accordion.h"




//
//Experimental API

namespace Reflex::GLX
{

	class Tree;		//provisional, unsafe API

}




//
//Tree

class Reflex::GLX::Tree : public Scroller
{
public:

	REFLEX_OBJECT(GLX::Tree, Scroller);



	//types

	class Node;


	REFLEX_GLX_EVENT_ID(NodeOpen);				//@bool "allow" = true
	REFLEX_GLX_EVENT_ID(NodeClose);				//@bool "allow" = true
	REFLEX_GLX_EVENT_ID(NodeSelect);			//@bool "allow" = true
	REFLEX_GLX_EVENT_ID(NodeDeselect);			//@bool "allow" = true
	REFLEX_GLX_EVENT_ID(NodeRequestInsert);		//@Node "source", @bool "after"		//TODO -> Insert
	REFLEX_GLX_EVENT_ID(NodeRemove);			//@bool "allow"



	//declarations

	Tree();

	~Tree();



	//content

	void Clear();



	//selection

	void SelectNone();

	void SelectPrev();

	void SelectNext();



	//components

	const TRef <Node> root;



protected:

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	ConstTRef <Style> m_node_style;

	Array < Reference <Node> > m_selection;

};




//
//Tree::Node

class Reflex::GLX::Tree::Node : public Accordion
{
public:

	//lifetime

	~Node();



	//children

	void Clear();

	TRef <Node> AddNode();


	Node * GetParent();

	Node * GetPrev();

	Node * GetNext();


	Node * GetFirst();

	Node * GetLast();



	//branch

	void Open(bool animate = true, bool reveal = true);

	using Accordion::Close;

	using Accordion::Toggle;

	using Accordion::IsOpen;



	//select

	bool Select(bool multi = false);

	void Deselect();

	bool Selected() const;


	void Reveal(bool animate = true);



protected:

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	friend class Tree;


	Node(Tree & tree);

	Node(Tree::Node & parent);


	Tree & tree;	//unsafe

	Node * m_parent;

};
