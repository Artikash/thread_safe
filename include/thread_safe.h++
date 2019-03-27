#include <mutex>
#include <type_traits>
#include <cassert>

namespace lib
{
	template <
		typename object_t_,
		typename mutex_t_ = std::mutex,
		typename shared_lock_t_ = std::unique_lock<mutex_t_>,
		typename lock_t_ = std::unique_lock<mutex_t_>
	>
		class thread_safe
	{
	public:

		template <typename... args_t>
		thread_safe(args_t&&... args) :
			object{ std::forward<args_t>(args)... }
		{}

		thread_safe(const thread_safe& other) :
			object{ other.acquire_shared().raw }
		{}

		thread_safe& operator=(const thread_safe& other)
		{
			if (this == &other) return *this;
			object_t temp = other.acquire_shared().raw;
			acquire().raw = temp;
			return *this;
		}

		thread_safe(thread_safe&& other) noexcept(std::is_nothrow_move_constructible_v<object_t>) :
			object{ std::move(other.acquire().raw) }
		{}

		thread_safe& operator=(thread_safe&& other) noexcept(std::is_nothrow_move_assignable_v<object_t>)
		{
			if (this == &other) return *this;
			object_t temp = std::move(other.acquire().raw);
			acquire().raw = std::move(temp);
			return *this;
		}

		~thread_safe()
		{
#ifndef NDEBUG
			assert(mutex.try_lock(), "Don't destroy an object while you're using it on another thread!");
			mutex.unlock();
#endif
		}

		using object_t = object_t_;
		using mutex_t = mutex_t_;
		using lock_t = lock_t_;
		using shared_lock_t = shared_lock_t_;

		struct locked_proxy_t
		{
			object_t* operator->() { return &raw; }
			lock_t lock;
			object_t& raw;
		};

		struct shared_locked_proxy_t
		{
			const object_t* operator->() { return &raw; }
			shared_lock_t lock;
			const object_t& raw;
		};

		locked_proxy_t acquire() { return { lock_t{ mutex }, object }; }

		shared_locked_proxy_t acquire_shared() const { return { shared_lock_t{ mutex }, object }; }

		locked_proxy_t operator->() { return acquire(); }

		shared_locked_proxy_t operator->() const { return acquire_shared(); }

		template <typename action_t>
		std::invoke_result_t<action_t, object_t&> execute(const action_t& action)
		{
			lock_t lock{ mutex };
			return action(object);
		}

		template <typename action_t>
		std::invoke_result_t<action_t, const object_t&> execute_shared(const action_t& action) const
		{
			shared_lock_t lock{ mutex };
			return action(object);
		}

		object_t copy() const
		{
			shared_lock_t lock{ mutex };
			return object;
		}

		void copy_into(object_t& dest) const
		{
			shared_lock_t lock{ mutex };
			dest = object;
		}

	private:
		object_t object;
		mutable mutex_t mutex;
	};
} // namespace lib
