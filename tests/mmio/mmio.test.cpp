#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <type_traits>

#include <catch2/catch.hpp>

#ifdef _WIN32
#	include <Windows.h>  // ensure windows.h compatibility
#endif

#include "mmio/mmio.hpp"

using namespace std::literals;

namespace
{
	template <mmio::mapmode M>
	void assert_closed(const mmio::mapped_file<M>& a_file)
	{
		REQUIRE(a_file.data() == nullptr);
		REQUIRE(a_file.empty());
		REQUIRE(!a_file.is_open());
		REQUIRE(a_file.size() == 0);
	}

	template <mmio::mapmode M>
	void assert_open(const mmio::mapped_file<M>& a_file, std::size_t a_size)
	{
		REQUIRE(a_file.data() != nullptr);
		REQUIRE(!a_file.empty());
		REQUIRE(a_file.size() == a_size);
	}

	template <mmio::mapmode M>
	void assert_movable(mmio::mapped_file<M>& a_original, std::size_t a_size)
	{
		mmio::mapped_file<M> copy{ std::move(a_original) };
		assert_closed(a_original);
		assert_open(copy, a_size);
		a_original = std::move(copy);
		assert_open(a_original, a_size);
		assert_closed(copy);
	}

	template <bool INPUT>
	[[nodiscard]] auto open_fstream(std::filesystem::path a_path)
	{
		using stream_t = std::conditional_t<INPUT, std::ifstream, std::ofstream>;

		if constexpr (!INPUT) {
			std::filesystem::create_directories(a_path.parent_path());
		}

		const auto openMode = INPUT ? std::ios_base::in : std::ios_base::out;
		stream_t result{ a_path, openMode | std::ios_base::binary };
		result.exceptions(std::ios_base::badbit);
		return result;
	}
}

TEST_CASE("reading")
{
	const std::filesystem::path root{ "reading"sv };
	const auto filePath = root / "example.txt"sv;

	const char payload[] = "the quick brown fox jumps over the lazy dog\n";
	const auto size = sizeof(payload) - 1;

	open_fstream<false>(filePath) << payload;

	mmio::mapped_file_source f;
	assert_closed(f);

	REQUIRE(f.open(filePath));
	assert_open(f, size);
	REQUIRE(std::memcmp(f.data(), payload, f.size()) == 0);

	assert_movable(f, size);

	f.close();
	assert_closed(f);

	std::size_t len = 19;
	f.open(filePath, len);
	assert_open(f, len);
	REQUIRE(std::memcmp(f.data(), payload, len) == 0);
}

TEST_CASE("writing")
{
	const std::filesystem::path root{ "writing"sv };
	const auto filePath = root / "example.txt"sv;

	const char payload[] = "she sells seashells by the seashore\n";
	const auto size = sizeof(payload) - 1;

	std::filesystem::remove(filePath);
	REQUIRE(!std::filesystem::exists(filePath));

	mmio::mapped_file_sink f;
	assert_closed(f);

	std::filesystem::create_directories(filePath.parent_path());
	REQUIRE(f.open(filePath, size));
	assert_open(f, size);
	std::memcpy(f.data(), payload, size);

	assert_movable(f, size);

	f.close();
	assert_closed(f);

	REQUIRE(std::filesystem::file_size(filePath) == size);
	std::string read;
	read.resize(size);
	open_fstream<true>(filePath).read(read.data(), size);
	REQUIRE(read == payload);
}
