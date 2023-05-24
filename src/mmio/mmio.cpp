#include "mmio/mmio.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <system_error>
#include <type_traits>
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

	namespace
	{
		// https://github.com/rust-lang/rust/blob/97d328012b9ed9b7d481c40e84aa1f2c65b33ec8/library/std/src/io/error.rs#L179
		enum class ErrorKind
		{
			NotFound,
			PermissionDenied,
			ConnectionRefused,
			ConnectionReset,
			HostUnreachable,
			NetworkUnreachable,
			ConnectionAborted,
			NotConnected,
			AddrInUse,
			AddrNotAvailable,
			NetworkDown,
			BrokenPipe,
			AlreadyExists,
			WouldBlock,
			NotADirectory,
			IsADirectory,
			DirectoryNotEmpty,
			ReadOnlyFilesystem,
			FilesystemLoop,
			StaleNetworkFileHandle,
			InvalidInput,
			InvalidData,
			TimedOut,
			WriteZero,
			StorageFull,
			NotSeekable,
			FilesystemQuotaExceeded,
			FileTooLarge,
			ResourceBusy,
			ExecutableFileBusy,
			Deadlock,
			CrossesDevices,
			TooManyLinks,
			InvalidFilename,
			ArgumentListTooLong,
			Interrupted,
			Unsupported,
			UnexpectedEof,
			OutOfMemory,
			Other,
			Uncategorized
		};

		using posix_error_t = std::remove_cv_t<std::remove_reference_t<decltype(errno)>>;

		[[nodiscard]] constexpr auto posix_to_errc(posix_error_t a_posix) noexcept
			-> std::errc
		{
#define CASE(a_from, a_to) \
	case a_from:           \
		return std::errc::a_to

			// https://en.cppreference.com/w/cpp/error/errc
			switch (a_posix) {
				CASE(EAFNOSUPPORT, address_family_not_supported);
				CASE(EADDRINUSE, address_in_use);
				CASE(EADDRNOTAVAIL, address_not_available);
				CASE(EISCONN, already_connected);
				CASE(E2BIG, argument_list_too_long);
				CASE(EDOM, argument_out_of_domain);
				CASE(EFAULT, bad_address);
				CASE(EBADF, bad_file_descriptor);
				CASE(EBADMSG, bad_message);
				CASE(EPIPE, broken_pipe);
				CASE(ECONNABORTED, connection_aborted);
				CASE(EALREADY, connection_already_in_progress);
				CASE(ECONNREFUSED, connection_refused);
				CASE(ECONNRESET, connection_reset);
				CASE(EXDEV, cross_device_link);
				CASE(EDESTADDRREQ, destination_address_required);
				CASE(EBUSY, device_or_resource_busy);
				CASE(ENOTEMPTY, directory_not_empty);
				CASE(ENOEXEC, executable_format_error);
				CASE(EEXIST, file_exists);
				CASE(EFBIG, file_too_large);
				CASE(ENAMETOOLONG, filename_too_long);
				CASE(ENOSYS, function_not_supported);
				CASE(EHOSTUNREACH, host_unreachable);
				CASE(EIDRM, identifier_removed);
				CASE(EILSEQ, illegal_byte_sequence);
				CASE(ENOTTY, inappropriate_io_control_operation);
				CASE(EINTR, interrupted);
				CASE(EINVAL, invalid_argument);
				CASE(ESPIPE, invalid_seek);
				CASE(EIO, io_error);
				CASE(EISDIR, is_a_directory);
				CASE(EMSGSIZE, message_size);
				CASE(ENETDOWN, network_down);
				CASE(ENETRESET, network_reset);
				CASE(ENETUNREACH, network_unreachable);
				CASE(ENOBUFS, no_buffer_space);
				CASE(ECHILD, no_child_process);
				CASE(ENOLINK, no_link);
				CASE(ENOLCK, no_lock_available);
				CASE(ENODATA, no_message_available);
				CASE(ENOMSG, no_message);
				CASE(ENOPROTOOPT, no_protocol_option);
				CASE(ENOSPC, no_space_on_device);
				CASE(ENOSR, no_stream_resources);
				CASE(ENXIO, no_such_device_or_address);
				CASE(ENODEV, no_such_device);
				CASE(ENOENT, no_such_file_or_directory);
				CASE(ESRCH, no_such_process);
				CASE(ENOTDIR, not_a_directory);
				CASE(ENOTSOCK, not_a_socket);
				CASE(ENOSTR, not_a_stream);
				CASE(ENOTCONN, not_connected);
				CASE(ENOMEM, not_enough_memory);
				CASE(ENOTSUP, not_supported);
				CASE(ECANCELED, operation_canceled);
				CASE(EINPROGRESS, operation_in_progress);
				CASE(EPERM, operation_not_permitted);
				CASE(EOPNOTSUPP, operation_not_supported);
				CASE(EWOULDBLOCK, operation_would_block);
				CASE(EOWNERDEAD, owner_dead);
				CASE(EACCES, permission_denied);
				CASE(EPROTO, protocol_error);
				CASE(EPROTONOSUPPORT, protocol_not_supported);
				CASE(EROFS, read_only_file_system);
				CASE(EDEADLK, resource_deadlock_would_occur);
				CASE(EAGAIN, resource_unavailable_try_again);
				CASE(ERANGE, result_out_of_range);
				CASE(ENOTRECOVERABLE, state_not_recoverable);
				CASE(ETIME, stream_timeout);
				CASE(ETXTBSY, text_file_busy);
				CASE(ETIMEDOUT, timed_out);
				CASE(ENFILE, too_many_files_open_in_system);
				CASE(EMFILE, too_many_files_open);
				CASE(EMLINK, too_many_links);
				CASE(ELOOP, too_many_symbolic_link_levels);
				CASE(EOVERFLOW, value_too_large);
				CASE(EPROTOTYPE, wrong_protocol_type);

			default:
#if __cpp_lib_is_constant_evaluated
				if constexpr (!std::is_constant_evaluated())
#endif
				{
					return std::errc::io_error;
				}
			}

#undef CASE
		}

		[[nodiscard]] constexpr auto error_kind_to_posix(ErrorKind a_kind) noexcept
			-> posix_error_t
		{
#define CASE(a_from, a_to)  \
	case ErrorKind::a_from: \
		return a_to

			// https://github.com/rust-lang/rust/blob/97d328012b9ed9b7d481c40e84aa1f2c65b33ec8/library/std/src/sys/unix/mod.rs#L237
			switch (a_kind) {
				CASE(ArgumentListTooLong, E2BIG);
				CASE(AddrInUse, EADDRINUSE);
				CASE(AddrNotAvailable, EADDRNOTAVAIL);
				CASE(ResourceBusy, EBUSY);
				CASE(ConnectionAborted, ECONNABORTED);
				CASE(ConnectionRefused, ECONNREFUSED);
				CASE(ConnectionReset, ECONNRESET);
				CASE(Deadlock, EDEADLK);
				//CASE(FilesystemQuotaExceeded, EDQUOT);
				CASE(AlreadyExists, EEXIST);
				CASE(FileTooLarge, EFBIG);
				CASE(HostUnreachable, EHOSTUNREACH);
				CASE(Interrupted, EINTR);
				CASE(InvalidInput, EINVAL);
				CASE(IsADirectory, EISDIR);
				CASE(FilesystemLoop, ELOOP);
				CASE(NotFound, ENOENT);
				CASE(OutOfMemory, ENOMEM);
				CASE(StorageFull, ENOSPC);
				CASE(Unsupported, ENOSYS);
				CASE(TooManyLinks, EMLINK);
				CASE(InvalidFilename, ENAMETOOLONG);
				CASE(NetworkDown, ENETDOWN);
				CASE(NetworkUnreachable, ENETUNREACH);
				CASE(NotConnected, ENOTCONN);
				CASE(NotADirectory, ENOTDIR);
				CASE(DirectoryNotEmpty, ENOTEMPTY);
				CASE(BrokenPipe, EPIPE);
				CASE(ReadOnlyFilesystem, EROFS);
				CASE(NotSeekable, ESPIPE);
				//CASE(StaleNetworkFileHandle, ESTALE);
				CASE(TimedOut, ETIMEDOUT);
				CASE(ExecutableFileBusy, ETXTBSY);
				CASE(CrossesDevices, EXDEV);

				CASE(PermissionDenied, EACCES);

				CASE(WouldBlock, EWOULDBLOCK);

			default:
#if __cpp_lib_is_constant_evaluated
				if constexpr (!std::is_constant_evaluated())
#endif
				{
					return EIO;
				}
			}

#undef CASE
		}

		[[nodiscard]] auto decode_os_error() noexcept
			-> std::errc
		{
#if MMIO_OS_WINDOWS
#	define CASE(a_from, a_to)                                           \
	case a_from:                                                         \
		do {                                                             \
			constexpr auto posix = error_kind_to_posix(ErrorKind::a_to); \
			constexpr auto errc = posix_to_errc(posix);                  \
			return errc;                                                 \
		} while (false)

			// https://github.com/rust-lang/rust/blob/97d328012b9ed9b7d481c40e84aa1f2c65b33ec8/library/std/src/sys/windows/mod.rs#L63
			switch (::GetLastError()) {
				CASE(ERROR_ACCESS_DENIED, PermissionDenied);
				CASE(ERROR_ALREADY_EXISTS, AlreadyExists);
				CASE(ERROR_FILE_EXISTS, AlreadyExists);
				CASE(ERROR_BROKEN_PIPE, BrokenPipe);
				CASE(ERROR_FILE_NOT_FOUND, NotFound);
				CASE(ERROR_PATH_NOT_FOUND, NotFound);
				CASE(ERROR_INVALID_DRIVE, NotFound);
				CASE(ERROR_BAD_NETPATH, NotFound);
				CASE(ERROR_BAD_NET_NAME, NotFound);
				CASE(ERROR_NO_DATA, BrokenPipe);
				CASE(ERROR_INVALID_NAME, InvalidFilename);
				CASE(ERROR_BAD_PATHNAME, InvalidFilename);
				CASE(ERROR_INVALID_PARAMETER, InvalidInput);
				CASE(ERROR_NOT_ENOUGH_MEMORY, OutOfMemory);
				CASE(ERROR_OUTOFMEMORY, OutOfMemory);
				CASE(ERROR_SEM_TIMEOUT, TimedOut);
				CASE(WAIT_TIMEOUT, TimedOut);
				CASE(ERROR_DRIVER_CANCEL_TIMEOUT, TimedOut);
				CASE(ERROR_OPERATION_ABORTED, TimedOut);
				CASE(ERROR_SERVICE_REQUEST_TIMEOUT, TimedOut);
				CASE(ERROR_COUNTER_TIMEOUT, TimedOut);
				CASE(ERROR_TIMEOUT, TimedOut);
				CASE(ERROR_RESOURCE_CALL_TIMED_OUT, TimedOut);
				CASE(ERROR_CTX_MODEM_RESPONSE_TIMEOUT, TimedOut);
				CASE(ERROR_CTX_CLIENT_QUERY_TIMEOUT, TimedOut);
				CASE(FRS_ERR_SYSVOL_POPULATE_TIMEOUT, TimedOut);
				CASE(ERROR_DS_TIMELIMIT_EXCEEDED, TimedOut);
				CASE(DNS_ERROR_RECORD_TIMED_OUT, TimedOut);
				CASE(ERROR_IPSEC_IKE_TIMED_OUT, TimedOut);
				CASE(ERROR_RUNLEVEL_SWITCH_TIMEOUT, TimedOut);
				CASE(ERROR_RUNLEVEL_SWITCH_AGENT_TIMEOUT, TimedOut);
				CASE(ERROR_CALL_NOT_IMPLEMENTED, Unsupported);
				CASE(ERROR_HOST_UNREACHABLE, HostUnreachable);
				CASE(ERROR_NETWORK_UNREACHABLE, NetworkUnreachable);
				CASE(ERROR_DIRECTORY, NotADirectory);
				CASE(ERROR_DIRECTORY_NOT_SUPPORTED, IsADirectory);
				CASE(ERROR_DIR_NOT_EMPTY, DirectoryNotEmpty);
				CASE(ERROR_WRITE_PROTECT, ReadOnlyFilesystem);
				CASE(ERROR_DISK_FULL, StorageFull);
				CASE(ERROR_HANDLE_DISK_FULL, StorageFull);
				CASE(ERROR_SEEK_ON_DEVICE, NotSeekable);
				//CASE(ERROR_DISK_QUOTA_EXCEEDED, FilesystemQuotaExceeded);
				CASE(ERROR_FILE_TOO_LARGE, FileTooLarge);
				CASE(ERROR_BUSY, ResourceBusy);
				CASE(ERROR_POSSIBLE_DEADLOCK, Deadlock);
				CASE(ERROR_NOT_SAME_DEVICE, CrossesDevices);
				CASE(ERROR_TOO_MANY_LINKS, TooManyLinks);
				CASE(ERROR_FILENAME_EXCED_RANGE, InvalidFilename);

				CASE(WSAEACCES, PermissionDenied);
				CASE(WSAEADDRINUSE, AddrInUse);
				CASE(WSAEADDRNOTAVAIL, AddrNotAvailable);
				CASE(WSAECONNABORTED, ConnectionAborted);
				CASE(WSAECONNREFUSED, ConnectionRefused);
				CASE(WSAECONNRESET, ConnectionReset);
				CASE(WSAEINVAL, InvalidInput);
				CASE(WSAENOTCONN, NotConnected);
				CASE(WSAEWOULDBLOCK, WouldBlock);
				CASE(WSAETIMEDOUT, TimedOut);
				CASE(WSAEHOSTUNREACH, HostUnreachable);
				CASE(WSAENETDOWN, NetworkDown);
				CASE(WSAENETUNREACH, NetworkUnreachable);

			default:
				return std::errc::io_error;
			}
#	undef CASE
#else
			return posix_to_errc(errno);
#endif
		}
	}

	template <mapmode MODE>
	mapped_file<MODE>::mapped_file(
		std::filesystem::path a_path,
		std::size_t a_size)
	{
		auto result = this->open(std::move(a_path), a_size);
		if (!result) {
			throw std::system_error{ *result };
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

	template <mapmode MODE>
	auto mapped_file<MODE>::open(
		std::filesystem::path a_path,
		std::size_t a_size) noexcept
		-> open_result
	{
		this->close();
		if (this->do_open(a_path.c_str(), a_size)) {
			return { std::error_code() };
		} else {
			this->close();
			return { std::make_error_code(decode_os_error()) };
		}
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
