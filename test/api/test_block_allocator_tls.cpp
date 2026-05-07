#include "catch.hpp"
#include "test_helpers.hpp"
#include "duckdb/storage/block_allocator.hpp"
#include "duckdb/common/allocator.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

using namespace duckdb;

// This test demonstrates a use-after-free bug in BlockAllocatorThreadLocalState.
//
// The thread_local BlockAllocatorThreadLocalState holds a non-owning optional_ptr
// to a BlockAllocator. When a thread caches block IDs (via Allocate/Free) and
// the BlockAllocator is subsequently destroyed while that thread is still alive,
// the thread's TLS destructor calls Clear(), which dereferences the now-dangling
// block_allocator pointer to push cached block IDs back to the (destroyed) global
// queues.
//
// Under AddressSanitizer, this produces a heap-use-after-free error.

TEST_CASE("BlockAllocator TLS use-after-free on thread exit", "[api][block_allocator]") {
	constexpr idx_t BLOCK_SIZE = 4096;
	constexpr idx_t VIRTUAL_MEM_SIZE = 256 * 1024 * 1024;
	constexpr idx_t PHYSICAL_MEM_SIZE = 256 * 1024 * 1024;

	std::thread worker;
	std::mutex mtx;
	std::condition_variable cv;
	bool alloc_done = false;
	bool allocator_destroyed = false;

	{
		Allocator alloc;
		BlockAllocator ba(alloc, BLOCK_SIZE, VIRTUAL_MEM_SIZE, PHYSICAL_MEM_SIZE);

		worker = std::thread([&]() {
			constexpr int NUM_BLOCKS = 16;
			data_ptr_t blocks[NUM_BLOCKS];
			for (int i = 0; i < NUM_BLOCKS; i++) {
				blocks[i] = ba.AllocateData(BLOCK_SIZE);
				REQUIRE(blocks[i] != nullptr);
			}
			for (int i = 0; i < NUM_BLOCKS; i++) {
				ba.FreeData(blocks[i], BLOCK_SIZE);
			}

			{
				std::lock_guard<std::mutex> lk(mtx);
				alloc_done = true;
			}
			cv.notify_one();

			{
				std::unique_lock<std::mutex> lk(mtx);
				cv.wait(lk, [&] { return allocator_destroyed; });
			}
		});

		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [&] { return alloc_done; });
		}
	}
	// BlockAllocator destructs here, which clears thread-local allocation state in thread-A, instead of thread-B.

	{
		std::lock_guard<std::mutex> lk(mtx);
		allocator_destroyed = true;
	}
	cv.notify_one();

	worker.join();
}

TEST_CASE("BlockAllocator TLS use-after-free via DuckDB API", "[api][block_allocator]") {
	std::thread worker;
	std::mutex mtx;
	std::condition_variable cv;
	bool queries_done = false;
	bool db_destroyed = false;

	{
		DBConfig config;
		config.options.block_allocator_size = 256 * 1024 * 1024;
		DuckDB db(nullptr, &config);

		worker = std::thread([&]() {
			{
				Connection con(db);
				con.Query("CREATE TABLE t AS SELECT range a FROM range(1000000)");
				auto result = con.Query("SELECT COUNT(*) FROM t");
				REQUIRE(CHECK_COLUMN(result, 0, {1000000}));
			}

			{
				std::lock_guard<std::mutex> lk(mtx);
				queries_done = true;
			}
			cv.notify_one();

			{
				std::unique_lock<std::mutex> lk(mtx);
				cv.wait(lk, [&] { return db_destroyed; });
			}
		});

		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [&] { return queries_done; });
		}
	}

	{
		std::lock_guard<std::mutex> lk(mtx);
		db_destroyed = true;
	}
	cv.notify_one();

	worker.join();
}
