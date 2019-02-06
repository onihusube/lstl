#pragma once

#include <type_traits>

namespace lstl {

	struct nullopt_t {
		constexpr explicit nullopt_t(std::nullptr_t) noexcept {}
	};

	constexpr nullopt_t nullopt{ nullptr };


	namespace detail {

		template<typename T>
		struct optional_storage {
			union storage {
				unsigned char dummy;
				T value;

				constexpr storage(nullopt_t) : dummy{}
				{}

				template<typename... Args>
				constexpr storage(Args&&... args) : value{std::forward<Args>(args)...}
				{}

				~storage() {};
			} m_data;

			bool m_init = false;

			constexpr optional_storage(nullopt_t)
				: m_data{ nullopt }
				, m_init{ false }
			{}

			template<typename... Args>
			constexpr optional_storage(Args&&... args)
				: m_data{ std::forward<Args>(args)... }
				, m_init{ true }
			{}

			~optional_storage() {
				if (m_init == true) {
					m_data.value.~T();
				}
			}
		};

		template<typename T>
		struct constexpr_optional_storage {
			union storage {
				unsigned char dummy;
				T value;

				constexpr storage(nullopt_t) : dummy{}
				{}

				template<typename... Args>
				constexpr storage(Args&&... args) : value{ std::forward<Args>(args)... }
				{}

				~storage() = default;
			} m_data;

			bool m_init = false;

			constexpr constexpr_optional_storage(nullopt_t)
				: m_data{ nullopt }
				, m_init{ false }
			{}

			template<typename... Args>
			constexpr constexpr_optional_storage(Args&&... args)
				: m_data{ std::forward<Args>(args)... }
				, m_init{ true }
			{}

		};

		template<typename T>
		using optional_base = std::conditional_t<std::is_trivially_destructible<T>::value, constexpr_optional_storage<T>, optional_storage<T>>;

	}

	template<typename T>
	struct optional : private detail::optional_base<T> {

		constexpr optional() : detail::optional_base<T>{nullopt}
		{}

		template<typename U=T>
		constexpr explicit optional(U&& value) : detail::optional_base<T>{ std::forward<U>(value) }
		{}

		T& operator*() & {
			return this->m_data.value;
		}

		constexpr const T& operator*() const & {
			return this->m_data.value;
		}

		T&& operator*() && {
			return this->m_data.value;
		}

		constexpr const T&& operator*() const && {
			return this->m_data.value;
		}

		constexpr explicit operator bool() const noexcept {
			return this->m_init;
		}

		void reset() noexcept {
			if (m_init == true) {
				m_data.value.~T();
				m_init = false;
			}
		}
	};


	template<typename T>
	struct optional<T&>;
}