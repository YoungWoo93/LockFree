#pragma once

#include <Windows.h>


template <typename T>
class LockFreeQueue
{
public:
	LockFreeQueue(int count = 100) : pool(count) {
		volatile node* n = pool.newNode();

		n->setNext(nullptr);

		_head = n;
		_tail = n;
	}
	~LockFreeQueue() {

	}

	void push(T item) {
		volatile node* newKey = pool.newNode();

		newKey->setItem(item);
		newKey->setNext(nullptr);

		while (true)
		{
			volatile node* tail = _tail; // t
			volatile node* head = _head; // h
			volatile node* key = tail->getNext(); // next

			if (key == nullptr)
			{
				if (InterlockedCompareExchangePointer((PVOID*)(tail->getNextAdd()), (PVOID)newKey, (PVOID)key) == key) {
					InterlockedCompareExchangePointer((PVOID*)(&_tail), (PVOID)newKey, (PVOID)tail);

					break;
				}
			}
			else
			{
				InterlockedCompareExchangePointer((PVOID*)(&_tail), (PVOID)key, (PVOID)tail);
			}
		}

		InterlockedIncrement64(&size);
	}

	bool pop(T& item) {
		if (InterlockedDecrement64(&size) < 0) {
			InterlockedIncrement64(&size);

			return false;
		}

		while (true)
		{
			volatile node* head = _head; //h
			volatile node* tail = _tail; //t
			volatile node* key = head->getNext(); //nextHead
			//volatile node* temp = head->getPtr();

			if (key == nullptr) {
				SwitchToThread();
				continue;
			}

			if (head == tail) {
				if (InterlockedCompareExchangePointer((PVOID*)(&_tail), (PVOID)(tail->getNext()), (PVOID)tail) == tail)
					continue;
			}

			item = key->getItem();
			if (InterlockedCompareExchangePointer((PVOID*)(&_head), (PVOID)(key), (PVOID)head) == head) {
				pool.deleteNode(head);
				break;
			}
		}

		return true;
	}


public:
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

		inline volatile node** getNextAdd() volatile {
			return &(getPtr()->_next);
		}

		inline volatile node* getPoolNext() volatile {
			return getPtr()->_next;
			//return getPtr()->_poolNext;
		}

		inline void setNext(volatile node* next) volatile {
			getPtr()->_next = next;
		}

		inline void setPoolNext(volatile node* poolNext) volatile {
			getPtr()->_next = poolNext;
			//getPtr()->_poolNext = poolNext;
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
		volatile node* _poolNext;
	};

	class nodePool
	{
	public:
		nodePool(int initSize = 100) : poolTop(reinterpret_cast<volatile node*>(this)) {
			_size = _capacity = initSize;

			for (int i = 0; i < initSize; i++)
			{
				node* temp = new node();
				temp->setPoolNext(poolTop);
				poolTop = temp->Key(InterlockedIncrement(&_pushCount));
			}
		}

		~nodePool() {

			while (poolTop != reinterpret_cast<volatile node*>(this))
			{
				node* temp = poolTop->getPtr();
				poolTop = poolTop->getPoolNext();

				delete temp;
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
				nextTop = tempTop->getPoolNext();
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
				n->setPoolNext(tempTop);
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


	volatile node* _head;
	volatile node* _tail;
	volatile long long int size;
	nodePool pool;
};