﻿#pragma once

#include <type_traits>
#include <exception>

#pragma warning(push)
//ユニコードの文字がShift-JISで分からないという警告抑止
#pragma warning(disable:4566)

namespace lstl {

	template<typename T>
	class optional;

	struct nullopt_t {
		constexpr explicit nullopt_t(std::nullptr_t) noexcept {}
	};

	struct in_place_t {
		explicit in_place_t() = default;
	};

	constexpr nullopt_t nullopt{ nullptr };

	constexpr in_place_t in_place{};


	class bad_optional_access : public std::exception {
	public:

		bad_optional_access() = default;

		const char* what() const noexcept override {
			return "Bad optional access";
		}
	};

	namespace optional_traits {

		/**
		* @brief 条件の論理積がtrueとなるときに有効化
		* @tparam Traits 条件列、boolに変換可能な::valueを持つ任意のメタ関数
		*/
		template<typename... Traits>
		using enabler = std::enable_if_t<std::conjunction<Traits...>::value, std::nullptr_t>;

		/**
		* @brief U → T への変換がoptionalの文脈で受け入れ可能かを調べる
		* @detail TがU&&で構築可能　かつ　Uがin_place_t　ではなく　Uはoptional<T>　でもない、場合にtrueとなる
		* @tparam T 変換先の型 
		* @tparam U 変換元の型
		*/
		template<typename T, typename U>
		using allow_conversion = std::conjunction<std::is_constructible<T, U&&>, std::negation<std::is_same<std::decay_t<U>, in_place_t>>, std::negation<std::is_same<optional<T>, std::decay_t<U>>>>;

		/**
		* @brief optional<U> → optional<T> への変換がoptionalの文脈で受け入れ可能かを調べる
		* @detail T != U かつ Tが optional<U>&/optional<U>&&/const optional<U>&/const optional<U>&& で構築できなく かつ
		* @detail optional<U>&/optional<U>&&/const optional<U>&/const optional<U>&& から Tへ変換不可能、であるときtrue
		* @detail つまり、指定されている条件が全てfalseであるときにtrue
		* @tparam T 変換先の型
		* @tparam U 変換元の型
		*/
		template<typename T, typename U>
		using allow_unwrap = std::negation<
			std::disjunction<
				std::is_same<T, U>, std::is_constructible<T, optional<U>&>, std::is_constructible<T, optional<U>&&>, std::is_constructible<T, const optional<U>&>, std::is_constructible<T, const optional<U>&&>,
				std::is_convertible<optional<U>&, T>, std::is_convertible<optional<U>&&, T>, std::is_convertible<const optional<U>&, T>, std::is_convertible<const optional<U>&&, T>
			>
		>;

		/**
		* @brief U → T への変換がoptionalへの代入の文脈で受け入れ可能かを調べる
		* @detail Uがoptional<T>ではなく　かつ　Tがスカラ型 でも　T == decay_­t<U> でもなく TがUによって構築可能　かつT&はUを代入可能　である場合にtrueとなる
		* @tparam T 変換先の型
		* @tparam U 変換元の型
		*/
		template<typename T, typename U>
		using allow_conversion_assign = std::conjunction<std::negation<std::is_same<optional<T>, std::decay_t<U>>>, std::negation<std::conjunction<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>>>, std::is_constructible<T, U>, std::is_assignable<T&, U>>;

		/**
		* @brief optional<U> → optional<T> への変換がoptionalへの代入の文脈で受け入れ可能かを調べる
		* @detail allow_unwrap<T, U>::value == true であり
		* @detail T& へ いかなる形の参照のoptional<U>も代入不可能 であるときにtrue
		* @tparam T 変換先の型
		* @tparam U 変換元の型
		*/
		template<typename T, typename U>
		using allow_unwrap_assign = std::conjunction<
			allow_unwrap<T, U>,
			std::negation<
				std::disjunction<std::is_assignable<T&, optional<U>&>, std::is_assignable<T&, optional<U>&&>, std::is_assignable<T&, const optional<U>&>, std::is_assignable<T&, const optional<U>&&>>
			>
		>;

		namespace no_adl_swap_impl {
			void swap();

			template<typename T, typename = void>
			struct no_adl_swap : std::true_type {};

			template<typename T>
			struct no_adl_swap<T, std::void_t<decltype(swap(std::declval<T&>(), std::declval<T&>()))>> : std::false_type {};
		}

		/**
		* @brief swapにかかる操作が全てtrivialであるかを調べる
		* @detail trivially destructible かつ trivialにムーブ構築・代入可能で、独自のswap関数を持たない場合にtrue
		* @tparam T 調べる型
		*/
		template<typename T>
		using is_trivially_swappable = std::conjunction<std::is_trivially_destructible<T>, std::is_trivially_move_constructible<T>, std::is_trivially_move_assignable<T>, no_adl_swap_impl::no_adl_swap<T>>;


		template<typename F, typename... Args>
		using invoke_result_t = std::decay_t<decltype(std::invoke(std::declval<F>(), std::declval<Args>()...))>;
	}

#if defined(_MSC_VER) && _MSC_VER == 1900

	/**
	* @brief std::is_swappable
	* @detail IntelliSenseエラー回避のため
	*/
	template<typename T>
	using is_swappable = std::_Is_swappable<T>;

	/**
	* @brief std::is_nothrow_swappable
	* @detail IntelliSenseエラー回避のため
	*/
	template<typename T>
	using is_nothrow_swappable = std::_Is_nothrow_swappable<T>;

#else

