#pragma once

#include <type_traits>
#include <exception>
#include <limits>

//MSVC用、2クラス以上継承時にEmpty Base Optimizationを有効にする
#if defined(_MSC_VER) && 190023918 <= _MSC_FULL_VER
#define ENABLE_EBO __declspec(empty_bases)
#else
#define ENABLE_EBO
#endif // defined(_MSC_VER)

namespace lstl {

	/**
	* @brief scope_exit、実行条件の詳細クラス群
	*/
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

	/**
	* @brief 継承可能なファンクタとそうでない関数ポインタとでストレージを切り替える実装
	*/
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


	/**
	* @brief 3つのscope_exit実装に共通している部分のクラス
	* @tparam ExitFunctor RAIIで実行する任意の関数呼び出し可能な型
	* @tparam Policy 実行条件を決めるポリシークラス（policy配下の3つ）
	*/
	template<typename ExitFunctor, typename Policy>
	struct ENABLE_EBO common_scope_exit : private Policy, private detail::functor_storage_traits<ExitFunctor>::type {
		
		using Storage = typename detail::functor_storage_traits<ExitFunctor>::type;

		/**
		* @brief デフォルトコンストラクタ
		* @detail 指定されたExitFunctorがデフォルトコンストラクト可能であるときのみ有効
		*/
		constexpr common_scope_exit() noexcept(std::is_nothrow_default_constructible<ExitFunctor>::value) = default;

		/**
		* @brief 関数オブジェクトをコピーして構築
		*/
		constexpr common_scope_exit(const ExitFunctor& functor) noexcept(std::is_nothrow_copy_constructible<ExitFunctor>::value)
			: Storage{ functor }
		{}

		/**
		* @brief 関数オブジェクトをムーブして構築
		*/
		constexpr common_scope_exit(ExitFunctor&& functor) noexcept(std::is_nothrow_move_constructible<ExitFunctor>::value)
			: Storage{ std::move(functor) }
		{}

		~common_scope_exit() noexcept {
			try {
				if (*this) (*this)();
			}
			catch (...) {}
		}

		/**
		* @brief ムーブコンストラクタ
		*/
		common_scope_exit(common_scope_exit&& other) noexcept(std::conjunction<std::is_nothrow_move_constructible<Policy>, std::is_nothrow_move_constructible<ExitFunctor>>::value)
			: Policy{ other }
			, Storage{ std::move(other) }
		{
			other.release();
		}

		/**
		* @brief 実行責任を放棄する
		* @detail 呼び出し後、実行可能な状況になっても実行されなくなる
		*/
		void release() noexcept {
			Policy::release();
		}

		//コピー不可
		common_scope_exit(const common_scope_exit&) = delete;
		common_scope_exit& operator=(const common_scope_exit&) = delete;
		common_scope_exit& operator=(common_scope_exit&&) = delete;
	};

	/**
	* @brief スコープを抜けるときに無条件で登録関数を実行するクラス
	* @tparam ExitFunctor 任意の関数呼び出し可能な型
	*/
	template<typename ExitFunctor>
	struct scope_exit : private common_scope_exit<ExitFunctor, policy::exit> {
		using common_scope_exit<ExitFunctor, policy::exit>::common_scope_exit;

		using common_scope_exit<ExitFunctor, policy::exit>::release;
	};

	/**
	* @brief スコープを抜けるとき、例外が投げられていなければ登録関数を実行するクラス
	* @tparam ExitFunctor 任意の関数呼び出し可能な型
	*/
	template<typename ExitFunctor>
	struct scope_fail : private common_scope_exit<ExitFunctor, policy::fail> {
		using common_scope_exit<ExitFunctor, policy::fail>::common_scope_exit;

		using common_scope_exit<ExitFunctor, policy::fail>::release;
	};

	/**
	* @brief スコープを抜けるとき、例外が投げられている場合に登録関数を実行するクラス
	* @tparam ExitFunctor 任意の関数呼び出し可能な型
	*/
	template<typename ExitFunctor>
	struct scope_success : private common_scope_exit<ExitFunctor, policy::succes> {
		using common_scope_exit<ExitFunctor, policy::succes>::common_scope_exit;

		using common_scope_exit<ExitFunctor, policy::succes>::release;
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

#undef ENABLE_EBO