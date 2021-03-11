// Link.h: interface for the XLink class.
//
//////////////////////////////////////////////////////////////////////


#pragma once
//#include "Utility.h"
#include <stddef.h>
#include <iterator>
//template <typename TYPE> TYPE *XConstruct();
//template <typename TYPE> void XDestruct(TYPE *ptr);

class XLink  
{
public :
	XLink *m_pNext;
	XLink *m_pPrev;

	void PushFront(XLink *pParent)
	{
		Append(pParent);
	}

	void PushBack(XLink *pParent)
	{
		Insert(pParent);
	}

	void MoveTo(XLink *pTo)
	{
		if (m_pNext != this) {
			pTo->m_pPrev->m_pNext = m_pNext;
			m_pNext->m_pPrev = pTo->m_pPrev;
			pTo->m_pPrev = m_pPrev;
			m_pPrev->m_pNext = pTo;
			m_pPrev = m_pNext = this; 
		}
	}

	bool Empty()
	{
		return (this == m_pNext);
	}
	void Initialize() 
	{ 
		m_pPrev = m_pNext = this; 
	}
	void Append(XLink *pLink)
	{
		m_pNext = pLink->m_pNext; 
		m_pPrev = pLink; 
		pLink->m_pNext->m_pPrev = this; 
		pLink->m_pNext = this; 
	}

	void Insert(XLink *pLink) 
	{ 
		m_pPrev = pLink->m_pPrev;
		m_pNext = pLink;
		pLink->m_pPrev->m_pNext = this;
		pLink->m_pPrev = this;
	}
	void Remove() 
	{ 
		m_pNext->m_pPrev = m_pPrev; 
		m_pPrev->m_pNext = m_pNext; 
	}

};

#define LINK_POINTER(link, type, member) ((type *)((char *)link - (size_t)&((type *)0)->member))
#define LINKED_LIST_(TYPE, MEMBER) linked_list_<std::add_pointer_t<TYPE>, offsetof(TYPE, MEMBER)>
#define LINKED_LIST(TYPE, MEMBER) linked_list<std::add_pointer_t<TYPE>, offsetof(TYPE, MEMBER)>

