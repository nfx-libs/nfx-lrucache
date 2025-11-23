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
 * @file Sample_LruCache.cpp
 * @brief Demonstrates LRU cache usage patterns
 * @details This sample shows how to use LruCache with factory functions,
 *          expiration policies, size limits, and cache operations
 */

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <nfx/cache/LruCache.h>

int main()
{
	using namespace nfx::cache;

	std::cout << "=== nfx-lrucache LRU Cache ===\n\n";

	//=====================================================================
	// 1. Basic cache operations
	//=====================================================================
	{
		std::cout << "1. Basic cache operations\n";
		std::cout << "-------------------------\n";

		LruCache<std::string, std::string> cache;

		// Create entry with factory function
		auto* value1 = cache.get( "key1", []() { return std::string{ "value1" }; } );
		std::cout << "Created: key1 = " << *value1 << "\n";

		// Find existing entry
		auto* found = cache.find( "key1" );
		std::cout << "Found: key1 = " << ( found ? *found : "not found" ) << "\n";

		// Find non-existent entry
		auto* notFound = cache.find( "key2" );
		std::cout << "Found: key2 = " << ( notFound ? *notFound : "not found" ) << "\n";

		std::cout << "Cache size: " << cache.size() << "\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 2. Factory pattern
	//=====================================================================
	{
		std::cout << "2. Factory pattern\n";
		std::cout << "------------------\n";

		LruCache<int, std::string> cache;

		// Factory is only called on cache miss
		auto* value1 = cache.get( 1, []() {
			std::cout << "Factory: Creating value for key 1\n";
			return std::string{ "expensive_value_1" };
		} );

		// Factory NOT called on cache hit
		[[maybe_unused]] auto* value2 = cache.get( 1, []() {
			std::cout << "  Factory: Creating value for key 1\n";
			return std::string{ "expensive_value_1" };
		} );

		std::cout << "Value: " << *value1 << "\n";
		std::cout << "Note: Factory only called once (cache hit on second access)\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 3. Size limits and LRU eviction
	//=====================================================================
	{
		std::cout << "3. Size limits and LRU eviction\n";
		std::cout << "--------------------------------\n";

		LruCacheOptions options{ 3, std::chrono::hours{ 1 } };
		LruCache<int, std::string> cache{ options };

		// Fill cache to capacity
		cache.get( 1, []() { return std::string{ "value1" }; } );
		cache.get( 2, []() { return std::string{ "value2" }; } );
		cache.get( 3, []() { return std::string{ "value3" }; } );
		std::cout << "Cache size: " << cache.size() << " (at capacity)\n";

		// Adding 4th entry evicts least recently used (key 1)
		cache.get( 4, []() { return std::string{ "value4" }; } );
		std::cout << "Added key 4, size: " << cache.size() << "\n";

		// Key 1 was evicted
		auto* evicted = cache.find( 1 );
		std::cout << "Key 1 found: " << ( evicted ? "YES" : "NO (evicted)" ) << "\n";

		// Key 4 exists
		auto* recent = cache.find( 4 );
		std::cout << "Key 4 found: " << ( recent ? "YES" : "NO" ) << "\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 4. Expiration policies
	//=====================================================================
	{
		std::cout << "4. Expiration policies\n";
		std::cout << "----------------------\n";

		LruCacheOptions options{ 0, std::chrono::milliseconds{ 100 } };
		LruCache<std::string, std::string> cache{ options };

		// Create entry with default expiration
		cache.get( "key1", []() { return std::string{ "value1" }; } );

		// Create entry with custom expiration
		cache.get( "key2", []() { return std::string{ "value2" }; }, []( CacheEntry& entry ) { entry.slidingExpiration = std::chrono::seconds{ 10 }; } );

		std::cout << "Created 2 entries with different expiration times\n";
		std::cout << "Key1: 100ms, Key2: 10s\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 5. Cache manipulation
	//=====================================================================
	{
		std::cout << "5. Cache manipulation\n";
		std::cout << "---------------------\n";

		LruCache<std::string, std::string> cache;

		// Add entries
		cache.get( "key1", []() { return std::string{ "value1" }; } );
		cache.get( "key2", []() { return std::string{ "value2" }; } );
		cache.get( "key3", []() { return std::string{ "value3" }; } );
		std::cout << "Initial size: " << cache.size() << "\n";
		std::cout << "Is empty: " << ( cache.isEmpty() ? "YES" : "NO" ) << "\n";

		// Remove specific entry
		bool removed = cache.remove( "key2" );
		std::cout << "Removed key2: " << ( removed ? "SUCCESS" : "FAILED" ) << "\n";
		std::cout << "Size after remove: " << cache.size() << "\n";

		// Clear all
		cache.clear();
		std::cout << "Size after clear: " << cache.size() << "\n";
		std::cout << "Is empty: " << ( cache.isEmpty() ? "YES" : "NO" ) << "\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 6. Practical use case - Database query cache
	//=====================================================================
	{
		std::cout << "6. Practical use case - Database query cache\n";
		std::cout << "---------------------------------------------\n";

		LruCacheOptions options{ 100, std::chrono::minutes{ 5 } };
		LruCache<std::string, std::vector<std::string>> queryCache{ options };

		auto executeQuery = [&]( const std::string& sql ) -> std::vector<std::string>* {
			return queryCache.get( sql, [&sql]() {
				std::cout << "  Executing: " << sql << "\n";
				// Simulate expensive database query
				std::vector<std::string> results;
				for ( int i = 0; i < 10; ++i )
				{
					results.push_back( "result_" + std::to_string( i ) );
				}
				return results;
			} );
		};

		std::cout << "First query (cache miss):\n";
		auto* results1 = executeQuery( "SELECT * FROM users" );
		std::cout << "  Returned " << results1->size() << " rows\n";

		std::cout << "Second query (cache hit):\n";
		auto* results2 = executeQuery( "SELECT * FROM users" );
		std::cout << "  Returned " << results2->size() << " rows\n";

		std::cout << "Cache size: " << queryCache.size() << " queries cached\n";
		std::cout << "\n";
	}

	//=====================================================================
	// 7. Heavy object caching
	//=====================================================================
	{
		std::cout << "7. Heavy object caching\n";
		std::cout << "-----------------------\n";

		struct Image
		{
			std::vector<uint8_t> data;
			int width;
			int height;

			Image( int w, int h ) : width{ w }, height{ h }
			{
				data.resize( w * h * 4 ); // RGBA
				// Simulate expensive image processing (blur filter)
				for ( size_t i = 0; i < data.size(); i += 4 )
				{
					data[i] = static_cast<uint8_t>( ( i * 73 ) % 256 );		 // R
					data[i + 1] = static_cast<uint8_t>( ( i * 151 ) % 256 ); // G
					data[i + 2] = static_cast<uint8_t>( ( i * 223 ) % 256 ); // B
					data[i + 3] = 255;										 // A
				}
			}
		};

		LruCacheOptions options{ 5, std::chrono::minutes{ 10 } };
		LruCache<std::string, Image> imageCache{ options };

		auto loadImage = [&]( const std::string& filename ) -> Image* {
			auto start = std::chrono::high_resolution_clock::now();
			auto* result = imageCache.get( filename, [&filename]() {
				std::cout << "  Loading: " << filename << " (" << ( 1920 * 1080 * 4 / 1024 ) << " KB)\n";
				return Image{ 1920, 1080 }; // Full HD image
			} );
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );
			std::cout << "  Time: " << duration.count() << " ms\n";
			return result;
		};

		std::cout << "Loading images:\n";
		auto* img1 = loadImage( "photo1.jpg" );
		std::cout << "  Dimensions: " << img1->width << "x" << img1->height << "\n";

		auto* img2 = loadImage( "photo2.jpg" );
		std::cout << "  Dimensions: " << img2->width << "x" << img2->height << "\n";

		std::cout << "Reloading photo1.jpg (cache hit):\n";
		auto* img1_cached = loadImage( "photo1.jpg" );
		std::cout << "  Dimensions: " << img1_cached->width << "x" << img1_cached->height << "\n";

		std::cout << "Cache size: " << imageCache.size() << " images cached\n";
		std::cout << "Note: Cache hit is significantly faster (no loading delay)\n";
		std::cout << "\n";
	}

	return 0;
}
