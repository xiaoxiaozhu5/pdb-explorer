#include <mutex>
#include <deque>

template<typename T>
class tsqueue
{
public:
	tsqueue() = default;
	tsqueue(const tsqueue<T>&) = delete;
	virtual ~tsqueue() { clear(); }

public:
	const T& front()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.front();
	}

	const T& back()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.back();
	}

	T pop_front()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto t = std::move(queue_.front());
		queue_.pop_front();
		return t;
	}

	T pop_back()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto t = std::move(queue_.back());
		queue_.pop_back();
		return t;
	}

	void push_back(const T& item)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			queue_.emplace_back(std::move(item));
		}

		std::unique_lock<std::mutex> ul(cv_mutex_);
		cv_.notify_one();
	}

	void push_front(const T& item)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			queue_.emplace_front(std::move(item));
		}

		std::unique_lock<std::mutex> ul(cv_mutex_);
		cv_.notify_one();
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

	size_t count()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.clear();
	}

	int wait(int timeout = 0)
	{
		int ret = 1;
		if (timeout == 0)
		{
			while (empty())
			{
				std::unique_lock<std::mutex> ul(cv_mutex_);
				cv_.wait(ul);
			}
		}
		else
		{
			std::unique_lock<std::mutex> ul(cv_mutex_);
			auto r = cv_.wait_for(ul, std::chrono::milliseconds(timeout), [this]{return !empty();});
			if (!r)
			{
				ret = 0;
			}
			else
			{
				ret = 1;
			}
		}
		return ret;
	}

protected:
	std::mutex mutex_;
	std::deque<T> queue_;
	std::condition_variable cv_;
	std::mutex cv_mutex_;
};