	template<typename T>
	using is_swappable = std::is_swappable<T>;

	template<typename T>
	using is_nothrow_swappable = std::is_nothrow_swappable<T>;

#endif // defined(_MSC_VER) && _MSC_VER <= 1900
	
	namespace detail {

		/**
		* @brief コピー構築の有効化
		*/
		template<typename Base, typename T>
		struct enable_copy_construct : public Base {
			using Base::Base;

			template<typename... Args>
			constexpr enable_copy_construct(Args&&... args) noexcept(std::is_nothrow_constructible<Base, Args&&...>::value)
				: Base{ std::forward<Args>(args)... }
			{}

			enable_copy_construct(const enable_copy_construct& other) : Base{ nullopt }
			{
				Base::construct_from_other(static_cast<const Base&>(other));
			}

			enable_copy_construct(enable_copy_construct&&) = default;
			enable_copy_construct& operator=(const enable_copy_construct&) = default;
			enable_copy_construct& operator=(enable_copy_construct&&) = default;
		};

		/**
		* @brief コピー構築のチェック
		* @detail Tが非トリビアルにコピー構築可能であるとき、コピーコンストラクタを定義する
		*/
		template<typename Base, typename T>
		using check_copy_construct = std::conditional_t<
			std::conjunction<std::is_copy_constructible<T>, std::negation<std::is_trivially_copy_constructible<T>>>::value,
			enable_copy_construct<Base, T>,
			Base
		>;

		/**
		* @brief ムーブ構築の有効化
		*/
		template<typename Base, typename T>
		struct enable_move_construct : public check_copy_construct<Base, T> {
			using base_t = check_copy_construct<Base, T>;
			using base_t::base_t;

			template<typename... Args>
			constexpr enable_move_construct(Args&&... args) noexcept(std::is_nothrow_constructible<base_t, Args&&...>::value)
				: base_t{ std::forward<Args>(args)... }
			{}

			enable_move_construct(const enable_move_construct&) = default;

			enable_move_construct(enable_move_construct&& other) : base_t{ nullopt } 
			{
				base_t::construct_from_other(static_cast<Base&&>(other));
			}

			enable_move_construct& operator=(const enable_move_construct&) = default;
			enable_move_construct& operator=(enable_move_construct&&) = default;
		};

		/**
		* @brief ムーブ構築のチェック
		* @detail Tが非トリビアルにムーブ構築可能であるとき、ムーブコンストラクタを定義する
		*/
		template<typename Base, typename T>
		using check_move_construct = std::conditional_t<
			std::conjunction<std::is_move_constructible<T>, std::negation<std::is_trivially_move_constructible<T>>>::value,
			enable_move_construct<Base, T>,
			check_copy_construct<Base, T>
		>;

		/**
		* @brief コピー代入をdelete
		*/
		template<typename Base, typename T, bool = std::is_copy_constructible <T>::value, bool = std::is_copy_assignable<T>::value>
		struct enable_copy_assign : check_move_construct<Base, T> {
			using base_t = check_move_construct<Base, T>;
			using base_t::base_t;

			template<typename... Args>
			constexpr enable_copy_assign(Args&&... args) noexcept(std::is_nothrow_constructible<base_t, Args&&...>::value)
				: base_t{ std::forward<Args>(args)... }
			{}

			enable_copy_assign(const enable_copy_assign&) = default;
			enable_copy_assign(enable_copy_assign&&) = default;
			enable_copy_assign& operator=(const enable_copy_assign&) = delete;
			//Tはコピー構築/代入どちらかが不可なので、optional<T>のコピー代入は不可
			enable_copy_assign& operator=(enable_copy_assign&&) = default;
		};

		/**
		* @brief コピー代入を定義
		* @detail Tがコピー構築可能であり、同時にコピー代入可能であるとき
		*/
		template<typename Base, typename T>
		struct enable_copy_assign<Base, T, true, true> : check_move_construct<Base, T> {
			using base_t = check_move_construct<Base, T>;
			using base_t::base_t;

			template<typename... Args>
			constexpr enable_copy_assign(Args&&... args) noexcept(std::is_nothrow_constructible<base_t, Args&&...>::value)
				: base_t{ std::forward<Args>(args)... }
			{}

			enable_copy_assign(const enable_copy_assign&) = default;
			enable_copy_assign(enable_copy_assign&&) = default;
			enable_copy_assign& operator=(const enable_copy_assign& other) noexcept(std::conjunction<std::is_nothrow_copy_assignable<T>, std::is_nothrow_copy_constructible<T>>::value) {
				if (this != &other) {
					base_t::assign_from_other(static_cast<const Base&>(other));
				}
				return *this;
			}

			enable_copy_assign& operator=(enable_copy_assign&& other) = default;
		};

		/**
		* @brief コピー代入のチェック
		*/
		template<typename Base, typename T>
		using check_copy_assign = std::conditional_t<
			std::conjunction<std::is_trivially_destructible<T>, std::is_trivially_copy_constructible<T>, std::is_trivially_copy_assignable<T>>::value,
			check_move_construct<Base, T>,
			enable_copy_assign<Base, T>
		>;

		/**
		* @brief ムーブ代入をdelete
		*/
		template<typename Base, typename T, bool = std::conjunction<std::is_move_constructible<T>, std::is_move_assignable<T>>::value>
		struct enable_move_assign : public check_copy_assign<Base, T> {
			using base_t = check_copy_assign<Base, T>;
			using base_t::base_t;

