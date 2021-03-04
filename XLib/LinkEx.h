// Link.h: interface for the XLink class.
//
//////////////////////////////////////////////////////////////////////


#pragma once
#include <stddef.h>
#include <iterator>
#include "Link.h"
#include "Utility.h"


class CPointerLink : public XLink
{
public:
	void *m_ptr;
};

typedef	linked_list<CPointerLink *> POINTER_LINK;


template <typename TYPE> class pointer_list
{
public:
	typedef	TYPE	&reference;
	typedef	TYPE	*pointer;
	typedef	TYPE	value_type;
protected:
	POINTER_LINK	m_linklist;

public:
	class iterator : public std::iterator<std::bidirectional_iterator_tag, TYPE>
	{	// iterator for mutable list
	public:

		iterator() {}
		explicit iterator(POINTER_LINK::iterator it) 
		{
			m_it = it;
		}

		reference operator*() 
		{	// return designated value
			return (reference) (*m_it)->m_ptr;
		}

		//pointer operator->() const
		//{	// return pointer to class object
		//	return (&**this);
		//}

		iterator& operator++()
		{	// preincrement
			++m_it;
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
			++m_it;
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
			return (m_it == _Right.m_it);
		}

		bool operator!=(const iterator& _Right) const
		{	// test for iterator inequality
			return (!(*this == _Right));
		}

	protected:
		POINTER_LINK::iterator m_it;
	};

	~pointer_list()
	{
		clear();
	}

	void	push_front(TYPE element)
	{
		CPointerLink *ptr = XConstruct<CPointerLink>(1);
		ptr->m_ptr = element;
		m_linklist.push_front(ptr);
	}		

	void	push_back(TYPE element)
	{
		CPointerLink *ptr = XConstruct<CPointerLink>();
		ptr->m_ptr = element;
		m_linklist.push_back(ptr);
	}

	TYPE	front()
	{
		return (TYPE) m_linklist.front()->m_ptr;
	}

	TYPE	back()
	{
		return (TYPE) m_linklist.back()->m_ptr;
	}

	void	pop_front()
	{
		CPointerLink *ptr = m_linklist.front();
		m_linklist.pop_front();
		XDestruct(ptr);
	}

	void	pop_back()
	{
		CPointerLink *ptr = m_linklist.back();
		m_linklist.pop_back();
		XDestruct(ptr);
	}

	bool	empty()
	{
		return m_linklist.empty();
	}

	iterator begin()
	{
		return iterator(m_linklist.begin());
	}

	iterator end()
	{
		return iterator(m_linklist.end());
	}

	iterator insert(iterator where, TYPE element)
	{
		CPointerLink *ptr = Construct<CPointerLink>();
		ptr->m_ptr = element;

		return iterator(m_linklist.insert(where.m_it, ptr));
	}

	iterator erase(iterator where)
	{
		CPointerLink *ptr = *where.m_it;
		where = iterator(m_linklist.erase(where.m_it));
		XDestruct(ptr);
		return where;
	}
		
	void	clear()
	{
		for (POINTER_LINK::iterator it = m_linklist.begin(); it != m_linklist.end(); ) {
			CPointerLink *ptr = *it++;
			XDestruct(ptr);
		}
		m_linklist.clear();
	}

	size_t	size()
	{
		return m_linklist.size();
	}

};
