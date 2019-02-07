#pragma once

#include <type_traits>
#include <exception>

namespace lstl {

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


	namespace detail {

		template<typename T, bool = std::is_trivially_destructible<T>::value>
		struct optional_storage {
			union {
				unsigned char m_dummy;
				std::remove_const_t<T> m_value;
			};

			bool m_has_value = false;

			constexpr optional_storage(nullopt_t)
				: m_dummy{}
				, m_has_value{ false }
			{}

			template<typename... Args>
			constexpr optional_storage(Args&&... args)
				: m_value{ std::forward<Args>(args)... }
				, m_has_value{ true }
			{}

			~optional_storage() {
				if (m_has_value == true) {
					m_value.~T();
				}
			}

			void reset() noexcept {
				if (m_has_value == true) {
					m_value.~T();
					m_has_value = false;
				}
			}
		};

		template<typename T>
		struct optional_storage<T, true> {
			union {
				unsigned char m_dummy;
				std::remove_const_t<T> m_value;
			};

			bool m_has_value = false;

			constexpr optional_storage(nullopt_t)
				: m_dummy{}
				, m_has_value{ false }
			{}

			template<typename... Args>
			constexpr optional_storage(Args&&... args)
				: m_value{ std::forward<Args>(args)... }
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

		constexpr optional() : base_storage{ nullopt }
		{}

		constexpr optional(nullopt_t) : base_storage{ nullopt }
		{}

		template<typename U = T>
		constexpr explicit optional(U&& value) : base_storage{ std::forward<U>(value) }
		{}

		T& operator*() & {
			return m_value;
		}

		constexpr const T& operator*() const & {
			return m_value;
		}

		T&& operator*() && {
			return std::move(m_value);
		}

		constexpr const T&& operator*() const && {
			return std::move(m_value);
		}

		T& value() & {
			if (m_has_value) {
				return m_value;
			}

			throw bad_optional_access{};
		}

		T&& value() && {
			if (m_has_value) {
				return std::move(m_value);
			}

			throw bad_optional_access{};
		}

		constexpr const T& value() const & {
			if (m_has_value) {
				return m_value;
			}

			throw bad_optional_access{};
		}

		constexpr const T&& value() const && {
			if (m_has_value) {
				return std::move(m_value);
			}

			throw bad_optional_access{};
		}

		constexpr explicit operator bool() const noexcept {
			return m_has_value;
		}

		constexpr bool has_value() const noexcept {
			return bool(*this);
		}

		using base_storage::reset;
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
}