			template<typename... Args>
			constexpr enable_move_assign(Args&&... args) noexcept(std::is_nothrow_constructible<base_t, Args&&...>::value)
				: base_t{ std::forward<Args>(args)... }
			{}

			enable_move_assign(const enable_move_assign&) = default;
			enable_move_assign(enable_move_assign&&) = default;
			enable_move_assign& operator=(const enable_move_assign&) = default;
			//Tはムーブ構築/代入どちらかが不可なので、optional<T>のムーブ代入は不可
			enable_move_assign& operator=(enable_move_assign&&) = delete;
		};

		/**
		* @brief ムーブ代入の定義
		*/
		template<typename Base, typename T>
		struct enable_move_assign<Base, T, true> : public check_copy_assign<Base, T> {
			using base_t = check_copy_assign<Base, T>;
			using base_t::base_t;

			template<typename... Args>
			constexpr enable_move_assign(Args&&... args) noexcept(std::is_nothrow_constructible<base_t, Args&&...>::value)
				: base_t{ std::forward<Args>(args)... }
			{}

			enable_move_assign(const enable_move_assign&) = default;
			enable_move_assign(enable_move_assign&&) = default;
			enable_move_assign& operator=(const enable_move_assign&) = default;

			enable_move_assign& operator=(enable_move_assign&& other) noexcept(std::conjunction<std::is_nothrow_move_assignable<T>, std::is_nothrow_move_constructible<T>>::value) {
				if (this != &other) {
					base_t::assign_from_other(static_cast<Base&&>(other));
				}
				return *this;
			}
		};

		/**
		* @brief optional<T>の特殊メンバ関数を有効化する（デストラクタ以外）
		* @detail 下から上へあがっていきます
		*/
		template<typename Base, typename T>
		using enable_special_menber_functions = std::conditional_t<
			std::conjunction<std::is_trivially_destructible<T>, std::is_trivially_move_constructible<T>, std::is_trivially_move_assignable<T>>::value,
			check_copy_assign<Base, T>,
			enable_move_assign<Base, T>
		>;
	}

	namespace detail {

		/**
		* @brief std::optionalの実装のためのストレージ領域
		* @detail trivially_destructibleでない型のための処理を提供
		* @tparam T 格納する要素型
		*/
		template<typename T, bool = std::is_trivially_destructible<T>::value>
		struct optional_storage {
			using hold_type = std::remove_const_t<T>;

			union {
				unsigned char m_dummy;
				hold_type m_value;
			};

			bool m_has_value = false;

			constexpr optional_storage(nullopt_t) noexcept
				: m_dummy{}
				, m_has_value{ false }
			{}

			template<typename... Args>
			constexpr optional_storage(Args&&... args) noexcept(std::is_nothrow_constructible<hold_type, Args&&...>::value)
				: m_value(std::forward<Args>(args)...)
				, m_has_value{ true }
			{}

			/**
			* @brief デストラクタ
			* @detail nothrow_destructibleな型を前提とするのでnoexcept
			*/
			~optional_storage() noexcept {
				if (m_has_value == true) {
					m_value.~hold_type();
				}
			}

			//デストラクタを明示宣言してしまっているので、これらも明示宣言しないと暗黙delete
			optional_storage(const optional_storage&) = default;
			optional_storage(optional_storage&&) = default;
			optional_storage& operator=(const optional_storage&) = default;
			optional_storage& operator=(optional_storage&&) = default;

			/**
			* @brief optionalを無効値保持状態にする
			* @detail 保持する値を破棄し、has_value() == false となる
			*/
			void reset() noexcept {
				if (m_has_value == true) {
					m_value.~hold_type();
					m_has_value = false;
				}
			}
		};

		/**
		* @brief std::optionalの実装のためのストレージ領域
		* @detail trivially_destructibleな型に最適化された処理を提供
		* @tparam T 格納する要素型
		*/
		template<typename T>
		struct optional_storage<T, true> {
			using hold_type = std::remove_const_t<T>;

			union {
				unsigned char m_dummy;
				hold_type m_value;
			};

			bool m_has_value = false;

			constexpr optional_storage(nullopt_t) noexcept
				: m_dummy{}
				, m_has_value{ false }
			{}

			template<typename... Args>
			constexpr optional_storage(Args&&... args) noexcept(std::is_nothrow_constructible<hold_type, Args&&...>::value)
				: m_value(std::forward<Args>(args)...)
				, m_has_value{ true }
			{}

			/**
			* @brief optionalを無効値保持状態にする
			* @detail has_value() == false となる
			*/
			void reset() noexcept {
				m_has_value = false;
				//trivially destructibleな型はデストラクタ呼び出しの必要がない
				//領域はこのoptionalオブジェクトの寿命終了とともに解放される
			}
		};

		/**
		* @brief std::optionalの実装のための土台
		* @detail 共通の構築・代入に関わる処理を提供する
		* @tparam T 格納する要素型
		*/
		template<typename T>
		struct optional_common_base : optional_storage<T> {
			using optional_storage<T>::optional_storage;

			template<typename... Args>
			constexpr optional_common_base(Args&&... args) noexcept(std::is_nothrow_constructible<optional_storage<T>, Args&&...>::value)
				: optional_storage<T>{ std::forward<Args>(args)... }
			{}

