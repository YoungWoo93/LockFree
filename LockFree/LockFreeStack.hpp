#pragma once

#include <Windows.h>
#include <iostream>

template <typename T>
class LockFreeStack
{
public:
	LockFreeStack(int count = 100) : pool(count) {

	}
	~LockFreeStack() {

	}

	void push(T item) {
		volatile node* n = pool.newNode();

		n->setItem(item);

		volatile node* tempTop;
		do {
			tempTop = top;
			n->setNext(tempTop);
		} while (InterlockedCompareExchangePointer((PVOID*)&top, (PVOID)n, (PVOID)tempTop) != tempTop);

		InterlockedIncrement64(&size);
	}

	bool pop(T* item) {
		if (InterlockedDecrement64(&size) < 0) {
			InterlockedIncrement64(&size);
			std::cout << "ERROR" << std::endl;
			return false;
		}

		volatile node* ret = top;
		volatile node* tempTop;
		volatile node* nextTop;
		do {
			tempTop = top;
			nextTop = tempTop->getNext();
		} while (InterlockedCompareExchangePointer((PVOID*)(&top), (PVOID)nextTop, (PVOID)tempTop) != tempTop);

		ret = tempTop;


		*item = ret->getItem();
		pool.deleteNode(ret);


		return true;
	}


public:
	volatile long long int size;

private:
	class node {
	public:
		node() {
		}
		node(T item) {
			this->item = item;
		}

		inline T getItem() volatile {
			return getPtr()->_item;
		}

		inline void setItem(T item) volatile {
			getPtr()->_item = item;
		}

		inline volatile node* getNext() volatile {
			return getPtr()->_next;
		}

		inline void setNext(volatile node* next) volatile {
			getPtr()->_next = next;
		}

		inline node* getPtr() volatile {
			return reinterpret_cast<node*>(0x00007FFFFFFFFFFF & reinterpret_cast<long long int>(this));
		}

		inline int getNumber() volatile {
			return this >> 47;
		}

		inline node* Key(long long int number) volatile {
			return reinterpret_cast<node*>((number << 47) | (0x00007FFFFFFFFFFF & reinterpret_cast<long long int>(this)));
		}

	private:
		T _item;
		volatile node* _next;
	};

	class nodePool
	{
	public:
		nodePool(int initSize = 100) : poolTop(nullptr) {
			_size = _capacity = initSize;

			for (int i = 0; i < initSize; i++)
			{
				node* temp = new node();
				temp->setNext(poolTop);
				poolTop = temp->Key(InterlockedIncrement(&_pushCount));
			}
		}

		volatile node* newNode()
		{
			volatile node* ret;

			if (InterlockedDecrement(&_size) < 0)
			{
				InterlockedIncrement(&_size);
				InterlockedIncrement(&_capacity);

				ret = new node;
				return ret->Key(InterlockedIncrement(&_pushCount));
			}


			volatile node* tempTop;
			volatile node* nextTop;
			do {
				tempTop = poolTop;
				nextTop = tempTop->getNext();
			} while (InterlockedCompareExchangePointer((PVOID*)&poolTop, (PVOID)nextTop, (PVOID)tempTop) != tempTop);

			ret = tempTop;
			return ret;
		}

		void deleteNode(volatile node* n)
		{
			n = n->Key(InterlockedIncrement(&_pushCount));

			volatile node* tempTop;
			do {
				tempTop = poolTop;
				n->setNext(tempTop);
			} while (InterlockedCompareExchangePointer((PVOID*)&poolTop, (PVOID)n, (PVOID)tempTop) != tempTop);


			InterlockedIncrement(&_size);
		}

		int size()
		{
			return _size;
		}

		int capacity()
		{
			return _capacity;
		}

	private:
		volatile node* poolTop;

		volatile long _pushCount;
		volatile long _size;
		volatile long _capacity;
	};


	volatile node* top;
	nodePool pool;
};