template <typename TYPE, size_t offset = 0> class linked_list_
{
public :
	typedef	TYPE &reference;
	typedef	TYPE *pointer;
	typedef	TYPE value_type;
protected :
	XLink m_list;

public :
	class iterator : public std::iterator<std::bidirectional_iterator_tag, TYPE>
	{	// iterator for mutable list
	public :

		iterator() {}
		iterator(TYPE ptr) 
		{
			m_ptr = ptr;
		}

		explicit iterator(XLink *pLink)
		{
			m_ptr = (TYPE) ((char *)pLink - offset)	;
		}

		XLink* ToLinkPtr() const
		{
			return (XLink *)((char *)m_ptr + offset);
		}
		
		reference operator*() 
		{	// return designated value
			return m_ptr;
		}

		//pointer operator->() const
		//{	// return pointer to class object
		//	return (&**this);
		//}

		iterator& operator++()
		{	// preincrement
			m_ptr = iterator(ToLinkPtr()->m_pNext).m_ptr;
			return (*this);
		}

		iterator operator++(int)
		{	// postincrement
			iterator _Tmp = *this;
			++*this;
			return (_Tmp);
		}

		iterator& operator--()
		{	// predecrement
			m_ptr = iterator(ToLinkPtr()->m_pPrev).m_ptr;
			return (*this);
		}

		iterator operator--(int)
		{	// postdecrement
			iterator _Tmp = *this;
			--*this;
			return (_Tmp);
		}
		
		bool operator==(const iterator& _Right) const
		{	// test for iterator equality
			return (m_ptr == _Right.m_ptr);
		}

		bool operator!=(const iterator& _Right) const
		{	// test for iterator inequality
			return (!(*this == _Right));
		}

	protected:
		TYPE m_ptr;
	};
	class reverse_iterator : public std::iterator<std::bidirectional_iterator_tag, TYPE>
	{	// iterator for mutable list
	public :

		reverse_iterator() {}
		reverse_iterator(TYPE ptr) 
		{
			m_ptr = ptr;
		}

		explicit reverse_iterator(XLink *pLink)
		{
			m_ptr = (TYPE) ((char *)pLink - offset)	;
		}

		XLink* ToLinkPtr() const
		{
			return (XLink *)((char *)m_ptr + offset);
		}
		
		reference operator*() 
		{	// return designated value
			return m_ptr;
		}

		//pointer operator->() const
		//{	// return pointer to class object
		//	return (&**this);
		//}

		reverse_iterator& operator++()
		{	// preincrement
			m_ptr = reverse_iterator(ToLinkPtr()->m_pPrev).m_ptr;
			return (*this);
		}

		reverse_iterator operator++(int)
		{	// postincrement
			reverse_iterator _Tmp = *this;
			++*this;
			return (_Tmp);
		}

		reverse_iterator& operator--()
		{	// predecrement
			m_ptr = reverse_iterator(ToLinkPtr()->m_pNext).m_ptr;
			return (*this);
		}

		reverse_iterator operator--(int)
		{	// postdecrement
			reverse_iterator _Tmp = *this;
			--*this;
			return (_Tmp);
		}
		
		bool operator==(const reverse_iterator& _Right) const
		{	// test for iterator equality
			return (m_ptr == _Right.m_ptr);
		}

		bool operator!=(const reverse_iterator& _Right) const
		{	// test for iterator inequality
			return (!(*this == _Right));
		}

	protected:
		TYPE m_ptr;
	};

	linked_list_() 
	{
		m_list.Initialize();
	}

	void push_front(TYPE element)
	{
		iterator(element).ToLinkPtr()->Append(&m_list);
	}		

	void push_back(TYPE element)
	{
		iterator(element).ToLinkPtr()->Insert(&m_list);
	}

	TYPE front()
	{
		return *iterator(m_list.m_pNext);
	}

	TYPE back()
	{
		return *iterator(m_list.m_pPrev);
	}

	void pop_front()
	{
		m_list.m_pNext->Remove();
	}

	void pop_back()
	{
		m_list.m_pPrev->Remove();
	}

	bool empty()
	{
		return m_list.Empty() != 0;
	}

	iterator begin()
	{
		return iterator(m_list.m_pNext);
	}

	iterator end()
	{
		return iterator(&m_list);
	}

	reverse_iterator rbegin()
	{	// return iterator for beginning of reversed mutable sequence
		return (reverse_iterator(m_list.m_pPrev));
	}

	reverse_iterator rend()
	{	// return iterator for end of reversed mutable sequence
		return reverse_iterator(&m_list);
	}

	iterator insert(iterator where, TYPE element)
	{
		iterator(element).ToLinkPtr()->Insert(where.ToLinkPtr());
		return iterator(element);
	}

	static iterator erase(iterator where)
	{
		(where++).ToLinkPtr()->Remove();
		return where;
	}
		
	void clear()
	{
		m_list.Initialize();
	}
};

template <typename TYPE, size_t offset = 0> class linked_list : public linked_list_<TYPE, offset>
{
protected :
	long m_nSize;
	typedef	linked_list_<TYPE, offset> Super;
	typedef Super::iterator iterator;
public :
	linked_list() 
	{
		m_nSize = 0;
	}

	void push_front(TYPE element)
	{
		Super::push_front(element);
		m_nSize++;
	}		

	void push_back(TYPE element)
	{
		Super::push_back(element);
		m_nSize++;
	}

	void pop_front()
	{
		Super::pop_front();
		m_nSize--;
	}

	void pop_back()
	{
		Super::pop_back();
		m_nSize--;
	}

	bool empty()
	{
		return m_nSize == 0;
	}

	iterator insert(iterator where, TYPE element)
	{
		m_nSize++;
		return Super::insert(where, element);
	}

	iterator erase(iterator where)
	{
		m_nSize--;
		return Super::erase(where);
	}
		
	void clear()
	{
		m_nSize = 0;
		Super::clear();
	}

	size_t size()
	{
		return m_nSize;
	}

};