			/**
			* @brief 領域の遅延初期化
			* @detail 事前条件として、m_has_value == falseであること（呼び出し側で保証する）
			* @return 初期化したオブジェクトへの参照
			*/
			template<typename... Args>
			auto construct(Args&&... args) noexcept(std::is_nothrow_constructible<hold_type, Args&&...>::value) -> hold_type& {
				//placement new
				::new (const_cast<void*>(static_cast<const volatile void*>(std::addressof(m_value)))) hold_type(std::forward<Args>(args)...);

				m_has_value = true;
				return m_value;
			}

			/**
			* @brief 他optional<T>の値からのコピー/ムーブ代入
			* @detail 事前条件として、m_has_value == falseであること（呼び出し側で保証する）
			*/
			template<typename U>
			void assign(U&& rhs) {
				if (this->m_has_value) {
					this->m_value = std::forward<U>(rhs);
				}
				else {
					construct(std::forward<U>(rhs));
				}
			}

			/**
			* @brief 他optional<T>からのコピー/ムーブ構築
			* @detail 事前条件として、m_has_value == falseであること（呼び出し側で保証する）
			*/
			template<typename Optional>
			void construct_from_other(Optional&& that) {
				if (that.m_has_value) {
					construct(std::forward<Optional>(that).m_value);
				}
			}

			/**
			* @brief 他optional<T>からのコピー/ムーブ代入
			*/
			template<typename Optional>
			void assign_from_other(Optional&& that) {
				if (that.m_has_value) {
					assign(std::forward<Optional>(that).m_value);
				}
				else {
					this->reset();
				}
			}

			/**
			* @brief swap実装、ストレージごと入れ替える
			*/
			template<typename U=T, optional_traits::enabler<optional_traits::is_trivially_swappable<U>> = nullptr>
			void swap_impl(optional_common_base<U>& rhs) {
				using storage = optional_storage<T>;

				//どちらか片方が有効値を保持している場合に入れ替え
				if (m_has_value || rhs.m_has_value) {
					std::swap(static_cast<storage&>(*this), static_cast<storage&>(rhs));
				}
			}

			/**
			* @brief swap実装、きちんとswap
			*/
			template<typename U, optional_traits::enabler<std::negation<optional_traits::is_trivially_swappable<U>>> = nullptr>
			void swap_impl(optional_common_base<U>& rhs) {
				using std::swap;

				if (m_has_value) {
					if (rhs.m_has_value) {
						//両方有効値を保持している
						swap(m_value, rhs.m_value);
					}
					else {
						//rhsが有効値を保持していない
						rhs.construct(std::move(m_value));
						reset();
					}
				}
				else if (rhs.m_has_value) {
					//*thisが有効値を保持しない
					construct(std::move(rhs.m_value));
					rhs.reset();
				}
				//両方が有効値を保持していない
			}
		};
	}

	/**
	* @brief std::optional、実装
	* @detail 無効値を保持可能な型、maybeモナド
	* @tparam T 格納する要素型、オブジェクト型であり、デストラクタが例外を投げずに実行可能であること
	*/
	template<typename T>
	class optional : private detail::enable_special_menber_functions<detail::optional_common_base<T>, T> {
		
		using base_storage = detail::enable_special_menber_functions<detail::optional_common_base<T>, T>;

	public:
		
		static_assert(std::conjunction<std::is_object<T>, std::is_nothrow_destructible<T>>::value, "T shall be an object type and shall satisfy the requirements of Destructible. (N4659 23.6.3 [optional.optional]/3)");
		
		using value_type = T;

		/**
		* @brief 無効値を持つoptional初期化
		*/
		constexpr optional() noexcept : base_storage{ nullopt }
		{}

		/**
		* @brief 無効値を持つoptional初期化
		*/
		constexpr optional(nullopt_t) noexcept : base_storage{ nullopt }
		{}

		/**
		* @brief コピーコンストラクタ
		* @detail Tがtrivially destructibleであればconstexprかつnoexcept
		*/
		optional(const optional&) = default;

		/**
		* @brief ムーブコンストラクタ
		* @detail Tがtrivially destructibleであればconstexprかつnoexcept
		*/
		optional(optional&&) = default;

		/**
		* @brief Tのコンストラクタ引数を受けて直接構築
		* @param args Tに与える引数
		*/
		template<typename... Args, optional_traits::enabler<std::is_constructible<T, Args&&...>> = nullptr>
		constexpr explicit optional(in_place_t, Args&&... args) : base_storage{ std::forward<Args>(args)... }
		{}

		/**
		* @brief Tのコンストラクタ引数を受けて直接構築
		* @detail 主に、STLコンテナ等の持つアロケータを受け取るコンストラクタを呼び出すためのもの
		* @param il Tに与える初期化リスト
		* @param args その他の引数
		*/
		template<typename U, typename... Args, optional_traits::enabler<std::is_constructible<T, std::initializer_list<U>&, Args&&...>> = nullptr>
		constexpr explicit optional(in_place_t, std::initializer_list<U> il, Args&&... args) : base_storage{ il, std::forward<Args>(args)... }
		{}

		/**
		* @brief Tを初期化可能なUの値を受けて構築
		* @detail allow_conversion<T, U> = true かつ is_convertible<U&&, T> = true の時にのみオーバーロードに参加
		* @detail T→Uへ暗黙変換可能な場合のコンストラクタ
		* @param value Tに変換可能なUの値
		*/
		template<typename U = T, optional_traits::enabler<optional_traits::allow_conversion<T, U>, std::is_convertible<U&&, T>> = nullptr>
		constexpr optional(U&& value) : base_storage{ std::forward<U>(value) }
		{}

