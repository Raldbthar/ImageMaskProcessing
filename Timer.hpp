#pragma once

#include <chrono>


template<typename timeUnit = std::chrono::milliseconds, typename clockType = std::chrono::high_resolution_clock>
class Timer
{
public:
	typedef std::chrono::time_point<clockType> timeStamp;

	Timer() : startingTime{ clockType::now() } {}

	void restart()
	{
		startingTime = clockType::now();
	}

	template<typename T = timeUnit>
	[[nodiscard]] T time_elapsed() const
	{
		return std::chrono::duration_cast<T>(clockType::now() - startingTime);
	}

private:
	timeStamp startingTime;
};
