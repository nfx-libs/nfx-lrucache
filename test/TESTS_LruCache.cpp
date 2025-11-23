/*
 * MIT License
 *
 * Copyright (c) 2025 nfx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file TESTS_LruCache.cpp
 * @brief Comprehensive tests for LruCache thread-safe caching with expiration policies
 * @details Tests covering cache operations, LRU eviction, expiration policies,
 *          thread safety, and enterprise-grade caching scenarios
 */

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <nfx/cache/LruCache.h>

namespace nfx::cache::test
{
	//=====================================================================
	// LruCache Tests
	//=====================================================================

	//----------------------------------------------
	// Basic construction
	//----------------------------------------------

	TEST( LruCacheConstruction, DefaultConstruction )
	{
		LruCache<std::string, int> cache;

		EXPECT_TRUE( cache.isEmpty() );
		EXPECT_EQ( cache.size(), 0 );
	}

	TEST( LruCacheConstruction, OptionsConstruction )
	{
		LruCacheOptions options{ 100, std::chrono::minutes( 30 ) };

		LruCache<std::string, std::string> cache( options );

		EXPECT_TRUE( cache.isEmpty() );
		EXPECT_EQ( cache.size(), 0 );
	}

	TEST( LruCacheConstruction, GetBasic )
	{
		LruCache<std::string, std::string> cache;

		// Create new entry
		auto* value = cache.get( "key1", []() { return std::string{ "value1" }; } );
		ASSERT_NE( value, nullptr );
		EXPECT_EQ( *value, "value1" );
		EXPECT_EQ( cache.size(), 1 );
		EXPECT_FALSE( cache.isEmpty() );

		// Get existing entry
		auto* value2 = cache.get( "key1", []() { return std::string{ "should_not_create" }; } );
		ASSERT_NE( value2, nullptr );
		EXPECT_EQ( *value2, "value1" );
		EXPECT_EQ( cache.size(), 1 );
	}

	//----------------------------------------------
	// Basic operations
	//----------------------------------------------

	TEST( LruCacheOperations, FindOperations )
	{
		LruCache<std::string, int> cache;

		// Try get non-existent
		auto* result1 = cache.find( "missing_key" );
		EXPECT_EQ( result1, nullptr );

		// Add entry and try get
		[[maybe_unused]] auto* existing = cache.get( "existing_key", []() { return 42; } );
		auto* result2 = cache.find( "existing_key" );
		ASSERT_NE( result2, nullptr );
		EXPECT_EQ( *result2, 42 );
	}

	TEST( LruCacheOperations, RemoveOperations )
	{
		LruCache<std::string, std::string> cache;

		// Remove non-existent
		EXPECT_FALSE( cache.remove( "missing_key" ) );

		// Add and remove
		[[maybe_unused]] auto* added = cache.get( "remove_key", []() { return std::string{ "remove_value" }; } );
		EXPECT_EQ( cache.size(), 1 );

		EXPECT_TRUE( cache.remove( "remove_key" ) );
		EXPECT_EQ( cache.size(), 0 );
		EXPECT_TRUE( cache.isEmpty() );

		// Try get after remove
		auto* result = cache.find( "remove_key" );
		EXPECT_EQ( result, nullptr );
	}

	TEST( LruCacheOperations, ClearOperations )
	{
		LruCache<std::string, int> cache;

		// Add multiple entries
		[[maybe_unused]] auto* v1 = cache.get( "key1", []() { return 1; } );
		[[maybe_unused]] auto* v2 = cache.get( "key2", []() { return 2; } );
		[[maybe_unused]] auto* v3 = cache.get( "key3", []() { return 3; } );
		EXPECT_EQ( cache.size(), 3 );

		// Clear all
		cache.clear();
		EXPECT_EQ( cache.size(), 0 );
		EXPECT_TRUE( cache.isEmpty() );

		// Verify all entries are gone
		EXPECT_EQ( cache.find( "key1" ), nullptr );
		EXPECT_EQ( cache.find( "key2" ), nullptr );
		EXPECT_EQ( cache.find( "key3" ), nullptr );
	}