		/**
		* @brief Tを初期化可能なUの値を受けて構築
		* @detail allow_conversion<T, U> = true かつ is_convertible<U&&, T> = false の時にのみオーバーロードに参加
		* @detail T→Uへ暗黙変換不可能な場合のコンストラクタ（explicitが付く）
		* @param value Tに変換可能なUの値
		*/
		template<typename U = T, optional_traits::enabler<optional_traits::allow_conversion<T, U>, std::negation<std::is_convertible<U&&, T>>> = nullptr>
		constexpr explicit optional(U&& value) : base_storage{ std::forward<U>(value) }
		{}

		/**
		* @brief Tに変換可能なUを持つoptionalからコピー構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, const U&> = true かつ is_convertible<const U&&, T> == true の時にのみオーバーロードに参加
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, const U&>, std::is_convertible<const U&&, T>> = nullptr>
		optional(const optional<U>& rhs) noexcept(noexcept(construct(*rhs))) {
			if (rhs) {
				construct(*rhs);
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからコピー構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, const U&> = true かつ is_convertible<const U&&, T> == false の時にのみオーバーロードに参加（explicitが付くだけ）
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, const U&>, std::negation<std::is_convertible<const U&&, T>>> = nullptr>
		explicit optional(const optional<U>& rhs) noexcept(noexcept(construct(*rhs))) {
			if (rhs) {
				construct(*rhs);
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからムーブ構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, U&&> = true かつ is_convertible<U&&, T> == true の時にのみオーバーロードに参加
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, U&&>, std::is_convertible<U&&, T>> = nullptr>
		optional(optional<U>&& rhs) noexcept(noexcept(construct(std::move(*rhs)))) {
			if (rhs) {
				construct(std::move(*rhs));
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからムーブ構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, U&&> = true かつ is_convertible<U&&, T> == false の時にのみオーバーロードに参加（explicitが付くだけ）
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, U&&>, std::negation<std::is_convertible<U&&, T>>> = nullptr>
		explicit optional(optional<U>&& rhs) noexcept(noexcept(construct(std::move(*rhs)))) {
			if (rhs) {
				construct(std::move(*rhs));
			}
		}

		/**
		* @brief nulloptを代入する
		* @detail 保持する値を開放する
		* @return *this
		*/
		optional& operator=(nullopt_t) noexcept {
			this->reset();
			return *this;
		}

		/**
		* @brief コピー代入
		* @detail Tがtrivially destructibleであればconstexprかつnoexcept
		* @return *this
		*/
		optional& operator=(const optional&) = default;

		/**
		* @brief ムーブ代入
		* @detail Tがtrivially destructibleであればconstexprかつnoexcept
		* @return *this
		*/
		optional& operator=(optional&&) = default;

		/**
		* @brief Tに変換可能な値の代入
		* @return *this
		*/
		template<typename U = T, optional_traits::enabler<optional_traits::allow_conversion_assign<T, U>> = nullptr>
		optional& operator=(U&& v) {
			assign(std::forward<U>(v));
			return *this;
		}

		/**
		* @brief Tに変換可能なUを持つoptional<U>からのコピー代入
		* @return *this
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap_assign<T, U>, std::is_constructible<T, const U&>, std::is_assignable<T&, const U&>> = nullptr>
		optional& operator=(const optional<U>& rhs) {
			if (rhs) {
				assign(*rhs);
			}
			else {
				reset();
			}

			return *this;
		}

		/**
		* @brief Tに変換可能なUを持つoptional<U>からのムーブ代入
		* @return *this
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap_assign<T, U>, std::is_constructible<T, U>, std::is_assignable<T&, U>> = nullptr>
		optional& operator=(optional<U>&& rhs) {
			if (rhs) {
				assign(std::move(*rhs));
			}
			else {
				reset();
			}


			return *this;
		}

		/**
		* @brief Tのコンストラクタ引数から直接構築する。
		* @param args Tの構築に必要な引数列
		* @return 構築した要素への参照
		*/
		template<typename... Args>
		T& emplace(Args&&... args) {
			reset();
			return construct(std::forward<Args>(args)...);
		}

		/**
		* @brief Tのコンストラクタ引数から直接構築する。
		* @detail Tがinitializer_list<U>とArgs...から構築可能であるときのみオーバーロードに参加
		* @param il Tに与える初期化リスト
		* @param args Tの構築に必要な追加の引数列
		* @return 構築した要素への参照
		*/
		template<typename U, typename... Args, optional_traits::enabler<std::is_constructible<T, std::initializer_list<U>&, Args&&...>> = nullptr>
		T& emplace(std::initializer_list<U> il, Args&&... args) {
			reset();
			return construct(il, std::forward<Args>(args)...);
		}

