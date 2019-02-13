#pragma once

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
		* @brief 条件の論理積がfalseとなるときに有効化
		* @tparam Traits 条件列、boolに変換可能な::valueを持つ任意のメタ関数
		*/
		template<typename... Traits>
		using disabler = std::enable_if_t<std::conjunction<Traits...>::value == false, std::nullptr_t>;

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
	}

	namespace detail {

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
			constexpr optional_storage(Args&&... args) noexcept(std::is_constructible<hold_type, Args&&...>::value)
				: m_value(std::forward<Args>(args)...)
				, m_has_value{ true }
			{}

			~optional_storage() {
				if (m_has_value == true) {
					m_value.~hold_type();
				}
			}

			//デストラクタを明示宣言してしまっているので、これらも明示宣言しないと暗黙delete
			optional_storage(const optional_storage&) = default;
			optional_storage(optional_storage&&) = default;
			optional_storage& operator=(const optional_storage&) = default;
			optional_storage& operator=(optional_storage&&) = default;

			void reset() noexcept {
				if (m_has_value == true) {
					m_value.~hold_type();
					m_has_value = false;
				}
			}
		};

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
			constexpr optional_storage(Args&&... args) noexcept(std::is_constructible<hold_type, Args&&...>::value)
				: m_value(std::forward<Args>(args)...)
				, m_has_value{ true }
			{}

			void reset() noexcept {
				m_has_value = false;
			}
		};
	}

	template<typename T>
	class optional : private detail::optional_storage<T> {
		
		using base_storage = detail::optional_storage<T>;

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

		optional(const optional&) = default;

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
		optional(const optional<U>& rhs) noexcept(noexcept(in_place_construct(*rhs))) {
			if (rhs) {
				in_place_construct(*rhs);
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからコピー構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, const U&> = true かつ is_convertible<const U&&, T> == false の時にのみオーバーロードに参加（explicitが付くだけ）
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, const U&>, std::negation<std::is_convertible<const U&&, T>>> = nullptr>
		explicit optional(const optional<U>& rhs) noexcept(noexcept(in_place_construct(*rhs))) {
			if (rhs) {
				in_place_construct(*rhs);
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからムーブ構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, U&&> = true かつ is_convertible<U&&, T> == true の時にのみオーバーロードに参加
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, U&&>, std::is_convertible<U&&, T>> = nullptr>
		optional(optional<U>&& rhs) noexcept(noexcept(in_place_construct(std::move(*rhs)))) {
			if (rhs) {
				in_place_construct(std::move(*rhs));
			}
		}

		/**
		* @brief Tに変換可能なUを持つoptionalからムーブ構築
		* @detail allow_unwrap<T, U> = true かつ is_constructible<T, U&&> = true かつ is_convertible<U&&, T> == false の時にのみオーバーロードに参加（explicitが付くだけ）
		* @param rhs Tに変換可能なUをもつoptional
		*/
		template<typename U, optional_traits::enabler<optional_traits::allow_unwrap<T, U>, std::is_constructible<T, U&&>, std::negation<std::is_convertible<U&&, T>>> = nullptr>
		explicit optional(optional<U>&& rhs) noexcept(noexcept(in_place_construct(std::move(*rhs)))) {
			if (rhs) {
				in_place_construct(std::move(*rhs));
			}
		}


		optional& operator=(nullopt_t) noexcept {
			this->reset();
			return *this;
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

		template<typename U>
		constexpr T value_or(U&& v) const & {
			static_assert(std::conjunction<std::is_copy_constructible<T>, std::is_convertible<U&&, T>>::value, "If is_­copy_­constructible_­v<T> && is_­convertible_­v<U&&, T> is false, the program is ill-formed. (N4659 23.6.3.5 [optional.observe]/18)");
			return (m_has_value) ? m_value : static_cast<T>(std::forward<U>(v));
		}

		template<typename U>
		T value_or(U&& v) && {
			static_assert(std::conjunction<std::is_move_constructible<T>, std::is_convertible<U&&, T>>::value, "If is_­copy_­constructible_­v<T> && is_­convertible_­v<U&&, T> is false, the program is ill-formed. (N4659 23.6.3.5 [optional.observe]/18)");
			return (m_has_value) ? std::move(m_value) : static_cast<T>(std::forward<U>(v));
		}

		constexpr explicit operator bool() const noexcept {
			return m_has_value;
		}

		constexpr bool has_value() const noexcept {
			return m_has_value;
		}

		using base_storage::reset;

	private:

		/**
		* @brief 領域の遅延初期化
		* @detail 事前条件として、m_has_value == falseであること（呼び出し側で保証する）
		* @return 初期化したオブジェクトへの参照
		*/
		template<typename... Args>
		auto in_place_construct(Args&&... args) noexcept(std::is_nothrow_constructible<hold_type, Args&&...>::value) -> hold_type& {
			//placement new (msvc 2019 preview2の_Construct_in_place関数を参考)
			::new (const_cast<void*>(static_cast<const volatile void*>(std::addressof(m_value)))) hold_type(std::forward<Args>(args)...);

			m_has_value = true;
			return m_value;
		}
	};


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