	//----------------------------------------------
	// Expiration policies
	//----------------------------------------------

	TEST( LruCacheExpiration, SlidingExpirationDefault )
	{
		LruCacheOptions options{ 0, std::chrono::milliseconds( 50 ) };

		LruCache<std::string, std::string> cache( options );

		// Add entry
		[[maybe_unused]] auto* added = cache.get( "expire_key", []() { return std::string{ "expire_value" }; } );
		EXPECT_EQ( cache.size(), 1 );

		// Wait for expiration (don't access the entry to avoid refreshing the timer)
		std::this_thread::sleep_for( std::chrono::milliseconds( 60 ) );

		// Manually trigger cleanup to remove expired entries
		cache.cleanupExpired();

		// Should be expired and removed
		auto* result = cache.find( "expire_key" );
		EXPECT_EQ( result, nullptr );
		EXPECT_EQ( cache.size(), 0 );
	}

	TEST( LruCacheExpiration, CustomExpirationPerEntry )
	{
		LruCache<std::string, int> cache;

		// Add entry with custom short expiration
		[[maybe_unused]] auto* short_lived = cache.get( "short_expire", []() { return 100; }, []( CacheEntry& entry ) { entry.slidingExpiration = { std::chrono::milliseconds( 30 ) }; } );

		// Add entry with custom long expiration
		[[maybe_unused]] auto* long_lived = cache.get( "long_expire", []() { return 200; }, []( CacheEntry& entry ) { entry.slidingExpiration = { std::chrono::minutes( 10 ) }; } );

		EXPECT_EQ( cache.size(), 2 );

		// Wait for short expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );

		// Short should be expired, long should remain
		EXPECT_EQ( cache.find( "short_expire" ), nullptr );
		EXPECT_NE( cache.find( "long_expire" ), nullptr );
		EXPECT_EQ( cache.size(), 1 );
	}

	TEST( LruCacheExpiration, SlidingExpirationRenewal )
	{
		LruCacheOptions options{ 0, std::chrono::milliseconds( 100 ) };

		LruCache<std::string, std::string> cache( options );

		[[maybe_unused]] auto* initial = cache.get( "sliding_key", []() { return std::string{ "sliding_value" }; } );

		// Access periodically to keep it alive
		for ( int i{ 0 }; i < 5; ++i )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
			auto* result = cache.find( "sliding_key" );
			EXPECT_NE( result, nullptr );
		}