		/**
		* @brief 他のoptional<T>とデータを入れ替える
		* @tparam U U=Tでなければならない
		* @detail Tがムーブ構築可能かつSwappableであること
		* @param rhs swapするoptional
		*/
		template<typename U = T, optional_traits::enabler<std::is_same<T, U>, std::is_move_constructible<T>, is_swappable<T>> = nullptr>
		void swap(optional<U>& rhs) noexcept(std::conjunction<std::is_nothrow_move_constructible<T>, is_nothrow_swappable<T>>::value) {
			//効率的なswapを選択するために、base_storageへ投げる
			swap_impl(rhs);
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値はvoidでないこと
		* @param func 適用するINVOKE可能な関数
		* @return 有効値を保持する場合、f(this->value())の戻り値をoptionalで包んで返す、そうでないならnullopt
		*/
		template<typename F>
		auto transform(F&& func) & noexcept(noexcept(func(m_value))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(m_value));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値はvoidでないこと
		* @param func 適用するINVOKE可能な関数
		* @return 有効値を保持する場合、f(this->value())の戻り値をoptionalで包んで返す、そうでないならnullopt
		*/
		template<typename F>
		constexpr auto transform(F&& func) const & noexcept(noexcept(func(m_value))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(m_value));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値はvoidでないこと
		* @param func 適用するINVOKE可能な関数
		* @return 有効値を保持する場合、f(std::move(this->value()))の戻り値をoptionalで包んで返す、そうでないならnullopt
		*/
		template<typename F>
		auto transform(F&& func) && noexcept(noexcept(func(std::move(m_value)))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(std::move(m_value)));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値はvoidでないこと
		* @param func 適用するINVOKE可能な関数
		* @return 有効値を保持する場合、f(std::move(this->value()))の戻り値をoptionalで包んで返す、そうでないならnullopt
		*/
		template<typename F>
		constexpr auto transform(F&& func) const && noexcept(noexcept(func(std::move(m_value)))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(std::move(m_value)));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値が何らかのoptionalであること
		* @param func 適用するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、f(this->value())、そうでないならnullopt
		*/
		template<typename F>
		auto and_then(F&& func) & noexcept(noexcept(func(m_value))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(m_value));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値が何らかのoptionalであること
		* @param func 適用するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、f(this->value())、そうでないならnullopt
		*/
		template<typename F>
		constexpr auto and_then(F&& func) const & noexcept(noexcept(func(m_value))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(m_value));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値が何らかのoptionalであること
		* @param func 適用するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、f(std::move(this->value()))、そうでないならnullopt
		*/
		template<typename F>
		auto and_then(F&& func) && noexcept(noexcept(func(std::move(m_value)))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(std::move(m_value)));
		}

		/**
		* @brief 中身に関数を適用しその結果をoptionalで返す
		* @detail 渡される関数の戻り値が何らかのoptionalであること
		* @param func 適用するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、f(this->value())、そうでないならnullopt
		*/
		template<typename F>
		constexpr auto and_then(F&& func) const && noexcept(noexcept(func(std::move(m_value)))) -> optional_traits::invoke_result_t<F, T> {
			return (m_has_value == false)
				? (nullopt)
				: (func(std::move(m_value)));
		}

		/**
		* @brief 中身がない時に関数を実行しその結果のoptionalを返す
		* @detail 渡される関数の戻り値が同じoptionalかvoidであること
		* @param func 実行するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、*this、そうでないならfunc()
		*/
		template<typename F>
		optional or_else(F&& func) & noexcept(noexcept(func())) {
			return (m_has_value == true)
				? (*this)
				: ((std::is_void<optional_traits::invoke_result_t<F>>::value)
					? (func(), nullopt)
					: (func()));
		}

		/**
		* @brief 中身がない時に関数を実行しその結果のoptionalを返す
		* @detail 渡される関数の戻り値が同じoptionalかvoidであること
		* @param func 実行するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、*this、そうでないならfunc()
		*/
		template<typename F>
		constexpr optional or_else(F&& func) const & noexcept(noexcept(func())) {
			return (m_has_value == true)
				? (*this)
				: ((std::is_void<optional_traits::invoke_result_t<F>>::value)
					? (func(), nullopt)
					: (func()));
		}

		/**
		* @brief 中身がない時に関数を実行しその結果のoptionalを返す
		* @detail 渡される関数の戻り値が同じoptionalかvoidであること
		* @param func 実行するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、std::move(*this)、そうでないならfunc()
		*/
		template<typename F>
		optional or_else(F&& func) && noexcept(noexcept(func())) {
			return (m_has_value == true)
				? std::move(*this)
				: ((std::is_void<optional_traits::invoke_result_t<F>>::value)
					? (func(), nullopt)
					: (func()));
		}

		/**
		* @brief 中身がない時に関数を実行しその結果のoptionalを返す
		* @detail 渡される関数の戻り値が同じoptionalかvoidであること
		* @param func 実行するINVOKE可能な関数（戻り値がoptionalであること）
		* @return 有効値を保持する場合、std::move(*this)、そうでないならfunc()
		*/
		template<typename F>
		constexpr optional or_else(F&& func) const && noexcept(noexcept(func())) {
			return (m_has_value == true)
				? std::move(*this)
				: ((std::is_void<optional_traits::invoke_result_t<F>>::value)
					? (func(), nullopt)
					: (func()));
		}

		constexpr const T* operator->() const {
			return std::addressof(m_value);
		}

		T* operator->() {
			return std::addressof(m_value);
		}

		T& operator*() & {
			return m_value;
		}

		T&& operator*() && {
			return std::move(m_value);
		}

		constexpr const T& operator*() const & {
			return m_value;
		}

		constexpr const T&& operator*() const && {
			return std::move(m_value);
		}

		T& value() & {
			return (m_has_value) ? m_value : (throw bad_optional_access{}, m_value);
		}

		T&& value() && {
			return (m_has_value) ? std::move(m_value) : (throw bad_optional_access{}, m_value);
		}

		constexpr const T& value() const & {
			return (m_has_value) ? m_value : (throw bad_optional_access{}, m_value);
		}

