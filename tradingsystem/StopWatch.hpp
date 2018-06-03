// StopWatch.hpp
//
// Author: Yuchen LIU
//
// An adapter class to measure how long it takes for an operation to complete

#ifndef StopWatch_HPP // Avoid multiple inclusion
#define StopWatch_HPP

// Header files
#include <chrono>

class StopWatch {
public: 
	StopWatch() {};
	void StartStopWatch() { start = std::chrono::system_clock::now(); };
	void StopStopWatch() {
		end = std::chrono::system_clock::now();
		elapsed_seconds = end - start;
	};
	void Reset() { elapsed_seconds = std::chrono::system_clock::duration::zero(); }
	double GetTime() const {
		return elapsed_seconds.count(); // xx seconds 
	}
private: 
	StopWatch(const StopWatch &) = default;
	StopWatch & operator=(const StopWatch &) = default;
	std::chrono::time_point <std::chrono::system_clock> start;
	std::chrono::time_point <std::chrono::system_clock> end;
	std::chrono::duration<double> elapsed_seconds;
};


#endif // !StopWatch_HPP