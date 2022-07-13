#include "mmio/mmio.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <system_error>
#include <utility>

#if MMIO_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN

#	define NOGDICAPMASKS
#	define NOVIRTUALKEYCODES
#	define NOWINMESSAGES
#	define NOWINSTYLES
#	define NOSYSMETRICS
#	define NOMENUS
#	define NOICONS
#	define NOKEYSTATES
#	define NOSYSCOMMANDS
#	define NORASTEROPS
#	define NOSHOWWINDOW
#	define OEMRESOURCE
#	define NOATOM
#	define NOCLIPBOARD
#	define NOCOLOR
#	define NOCTLMGR
#	define NODRAWTEXT
#	define NOGDI
#	define NOKERNEL
#	define NOUSER
#	define NONLS
#	define NOMB
#	define NOMEMMGR
#	define NOMETAFILE
#	define NOMINMAX
#	define NOMSG
#	define NOOPENFILE
#	define NOSCROLL
#	define NOSERVICE
#	define NOSOUND
#	define NOTEXTMETRIC
#	define NOWH
#	define NOWINOFFSETS
#	define NOCOMM
#	define NOKANJI
#	define NOHELP
#	define NOPROFILER
#	define NODEFERWINDOWPOS
#	define NOMCX

#	include <Windows.h>
#else
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

namespace mmio
{
#if MMIO_OS_WINDOWS
	void* const native_handle_type::invalid_handle_value = INVALID_HANDLE_VALUE;
#else
	void* const native_handle_type::map_failed = MAP_FAILED;
#endif

	template <mapmode MODE>
	mapped_file<MODE>::mapped_file(
		std::filesystem::path a_path,
		std::size_t a_size)
	{
		if (!this->open(std::move(a_path), a_size)) {
			throw std::system_error{
				std::error_code{ errno, std::generic_category() },
				"failed to open file"
			};
		}
	}

	template <mapmode MODE>
	void mapped_file<MODE>::close() noexcept
	{
#if MMIO_OS_WINDOWS
		if (this->_handle.base_address != nullptr) {
			[[maybe_unused]] const auto success = ::UnmapViewOfFile(this->_handle.base_address);
			assert(success != 0);
			this->_handle.base_address = nullptr;
		}

		if (this->_handle.file_mapping_object != nullptr) {
			[[maybe_unused]] const auto success = ::CloseHandle(this->_handle.file_mapping_object);
			assert(success != 0);
			this->_handle.file_mapping_object = nullptr;
		}

		if (this->_handle.file != INVALID_HANDLE_VALUE) {
			[[maybe_unused]] const auto success = ::CloseHandle(this->_handle.file);
			assert(success != 0);
			this->_handle.file = INVALID_HANDLE_VALUE;
			this->_size = 0;
		}
#else
		if (this->_handle.addr != MAP_FAILED) {
			if constexpr (MODE == mapmode::readwrite) {
				[[maybe_unused]] const auto success = ::msync(this->_handle.addr, this->_size, MS_SYNC);
				assert(success == 0);
			}
			[[maybe_unused]] const auto success = ::munmap(this->_handle.addr, this->_size);
			assert(success == 0);
			this->_handle.addr = MAP_FAILED;
		}

		if (this->_handle.fd != -1) {
			[[maybe_unused]] const auto success = ::close(this->_handle.fd);
			assert(success == 0);
			this->_handle.fd = -1;
			this->_size = 0;
		}
#endif
	}

	template <mapmode MODE>
	auto mapped_file<MODE>::data() const noexcept
		-> value_type*
	{
#if MMIO_OS_WINDOWS
		return static_cast<value_type*>(this->_handle.base_address);
#else
		return this->_handle.addr != MAP_FAILED ?
                   static_cast<value_type*>(this->_handle.addr) :
                   nullptr;
#endif
	}

	template <mapmode MODE>
	bool mapped_file<MODE>::is_open() const noexcept
	{
#if MMIO_OS_WINDOWS
		return this->_handle.base_address != nullptr;
#else
		return this->_handle.addr != MAP_FAILED;
#endif
	}

#if MMIO_OS_WINDOWS
	template <mapmode MODE>
	bool mapped_file<MODE>::do_open(
		const wchar_t* a_path,
		std::size_t a_size) noexcept
	{
		this->_handle.file = ::CreateFileW(
			a_path,
			MODE == mapmode::readonly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
			MODE == mapmode::readonly ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			MODE == mapmode::readonly ? OPEN_EXISTING : CREATE_ALWAYS,
			MODE == mapmode::readonly ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL,
			nullptr);
		if (this->_handle.file == INVALID_HANDLE_VALUE) {
			return false;
		}

		::LARGE_INTEGER size = {};
		if (a_size == dynamic_size) {
			if (::GetFileSizeEx(this->_handle.file, &size) == 0) {
				return false;
			}
		} else {
			size.QuadPart = a_size;
		}
		this->_size = static_cast<std::size_t>(size.QuadPart);

		this->_handle.file_mapping_object = ::CreateFileMappingW(
			this->_handle.file,
			nullptr,
			MODE == mapmode::readonly ? PAGE_READONLY : PAGE_READWRITE,
			size.HighPart,
			size.LowPart,
			nullptr);
		if (this->_handle.file_mapping_object == nullptr) {
			return false;
		}

		this->_handle.base_address = ::MapViewOfFile(
			this->_handle.file_mapping_object,
			MODE == mapmode::readonly ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			0,
			0);
		if (this->_handle.base_address == nullptr) {
			return false;
		}

		return true;
	}
#else
	template <mapmode MODE>
	bool mapped_file<MODE>::do_open(
		const char* a_path,
		std::size_t a_size) noexcept
	{
		this->_handle.fd = ::open(
			a_path,
			MODE == mapmode::readonly ? O_RDONLY : O_RDWR | O_CREAT,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  // -rw-r--r--
		if (this->_handle.fd == -1) {
			return false;
		}

		struct ::stat s = {};
		if (::fstat(this->_handle.fd, &s) == -1) {
			return false;
		}
		if (a_size != dynamic_size) {
			// extend file to requested size if too small
			if (static_cast<std::size_t>(s.st_size) < a_size &&
				::ftruncate(this->_handle.fd, a_size) == -1) {
				return false;
			}
			s.st_size = static_cast<::off_t>(a_size);
		}
		this->_size = static_cast<std::size_t>(s.st_size);

		this->_handle.addr = ::mmap(
			nullptr,
			this->_size,
			MODE == mapmode::readonly ? PROT_READ : PROT_READ | PROT_WRITE,
			MAP_SHARED,
			this->_handle.fd,
			0);
		if (this->_handle.addr == MAP_FAILED) {
			return false;
		}

		return true;
	}
#endif

	template class mapped_file<mapmode::readonly>;
	template class mapped_file<mapmode::readwrite>;
}