		constexpr const T&& value() const && {
			return (m_has_value) ? std::move(m_value) : (throw bad_optional_access{}, m_value);
		}


		/**
		* @brief 内部の値か、それが無ければ指定した値を返す
		* @detail Tが_copy_constructible　かつ　U&& → Tへ変換可能であること
		* @param v 無効値だった時に返す値
		*/
		template<typename U, optional_traits::enabler<std::is_copy_constructible<T>, std::is_convertible<U&&, T>> = nullptr>
		constexpr T value_or(U&& v) const & {
			//static_assert(std::conjunction<std::is_copy_constructible<T>, std::is_convertible<U&&, T>>::value, "If is_­copy_­constructible_­v<T> && is_­convertible_­v<U&&, T> is false, the program is ill-formed. (N4659 23.6.3.5 [optional.observe]/18)");
			return (m_has_value) ? m_value : static_cast<T>(std::forward<U>(v));
		}

		/**
		* @brief 内部の値か、それが無ければ指定した値を返す（右辺値用）
		* @detail Tが_copy_constructible　かつ　U&& → Tへ変換可能であること
		* @param v 無効値だった時に返す値
		*/
		template<typename U, optional_traits::enabler<std::is_copy_constructible<T>, std::is_convertible<U&&, T>> = nullptr>
		constexpr T value_or(U&& v) const && {
			//static_assert(std::conjunction<std::is_move_constructible<T>, std::is_convertible<U&&, T>>::value, "If is_­copy_­constructible_­v<T> && is_­convertible_­v<U&&, T> is false, the program is ill-formed. (N4659 23.6.3.5 [optional.observe]/18)");
			return (m_has_value) ? std::move(m_value) : static_cast<T>(std::forward<U>(v));
		}

		constexpr explicit operator bool() const noexcept {
			return m_has_value;
		}

		constexpr bool has_value() const noexcept {
			return m_has_value;
		}

		using base_storage::reset;
	};


	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @return 片方だけ有効ならfalse、両方とも有効でないならtrue、両方有効なら中身を比較（==）
	*/
	template<typename T, typename U>
	constexpr bool operator==(const optional<T>& lhs, const optional<U>& rhs) {
		//片方だけが有効値を保持する場合はfalse
		//両方とも有効値を保持しないならtrue
		//両方とも有効値を保持するならばそれを比較
		return (bool(lhs) != bool(rhs))
			? (false)
			: ((bool(lhs) == false)
				? (true)
				: (*lhs == *rhs));
	}

	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @return 片方だけ有効ならtrue、両方とも有効でないならfalse、両方有効なら中身を比較（!=）
	*/
	template<typename T, typename U>
	constexpr bool operator!=(const optional<T>& lhs, const optional<U>& rhs) {
		return !(lhs == rhs);
	}

	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @detail 比較において、nulloptは無限小であるかのようにふるまう
	* @return 右辺が無効値ならfalse、左辺が無効値ならtrue、両方有効なら中身を比較（<）
	*/
	template<typename T, typename U>
	constexpr bool operator<(const optional<T>& lhs, const optional<U>& rhs) {
		//右辺が無効値ならfalse
		//左辺が無効値ならtrue
		//両方とも有効ならそれを比較
		return (bool(rhs) == false)
			? (false)
			: (bool(lhs) == false
				? (true)
				: (*lhs < *rhs));
	}

	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @detail 比較において、nulloptは無限小であるかのようにふるまう
	* @return 右辺が無効値ならfalse、左辺が無効値ならtrue、両方有効なら中身を比較（>）
	*/
	template<typename T, typename U>
	constexpr bool operator>(const optional<T>& lhs, const optional<U>& rhs) {
		return rhs < lhs;
	}

	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @detail 比較において、nulloptは無限小であるかのようにふるまう
	* @return 右辺が無効値ならtrue、左辺が無効値ならfalse、両方有効なら中身を比較（<=）
	*/
	template<typename T, typename U>
	constexpr bool operator<=(const optional<T>& lhs, const optional<U>& rhs) {
		return !(rhs < lhs);
	}

	/**
	* @brief 比較可能なoptional同士の比較
	* @param lhs 右辺
	* @param rhs 左辺
	* @detail 比較において、nulloptは無限小であるかのようにふるまう
	* @return 右辺が無効値ならtrue、左辺が無効値ならfalse、両方有効なら中身を比較（>=）
	*/
	template<typename T, typename U>
	constexpr bool operator>=(const optional<T>& lhs, const optional<U>& rhs) {
		return !(lhs < rhs);
	}


