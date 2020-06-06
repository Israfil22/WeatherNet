#pragma once
#include <vector>
#include <iostream>

class HC_locker
{
	public:
		HC_locker(volatile bool* lck_ptr, unsigned long long int delay = 10);
	protected:
		volatile bool* LOCK_PTR;
};

