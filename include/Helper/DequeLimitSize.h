#pragma once

#include <deque>
#include <numeric>

template<class T>
class DequeLimitSize : public std::deque<T>
{
private:
	size_t limitSize_;

public:
	DequeLimitSize(size_t limitSize) :
		std::deque<T>(),
		limitSize_{ limitSize }
	{
	}

    size_t limitsize() const
    {
        return limitSize_;
    }

	void push_back(const T& x)
	{
		while (std::deque<T>::size() >= limitSize_) std::deque<T>::pop_front();
		std::deque<T>::push_back(x);
	}

	T average() const
	{
		if (std::deque<T>::size() <= 0) return T();
		return std::accumulate(std::deque<T>::begin(), std::deque<T>::end(), T()) / std::deque<T>::size();
	}

};