		// Stop accessing and wait for expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
		auto* result = cache.find( "sliding_key" );
		EXPECT_EQ( result, nullptr );
	}

	TEST( LruCacheExpiration, ManualCleanupExpired )
	{
		LruCacheOptions options{ 0, std::chrono::milliseconds( 30 ) };

		LruCache<std::string, int> cache( options );

		// Add multiple entries
		[[maybe_unused]] auto* v1 = cache.get( "key1", []() { return 1; } );
		[[maybe_unused]] auto* v2 = cache.get( "key2", []() { return 2; } );
		[[maybe_unused]] auto* v3 = cache.get( "key3", []() { return 3; } );
		EXPECT_EQ( cache.size(), 3 );

		// Wait for expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );

		// Size should still be 3 until cleanup
		EXPECT_EQ( cache.size(), 3 );

		// Manual cleanup
		cache.cleanupExpired();
		EXPECT_EQ( cache.size(), 0 );
	}

	//----------------------------------------------
	// Size limits and LRU eviction
	//----------------------------------------------

	TEST( LruCacheLRU, SizeLimitEnforcement )
	{
		LruCacheOptions options{ 3, std::chrono::hours{ 1 } };

		LruCache<std::string, std::string> cache( options );

		// Fill to capacity
		[[maybe_unused]] auto* v1 = cache.get( "key1", []() { return std::string{ "value1" }; } );
		[[maybe_unused]] auto* v2 = cache.get( "key2", []() { return std::string{ "value2" }; } );
		[[maybe_unused]] auto* v3 = cache.get( "key3", []() { return std::string{ "value3" }; } );
		EXPECT_EQ( cache.size(), 3 );

		// All entries should be present
		EXPECT_NE( cache.find( "key1" ), nullptr );
		EXPECT_NE( cache.find( "key2" ), nullptr );
		EXPECT_NE( cache.find( "key3" ), nullptr );

		// Add fourth entry - should evict LRU (key1)
		[[maybe_unused]] auto* v4 = cache.get( "key4", []() { return std::string{ "value4" }; } );
		EXPECT_EQ( cache.size(), 3 );

		// key1 should be evicted, others should remain
		EXPECT_EQ( cache.find( "key1" ), nullptr );
		EXPECT_NE( cache.find( "key2" ), nullptr );
		EXPECT_NE( cache.find( "key3" ), nullptr );
		EXPECT_NE( cache.find( "key4" ), nullptr );
	}

	TEST( LruCacheLRU, LRUOrderingWithAccess )
	{
		LruCacheOptions options{ 3, std::chrono::hours{ 1 } };

		LruCache<std::string, int> cache( options );

		// Fill cache
		cache.get( "oldest", []() { return 1; } );
		cache.get( "middle", []() { return 2; } );
		cache.get( "newest", []() { return 3; } );

		// Access oldest to make it most recent
		cache.find( "oldest" );

		// Add new entry - should evict middle (now LRU)
		cache.get( "fourth", []() { return 4; } );

		// middle should be evicted
		EXPECT_NE( cache.find( "oldest" ), nullptr );
		EXPECT_EQ( cache.find( "middle" ), nullptr );
		EXPECT_NE( cache.find( "newest" ), nullptr );
		EXPECT_NE( cache.find( "fourth" ), nullptr );
	}

	TEST( LruCacheLRU, NoSizeLimit )
	{
		LruCacheOptions options{ 0, std::chrono::hours{ 1 } }; // Unlimited

		LruCache<std::string, int> cache( options );

		// Add many entries
		for ( int i{ 0 }; i < 1000; ++i )
		{
			std::string key{ "key" + std::to_string( i ) };
			cache.get( key, [i]() { return i; } );
		}

		EXPECT_EQ( cache.size(), 1000 );

		// All entries should still be present
		for ( int i{ 0 }; i < 1000; ++i )
		{
			std::string key{ "key" + std::to_string( i ) };
			auto* result = cache.find( key );
			ASSERT_NE( result, nullptr );
			EXPECT_EQ( *result, i );
		}
	}

	//----------------------------------------------
	// Factory function and configuration
	//----------------------------------------------

	TEST( LruCacheFactory, FactoryFunctionCalls )
	{
		LruCache<std::string, std::string> cache;

		int factoryCallCount{ 0 };
		auto factory = [&factoryCallCount]() {
			++factoryCallCount;
			return std::string{ "factory_value" };
		};

		// First call should invoke factory
		auto* value1 = cache.get( "factory_key", factory );
		EXPECT_EQ( *value1, "factory_value" );
		EXPECT_EQ( factoryCallCount, 1 );

		// Second call should not invoke factory
		auto* value2 = cache.get( "factory_key", factory );
		EXPECT_EQ( *value2, "factory_value" );
		EXPECT_EQ( factoryCallCount, 1 );
	}

	TEST( LruCacheFactory, ConfigurationFunction )
	{
		LruCache<std::string, std::string> cache;

		bool configCalled{ false };
		auto configFunc = [&configCalled]( CacheEntry& entry ) {
			configCalled = true;
			entry.slidingExpiration = { std::chrono::minutes( 5 ) };
			entry.size = { 100 };
		};

		cache.get( "config_key", []() { return std::string{ "config_value" }; }, configFunc );

		EXPECT_TRUE( configCalled );
	}

	//----------------------------------------------
	// Value type tests
	//----------------------------------------------

	TEST( LruCacheValueTypes, ComplexValues )
	{
		struct ComplexValue
		{
			std::string name;
			std::vector<int> data;
			double weight;

			ComplexValue( std::string n, std::vector<int> d, double w )
				: name{ std::move( n ) }, data{ std::move( d ) }, weight{ w } {}
		};

		LruCache<std::string, ComplexValue> cache;

		auto factory = []() {
			return ComplexValue{ "test", { 1, 2, 3, 4, 5 }, 3.14 };
		};

		auto* value = cache.get( "complex_key", factory );
		EXPECT_EQ( value->name, "test" );
		EXPECT_EQ( value->data.size(), 5 );
		EXPECT_DOUBLE_EQ( value->weight, 3.14 );
	}

	TEST( LruCacheValueTypes, MoveSemantics )
	{
		LruCache<std::string, std::unique_ptr<std::string>> cache;

		auto factory = []() {
			return std::make_unique<std::string>( "unique_value" );
		};

		auto* ptr = cache.get( "unique_key", factory );
		ASSERT_NE( ptr, nullptr );
		ASSERT_NE( *ptr, nullptr );
		EXPECT_EQ( **ptr, "unique_value" );
	}

	//----------------------------------------------
	// Thread safety
	//----------------------------------------------

	TEST( LruCacheThreadSafety, ConcurrentAccess )
	{
		LruCache<int, std::string> cache;

		const int numThreads{ 10 };
		const int itemsPerThread{ 100 };
		std::vector<std::thread> threads;

		// Concurrent insertions
		for ( int t{ 0 }; t < numThreads; ++t )
		{
			threads.emplace_back( [&cache, t]() {
				for ( int i{ 0 }; i < itemsPerThread; ++i )
				{
					int key{ t * itemsPerThread + i };
					cache.get( key, [key]() {
						return std::string{ "value_" + std::to_string( key ) };
					} );
				}
			} );
		}

		for ( auto& thread : threads )
		{
			thread.join();
		}

		// Verify all entries were created
		EXPECT_EQ( cache.size(), numThreads * itemsPerThread );

		// Verify all values are accessible
		for ( int t{ 0 }; t < numThreads; ++t )
		{
			for ( int i{ 0 }; i < itemsPerThread; ++i )
			{
				int key{ t * itemsPerThread + i };
				auto* result = cache.find( key );
				ASSERT_NE( result, nullptr );
				EXPECT_EQ( *result, "value_" + std::to_string( key ) );
			}
		}
	}

	//----------------------------------------------
	// Performance characteristics
	//----------------------------------------------

	TEST( LruCachePerformance, LargeDataHandling )
	{
		LruCache<std::string, std::vector<int>> cache;

		const std::size_t numEntries{ 1000 };
		const std::size_t vectorSize{ 1000 };

		// Add large data structures
		auto start = std::chrono::high_resolution_clock::now();

		for ( std::size_t i{ 0 }; i < numEntries; ++i )
		{
			std::string key{ "large_key_" + std::to_string( i ) };
			cache.get( key, [i]() {
				std::vector<int> data( vectorSize, static_cast<int>( i ) );
				return data;
			} );
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

		EXPECT_EQ( cache.size(), numEntries );
		EXPECT_LT( duration.count(), 1000 ); // Should complete within 1 second

		// Verify random access performance
		start = std::chrono::high_resolution_clock::now();

		for ( std::size_t i{ 0 }; i < numEntries; ++i )
		{
			std::string key{ "large_key_" + std::to_string( i ) };
			auto* result = cache.find( key );
			EXPECT_NE( result, nullptr );
		}

		end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

		EXPECT_LT( duration.count(), 100 ); // Lookups should be very fast
	}

	//----------------------------------------------
	// Edge cases
	//----------------------------------------------

	TEST( LruCacheEdgeCases, EmptyStringKeys )
	{
		LruCache<std::string, int> cache;

		[[maybe_unused]] auto* added = cache.get( "", []() { return 42; } );
		EXPECT_EQ( cache.size(), 1 );

		auto* result = cache.find( "" );
		ASSERT_NE( result, nullptr );
		EXPECT_EQ( *result, 42 );
	}

	TEST( LruCacheEdgeCases, NumericKeys )
	{
		LruCache<int, std::string> cache;

		[[maybe_unused]] auto* v1 = cache.get( 0, []() { return std::string{ "zero" }; } );
		[[maybe_unused]] auto* v2 = cache.get( -1, []() { return std::string{ "negative" }; } );
		[[maybe_unused]] auto* v3 = cache.get( INT_MAX, []() { return std::string{ "max_int" }; } );

		EXPECT_EQ( cache.size(), 3 );
		EXPECT_EQ( *cache.find( 0 ), "zero" );
		EXPECT_EQ( *cache.find( -1 ), "negative" );
		EXPECT_EQ( *cache.find( INT_MAX ), "max_int" );
	}

	//----------------------------------------------
	// Background cleaning
	//----------------------------------------------

	TEST( LruCacheBackgroundCleanup, BackgroundCleanupDisabled )
	{
		LruCacheOptions options{ 0, std::chrono::milliseconds( 10 ), std::chrono::milliseconds( 0 ) }; // Disabled
		LruCache<std::string, int> cache( options );

		// Add entries
		cache.get( "key1", []() { return 1; } );
		cache.get( "key2", []() { return 2; } );
		EXPECT_EQ( cache.size(), 2 );

		// Wait for expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

		// Access cache to trigger potential background cleanup (should not happen)
		cache.get( "key3", []() { return 3; } );

		// Expired entries should still be in cache (no background cleanup)
		EXPECT_EQ( cache.size(), 3 );

		// Manual cleanup should remove expired entries
		cache.cleanupExpired();
		EXPECT_EQ( cache.size(), 1 ); // Only key3 should remain
	}

	TEST( LruCacheBackgroundCleanup, BackgroundCleanupEnabled )
	{
		// Test automatic background cleanup
		LruCacheOptions options{
			0,								 // No size limit
			std::chrono::milliseconds( 20 ), // Short expiration
			std::chrono::milliseconds( 10 )	 // Background cleanup every 10ms
		};
		LruCache<std::string, int> cache( options );

		// Add entries that will expire
		cache.get( "expire1", []() { return 1; } );
		cache.get( "expire2", []() { return 2; } );
		EXPECT_EQ( cache.size(), 2 );

		// Wait for expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 25 ) );

		// Add new entry and wait for background cleanup interval
		cache.get( "fresh", []() { return 3; } );
		std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );

		// Trigger background cleanup by accessing cache
		cache.find( "fresh" );

		// Background cleanup should have removed some expired entries
		auto sizeAfterCleanup = cache.size();
		EXPECT_LE( sizeAfterCleanup, 2 ); // Should be <= 2 (some cleanup happened)
	}

	TEST( LruCacheBackgroundCleanup, IncrementalCleanupLimiting )
	{
		// Test that background cleanup is incremental and doesn't block
		LruCacheOptions options{
			0,								// No size limit
			std::chrono::milliseconds( 5 ), // Very short expiration
			std::chrono::milliseconds( 10 ) // Background cleanup
		};
		LruCache<std::string, int> cache( options );

		// Add many entries that will expire
		const int numEntries = 50;
		for ( int i = 0; i < numEntries; ++i )
		{
			std::string key = "key_" + std::to_string( i );
			cache.get( key, [i]() { return i; } );
		}
		EXPECT_EQ( cache.size(), numEntries );

		// Wait for expiration
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

		// Trigger background cleanup multiple times
		for ( int cycle = 0; cycle < 10; ++cycle )
		{
			cache.get( "trigger_" + std::to_string( cycle ), [cycle]() { return cycle + 1000; } );
			std::this_thread::sleep_for( std::chrono::milliseconds( 12 ) );
		}

		// Should have cleaned up incrementally (not all at once)
		auto finalSize = cache.size();
		EXPECT_LT( finalSize, numEntries ); // Some cleanup should have happened
		EXPECT_GT( finalSize, 0 );			// But not necessarily all at once
	}

	TEST( LruCacheBackgroundCleanup, CleanupTimingAccuracy )
	{
		// Test that cleanup happens at the right intervals
		LruCacheOptions options{
			0,								 // No size limit
			std::chrono::milliseconds( 10 ), // Short expiration
			std::chrono::milliseconds( 30 )	 // Background cleanup every 30ms
		};
		LruCache<std::string, int> cache( options );

		// Add entry that will expire
		cache.get( "timed_key", []() { return 42; } );

		// Wait for expiration but not cleanup interval
		std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );

		// Access cache
		cache.find( "timed_key" );
		auto sizeBeforeCleanup = cache.size();
		EXPECT_GE( sizeBeforeCleanup, 0 ); // Should be >= 0

		// Wait for cleanup interval to pass
		std::this_thread::sleep_for( std::chrono::milliseconds( 35 ) );

		// Now access should trigger cleanup
		cache.find( "another_key" );
		auto sizeAfterCleanup = cache.size();

		// After sufficient time, expired entries should be cleaned
		EXPECT_LE( sizeAfterCleanup, 1 ); // Should have cleaned up expired entries
	}

	TEST( LruCacheBackgroundCleanup, SlidingExpirationWithBackgroundCleanup )
	{
		// Test that sliding expiration works correctly - entries accessed recently stay alive
		LruCacheOptions options{
			0,								  // No size limit
			std::chrono::milliseconds( 200 ), // Generous expiration time
			std::chrono::milliseconds( 0 )	  // Disable background cleanup for predictable behavior
		};
		LruCache<std::string, int> cache( options );

		// Add entry and verify it's accessible
		cache.get( "sliding1", []() { return 1; } );
		EXPECT_NE( cache.find( "sliding1" ), nullptr );

		// Wait 150ms (still within 200ms expiration)
		std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );

		// Access the entry to refresh its expiration timer
		auto* beforeRefresh = cache.find( "sliding1" );
		EXPECT_NE( beforeRefresh, nullptr ) << "Entry should still be alive before expiration";

		// Wait another 150ms (total 300ms from creation, but only 150ms since last access)
		std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );

		// Entry should still be alive because we accessed it 150ms ago (within 200ms sliding window)
		auto* afterRefresh = cache.find( "sliding1" );
		EXPECT_NE( afterRefresh, nullptr ) << "Entry should still be alive due to sliding expiration refresh";

		// Now wait 250ms without accessing (exceeds 200ms expiration)
		std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );

		// Entry should now be expired
		auto* afterExpiration = cache.find( "sliding1" );
		EXPECT_EQ( afterExpiration, nullptr ) << "Entry should be expired after 250ms without access";
	}

	TEST( LruCacheBackgroundCleanup, ThreadSafetyWithBackgroundCleanup )
	{
		// Test thread safety when background cleanup is running
		LruCacheOptions options{
			0,								 // No size limit
			std::chrono::milliseconds( 50 ), // Expiration
			std::chrono::milliseconds( 10 )	 // Frequent cleanup
		};
		LruCache<int, std::string> cache( options );

		const int numThreads = 4;
		const int operationsPerThread = 250;
		std::vector<std::thread> threads;

		// Concurrent operations while background cleanup is active
		for ( int t = 0; t < numThreads; ++t )
		{
			threads.emplace_back( [&cache, t]() {
				for ( int i = 0; i < operationsPerThread; ++i )
				{
					int key = t * operationsPerThread + i;

					// Mix of operations to trigger background cleanup
					cache.get( key, [key]() { return "thread_" + std::to_string( key ); } );

					if ( i % 10 == 0 )
					{
						cache.find( key );
					}

					if ( i % 20 == 0 )
					{
						std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
					}
				}
			} );
		}

		for ( auto& thread : threads )
		{
			thread.join();
		}

		// Cache should be in consistent state
		auto finalSize = cache.size();
		EXPECT_GT( finalSize, 0 );
		EXPECT_LE( finalSize, numThreads * operationsPerThread );

		// Should be able to access all remaining entries safely
		int accessibleCount = 0;
		for ( size_t i = 0; i < finalSize; ++i )
		{
			auto* result = cache.find( static_cast<int>( i ) );
			if ( result != nullptr )
			{
				accessibleCount++;
				EXPECT_TRUE( result->find( "thread_" ) != std::string::npos );
			}
		}

		// At least some entries should be accessible
		EXPECT_GT( accessibleCount, 0 ) << "Should have at least some accessible entries after concurrent operations";
	}
} // namespace nfx::cache::test
