#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

#if defined(_WIN32) ||      \
	defined(_WIN64) ||      \
	defined(__WIN32__) ||   \
	defined(__TOS_WIN__) || \
	defined(__WINDOWS)
#	define MMIO_OS_WINDOWS true
#else
#	define MMIO_OS_WINDOWS false
#endif

namespace mmio
{
	enum class mapmode;
	struct native_handle_type;
	class open_result;
	template <mapmode>
	class mapped_file;

	enum class mapmode
	{
		readonly,
		readwrite
	};

	constexpr auto dynamic_size = static_cast<std::size_t>(-1);

#if MMIO_OS_WINDOWS
	struct native_handle_type final
	{
		static void* const invalid_handle_value;

		void* file{ invalid_handle_value };
		void* file_mapping_object{ nullptr };
		void* base_address{ nullptr };
	};
#else
	struct native_handle_type final
	{
		static void* const map_failed;

		int fd{ -1 };
		void* addr{ map_failed };
	};
#endif

	class open_result final
	{
	public:
		using value_type = std::error_code;

		open_result() = delete;

		[[nodiscard]] explicit operator bool() const noexcept { return this->_error.value() == 0; }
		[[nodiscard]] const value_type& operator*() const noexcept { return this->_error; }
		[[nodiscard]] const value_type* operator->() const noexcept { return &this->_error; }

	private:
		template <mapmode>
		friend class mapped_file;

		open_result(value_type a_error) noexcept :
			_error(std::move(a_error))
		{}

		value_type _error;
	};

	template <mapmode MODE>
	class mapped_file final :
		public std::enable_shared_from_this<mapped_file<MODE>>
	{
	public:
		using value_type =
			std::conditional_t<
				MODE == mapmode::readonly,
				const std::byte,
				std::byte>;

		using iterator = value_type*;

		mapped_file() noexcept = default;
		mapped_file(const mapped_file&) = delete;
		mapped_file(mapped_file&& a_rhs) noexcept { this->do_move(std::move(a_rhs)); }
		mapped_file(std::filesystem::path a_path, std::size_t a_size = dynamic_size);

		~mapped_file() noexcept { this->close(); }

		mapped_file& operator=(const mapped_file&) = delete;
		mapped_file& operator=(mapped_file&& a_rhs) noexcept
		{
			if (this != &a_rhs) {
				this->do_move(std::move(a_rhs));
			}
			return *this;
		}

		[[nodiscard]] auto begin() const noexcept -> iterator { return this->data(); }
		[[nodiscard]] auto end() const noexcept -> iterator { return this->data() + this->size(); }

		void close() noexcept;
		[[nodiscard]] auto data() const noexcept -> value_type*;
		[[nodiscard]] bool empty() const noexcept { return this->size() == 0; }
		[[nodiscard]] bool is_open() const noexcept;

		[[nodiscard]] auto native_handle() const noexcept
			-> const native_handle_type&
		{
			return this->_handle;
		}

		auto open(
			std::filesystem::path a_path,
			std::size_t a_size = dynamic_size) noexcept
			-> open_result;

		[[nodiscard]] auto size() const noexcept -> std::size_t { return this->_size; }

	private:
		void do_move(mapped_file&& a_rhs) noexcept
		{
			this->_handle = std::exchange(a_rhs._handle, native_handle_type{});
			this->_size = std::exchange(a_rhs._size, 0);
		}

		[[nodiscard]] bool do_open(
			const std::filesystem::path::value_type* a_path,
			std::size_t a_size) noexcept;

		native_handle_type _handle;
		std::size_t _size{ 0 };
	};

	extern template class mapped_file<mapmode::readonly>;
	extern template class mapped_file<mapmode::readwrite>;

	using mapped_file_source = mapped_file<mapmode::readonly>;
	using mapped_file_sink = mapped_file<mapmode::readwrite>;
}
