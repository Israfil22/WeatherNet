#pragma once

#include "HC.mutex.h"
#include <chrono>
#include <thread>

HC_locker::HC_locker(volatile bool *lck_ptr, unsigned long long int delay){
	this->LOCK_PTR = lck_ptr;
	while(*this->LOCK_PTR){
		std::this_thread::sleep_for(std::chrono::microseconds(delay));
	}
}