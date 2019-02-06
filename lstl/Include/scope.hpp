#pragma once

#include <type_traits>
#include <exception>
#include <limits>

#if defined(_MSC_VER) && 190023918 <= _MSC_FULL_VER
#define ENABLE_EBO __declspec(empty_bases)
#else
#define ENABLE_EBO
#endif // defined(_MSC_VER)

namespace lstl {

	namespace policy {

		struct exit {
			bool m_Responsibility = true;

			void release() noexcept {
				m_Responsibility = false;
			}

			explicit operator bool() const noexcept {
				return m_Responsibility;
			}
		};

		struct fail {
			int m_BeforExceptions = std::uncaught_exceptions();

			void release() noexcept {
				m_BeforExceptions = (std::numeric_limits<int>::max)();
			}

			explicit operator bool() const noexcept {
				return m_BeforExceptions < std::uncaught_exceptions();
			}
		};

		struct succes {
			int m_BeforExceptions = std::uncaught_exceptions();

			void release() noexcept {
				m_BeforExceptions = -1;
			}

			explicit operator bool() const noexcept {
				return std::uncaught_exceptions() <= m_BeforExceptions;
			}
		};
	}

	namespace detail {
		template<typename R>
		struct funcptr_storage {
			R(*m_FuncPtr)();

			void operator()() {
				m_FuncPtr();
			}
		};

		//template<typename R, typename... Args>
		//struct bind_storage {

		//	template<typename... BindArgs>
		//	constexpr bind_storage(BindArgs&&... args)
		//		: m_Args{ std::forward<BindArgs>(args)... }
		//	{
		//		static_assert(std::conjunction<std::is_same<Args, BindArgs>...>);
		//	}

		//	void operator()() {
		//		this->call(std::make_index_sequence<sizeof...(Args)>{})
		//	}

		//private:
		//	R(*m_FuncPtr)(Args...);
		//	std::tuple<Args...> m_Args;

		//	template<std::size_t... Index>
		//	void call(std::index_sequence<Index...>) {
		//		m_FuncPtr(std::get<Index>(m_Args)...);
		//	}
		//};

		template<typename Callable, typename... Args>
		struct functor_storage_traits;

		template<typename Callable>
		struct functor_storage_traits<Callable> {
			using type = Callable;
		};

		template<typename R>
		struct functor_storage_traits<R(*)()> {
			using type = funcptr_storage<R>;
		};

		template<typename R, typename Arg, typename... Args>
		struct functor_storage_traits<R(*)(Arg, Args...)>;
	}


	template<typename ExitFunctor, typename Policy>
	struct ENABLE_EBO common_scope_exit : private Policy, private detail::functor_storage_traits<ExitFunctor>::type {
		
		using Storage = typename detail::functor_storage_traits<ExitFunctor>::type;

		constexpr common_scope_exit() noexcept(noexcept(std::is_nothrow_default_constructible<ExitFunctor>::value)) = default;

		constexpr common_scope_exit(const ExitFunctor& functor) noexcept(noexcept(std::is_nothrow_copy_constructible<ExitFunctor>::value))
			: Storage{ functor }
		{}

		constexpr common_scope_exit(ExitFunctor&& functor) noexcept(noexcept(std::is_nothrow_move_constructible<ExitFunctor>::value))
			: Storage{ std::move(functor) }
		{}

		~common_scope_exit() noexcept {
			try {
				if (*this) (*this)();
			}
			catch (...) {}
		}

		common_scope_exit(common_scope_exit&& other) noexcept(noexcept(std::conjunction<std::is_nothrow_move_constructible<Policy>, std::is_nothrow_move_constructible<ExitFunctor>>::value))
			: Policy{ std::move(other) }
			, Storage{ std::move(other) }
		{
			other.release();
		}

		using Policy::release;

		//コピー不可
		common_scope_exit(const common_scope_exit&) = delete;
		common_scope_exit& operator=(const common_scope_exit&) = delete;
		common_scope_exit& operator=(common_scope_exit&&) = delete;
	};

	template<typename ExitFunctor>
	struct scope_exit : public common_scope_exit<ExitFunctor, policy::exit> {
		using common_scope_exit<ExitFunctor, policy::exit>::common_scope_exit;
	};

	template<typename ExitFunctor>
	struct scope_fail : public common_scope_exit<ExitFunctor, policy::fail> {
		using common_scope_exit<ExitFunctor, policy::fail>::common_scope_exit;
	};

	template<typename ExitFunctor>
	struct scope_success : public common_scope_exit<ExitFunctor, policy::succes> {
		using common_scope_exit<ExitFunctor, policy::succes>::common_scope_exit;
	};
	
#ifdef __cpp_deduction_guides

	template<typename R, typename... Args>
	scope_exit(R(Args...))->scope_exit<R(*)(Args...)>;

	template<typename Functor>
	scope_exit(Functor)->scope_exit<Functor>;

	template<typename R, typename... Args>
	scope_fail(R(Args...))->scope_fail<R(*)(Args...)>;

	template<typename Functor>
	scope_fail(Functor)->scope_fail<Functor>;

	template<typename R, typename... Args>
	scope_success(R(Args...))->scope_success<R(*)(Args...)>;

	template<typename Functor>
	scope_success(Functor)->scope_success<Functor>;

#endif // __cpp_deduction_guides
}