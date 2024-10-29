#include "../LockFreeQueue.hpp"

#include <iostream>
#include <Windows.h>
#include <thread>
#include <queue>
#include <vector>
#include <fstream>

using namespace std;

SRWLOCK lock = SRWLOCK_INIT;
SRWLOCK srw = SRWLOCK_INIT;

LockFreeQueue<int>	lockfreeQueueNew;
queue<int> SRWQueue;

vector<thread> threads;
vector<unsigned long long int> results;

volatile bool flag = false;
volatile int dummy = 0;
ofstream f;

void workerThreadNew(unsigned long long int* result)
{
	while (!flag) {
		Sleep(1);
	};

	while (flag)
	{
		for (int i = 0; i < 10; i++)
		{
			lockfreeQueueNew.push(1);
			(*result)++;
		}
		for (int i = 0; i < 10; i++)
		{
			int temp;
			lockfreeQueueNew.pop(temp);
			(*result)++;
		}
	}

}


void SRWThread(unsigned long long int* result)
{
	while (!flag) {
		Sleep(1);
	};

	while (flag)
	{
		for (int i = 0; i < 10; i++)
		{
			AcquireSRWLockExclusive(&srw);
			SRWQueue.push(1);
			(*result)++;
			ReleaseSRWLockExclusive(&srw);
		}
		for (int i = 0; i < 10; i++)
		{
			int temp;
			AcquireSRWLockExclusive(&srw);
			temp = SRWQueue.front();
			SRWQueue.pop();
			(*result)++;
			ReleaseSRWLockExclusive(&srw);
		}
	}

}

void dummyThread()
{
	int temp = 0;
	while (!flag) {
		Sleep(1);
	};

	while (flag)
	{
		temp++;
	}

}

void lockFreeTestFunctionNew(int workers, int dummys)
{
	unsigned long long int result = 0;
	threads.clear();
	results.clear();

	results.assign(workers, 0);


	for (int i = 0; i < workers; i++)
	{
		threads.push_back(thread(workerThreadNew, &results[i]));
	}
	for (int i = 0; i < dummys; i++)
	{
		threads.push_back(thread(dummyThread));
	}

	Sleep(1000);
	flag = true;


	Sleep(0.2 * 60 * 1000);
	flag = false;

	for (int i = 0; i < threads.size(); i++)
		threads[i].join();

	for (int i = 0; i < results.size(); i++)
		result += results[i];

	f.open("result.txt", ios::app);
	f << "LF worker/dummy/result :\t" << workers << "\t" << dummys << "\t" << result << endl;
	f.close();
}


void SRWTestFunction(int workers, int dummys)
{
	unsigned long long int result = 0;
	threads.clear();
	results.clear();

	results.assign(workers, 0);


	for (int i = 0; i < workers; i++)
	{
		threads.push_back(thread(SRWThread, &results[i]));
	}
	for (int i = 0; i < dummys; i++)
	{
		threads.push_back(thread(dummyThread));
	}

	Sleep(1000);
	flag = true;


	Sleep(0.2 * 60 * 1000);
	flag = false;

	for (int i = 0; i < threads.size(); i++)
		threads[i].join();

	for (int i = 0; i < results.size(); i++)
		result += results[i];

	f.open("result.txt", ios::app);
	f << "SRW worker/dummy/result :\t" << workers << "\t" << dummys << "\t" << result << endl;
	f.close();
}



void main()
{

	int workers = 1;
	lockFreeTestFunctionNew(workers, 0);
	for (int dummys = 1; dummys < 16; dummys++)
	{
		lockFreeTestFunctionNew(workers, dummys);
	}

	workers = 4;
	for (int dummys = 1; dummys < 16; dummys++)
	{
		lockFreeTestFunctionNew(workers, dummys);
	}

	workers = 8;
	for (int dummys = 1; dummys < 16; dummys++)
	{
		lockFreeTestFunctionNew(workers, dummys);
	}

	workers = 1;
	lockFreeTestFunctionNew(workers, 0);
	for (int dummys = 1; dummys < 16; dummys++)
	{
		SRWTestFunction(workers, dummys);
	}

	workers = 4;
	for (int dummys = 1; dummys < 16; dummys++)
	{
		SRWTestFunction(workers, dummys);
	}

	workers = 8;
	for (int dummys = 1; dummys < 16; dummys++)
	{
		SRWTestFunction(workers, dummys);
	}

}