	/**
	* @brief nulloptとの比較
	* @param x 左辺
	* @return !x.has_value()
	*/
	template<typename T>
	constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept {
		return !bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @param x 右辺
	* @return !x.has_value()
	*/
	template<typename T>
	constexpr bool operator==(nullopt_t, const optional<T>& x) noexcept {
		return !bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @param x 比較するoptional
	* @return x.has_value()
	*/
	template<typename T>
	constexpr bool operator!=(const optional<T>& x, nullopt_t) noexcept {
		return bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @param x 比較するoptional
	* @return x.has_value()
	*/
	template<typename T>
	constexpr bool operator!=(nullopt_t, const optional<T>& x) noexcept {
		return bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return false
	*/
	template<typename T>
	constexpr bool operator<(const optional<T>& x, nullopt_t) noexcept {
		return false;
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return x.has_value()
	*/
	template<typename T>
	constexpr bool operator<(nullopt_t, const optional<T>& x) noexcept {
		return bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return x.has_value()
	*/
	template<typename T>
	constexpr bool operator>(const optional<T>& x, nullopt_t) noexcept {
		return bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return false
	*/
	template<typename T>
	constexpr bool operator>(nullopt_t, const optional<T>& x) noexcept {
		return false;
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return !x.has_value()
	*/
	template<typename T>
	constexpr bool operator<=(const optional<T>& x, nullopt_t) noexcept {
		return !bool(x);
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return true
	*/
	template<typename T>
	constexpr bool operator<=(nullopt_t, const optional<T>& x) noexcept {
		return true;
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return true
	*/
	template<typename T>
	constexpr bool operator>=(const optional<T>& x, nullopt_t) noexcept {
		return true;
	}

	/**
	* @brief nulloptとの比較
	* @detail nulloptは無限小であるかのようにふるまう
	* @param x 比較するoptional
	* @return !x.has_value()
	*/
	template<typename T>
	constexpr bool operator>=(nullopt_t, const optional<T>& x) noexcept {
		return !bool(x);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（==）
	*/
	template<typename T, typename U>
	constexpr bool operator==(const optional<T>& x, const U& v) {
		return (bool(x)) ? (*x == v) : false;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（==）
	*/
	template<typename T, typename U>
	constexpr bool operator==(const U& v, const optional<T>& x) {
		return (bool(x)) ? (*x == v) : false;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（!=）
	*/
	template<typename T, typename U>
	constexpr bool operator!=(const optional<T>& x, const U& v) {
		return !(x == v);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（!=）
	*/
	template<typename T, typename U>
	constexpr bool operator!=(const U& v, const optional<T>& x) {
		return !(x == v);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（<）
	*/
	template<typename T, typename U>
	constexpr bool operator<(const optional<T>& x, const U& v) {
		return (bool(x)) ? (*x < v) : true;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（<）
	*/
	template<typename T, typename U>
	constexpr bool operator<(const U& v, const optional<T>& x) {
		return (bool(x)) ? (v < *x) : false;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（>）
	*/
	template<typename T, typename U>
	constexpr bool operator>(const optional<T>& x, const U& v) {
		return v < x;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（>）
	*/
	template<typename T, typename U>
	constexpr bool operator>(const U& v, const optional<T>& x) {
		return x < v;
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（<=）
	*/
	template<typename T, typename U>
	constexpr bool operator<=(const optional<T>& x, const U& v) {
		return !(v < x);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（<=）
	*/
	template<typename T, typename U>
	constexpr bool operator<=(const U& v, const optional<T>& x) {
		return !(x < v);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならfalse、有効なら中身とvを比較（>=）
	*/
	template<typename T, typename U>
	constexpr bool operator>=(const optional<T>& x, const U& v) {
		return !(x < v);
	}

	/**
	* @brief optional<T>と素の値の比較
	* @param x 比較するoptional
	* @param v 比較するオブジェクト
	* @return xが無効値ならtrue、有効なら中身とvを比較（>=）
	*/
	template<typename T, typename U>
	constexpr bool operator>=(const U& v, const optional<T>& x) {
		return !(v < x);
	}

	//参照型、nullopt_tとin_place_t及びそのCV修飾はoptionalの要素になれない

	template<typename T>
	struct optional<T&>;

	template<typename T>
	struct optional<T&&>;

	template<>
	struct optional<nullopt_t>;

	template<>
	struct optional<const nullopt_t>;

	template<>
	struct optional<const volatile nullopt_t>;

	template<>
	struct optional<volatile nullopt_t>;

	template<>
	struct optional<in_place_t>;

	template<>
	struct optional<const in_place_t>;

	template<>
	struct optional<const volatile in_place_t>;

	template<>
	struct optional<volatile in_place_t>;

	/**
	* @brief optional同士のデータを入れ替える
	* @detail Tがムーブ構築可能かつSwappableであること
	* @detail x.swap(y)を行う
	* @param x swapするoptional
	* @param y swapするoptional
	*/
	template<typename T, optional_traits::enabler<std::is_move_constructible<T>, is_swappable<T>> = nullptr>
	void swap(optional<T>& x, optional<T>& y) noexcept(noexcept(x.swap(y))) {
		x.swap(y);
	}

	/**
	* @brief optionalを構築する
	* @param v T型の初期値
	* @return vを保持するoptional<T>
	*/
	template<typename T>
	constexpr auto make_optional(T&& v) -> optional<std::decay_t<T>> {
		return optional<std::decay_t<T>>{ std::forward<T>(v) };
	}

	/**
	* @brief optionalを構築する
	* @param args Tの構築に必要な引数列
	* @return T{std::forward<Args>(args)...}を保持するoptional<T>
	*/
	template<typename T, typename... Args>
	constexpr auto make_optional(Args&&... args) -> optional<T> {
		return optional<T>{ in_place, std::forward<Args>(args)... };
	}

	/**
	* @brief optionalを構築する
	* @param il Tの構築に必要なinitializer_list<U>
	* @param args Tの構築に必要な追加の引数
	* @return T{il, std::forward<Args>(args)...}を保持するoptional<T>
	*/
	template<typename T, typename U, typename... Args>
	constexpr auto make_optional(std::initializer_list<U> il, Args&&... args) -> optional<T> {
		return optional<T>{ in_place, il, std::forward<Args>(args)... };
	}
}

//警告抑止の解除
#pragma warning(pop)