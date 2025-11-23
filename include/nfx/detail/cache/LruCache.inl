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
 * @file LruCache.inl
 * @brief Implementation of LruCache template methods
 * @details Template method implementations for thread-safe memory cache
 *          with LRU eviction and configurable expiration policies
 */

namespace nfx::cache
{
	//=====================================================================
	// LruCacheOptions
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	inline LruCacheOptions::LruCacheOptions(
		std::size_t sizeLimit,
		std::chrono::milliseconds defaultSlidingExpiration,
		std::chrono::milliseconds backgroundCleanupInterval )
		: m_sizeLimit{ sizeLimit },
		  m_slidingExpiration{ defaultSlidingExpiration },
		  m_backgroundCleanupInterval{ backgroundCleanupInterval }
	{
	}

	//----------------------------------------------
	// Accessors
	//----------------------------------------------

	inline std::size_t LruCacheOptions::sizeLimit() const
	{
		return m_sizeLimit;
	}

	inline std::chrono::milliseconds LruCacheOptions::slidingExpiration() const
	{
		return m_slidingExpiration;
	}

	inline std::chrono::milliseconds LruCacheOptions::backgroundCleanupInterval() const
	{
		return m_backgroundCleanupInterval;
	}

	//=====================================================================
	// CacheEntry
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	inline CacheEntry::CacheEntry( std::chrono::milliseconds expiration )
		: lastAccessed{ std::chrono::steady_clock::now() },
		  slidingExpiration{ expiration }
	{
	}

	//----------------------------------------------
	// Expiration checking
	//----------------------------------------------

	inline bool CacheEntry::isExpired() const noexcept
	{
		auto now{ std::chrono::steady_clock::now() };
		return ( now - lastAccessed ) > slidingExpiration;
	}

	//----------------------------------------------
	// Access management
	//----------------------------------------------

	void inline CacheEntry::touch() noexcept
	{
		lastAccessed = std::chrono::steady_clock::now();
	}

	//=====================================================================
	// LruCache
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline LruCache<TKey, TValue>::LruCache( const LruCacheOptions& options )
		: m_options{ options },
		  m_lruHead{ nullptr },
		  m_lruTail{ nullptr },
		  m_lastCleanupTime{ std::chrono::steady_clock::now() }
	{
		if ( m_options.sizeLimit() > 0 )
		{
			m_cache.reserve( m_options.sizeLimit() );
		}
	}

	//----------------------------------------------
	// Cache operations
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline TValue* LruCache<TKey, TValue>::get( const TKey& key, FactoryFunction factory, ConfigFunction configure )
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		// Check for background cleanup opportunity
		checkAndPerformBackgroundCleanup();

		auto it = m_cache.find( key );
		if ( it != m_cache.end() )
		{
			if ( !it->second.metadata.isExpired() )
			{
				it->second.metadata.touch();		   // Reset expiration
				moveToLruHead( &it->second.metadata ); // Mark as recent

				return &it->second.value;
			}
			else
			{
				removeFromLru( &it->second.metadata ); // Clean expired

				m_cache.erase( it );
			}
		}

		TValue value{ factory() };
		CacheEntry metadata{ m_options.slidingExpiration() };

		if ( configure )
		{
			configure( metadata );
		}

		if ( m_options.sizeLimit() > 0 && m_cache.size() >= m_options.sizeLimit() )
		{
			evictLeastRecentlyUsed();
		}

		auto [insert_it, inserted]{ m_cache.try_emplace( key, std::move( value ), std::move( metadata ) ) };
		insert_it->second.metadata.keyPtr = &insert_it->first;
		addToLruHead( &insert_it->second.metadata );

		return &insert_it->second.value;
	}

	//----------------------------------------------
	// Lookup operations
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline TValue* LruCache<TKey, TValue>::find( const TKey& key )
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		// Check for background cleanup opportunity
		checkAndPerformBackgroundCleanup();

		auto it{ m_cache.find( key ) };
		if ( it != m_cache.end() && !it->second.metadata.isExpired() )
		{
			it->second.metadata.touch();
			moveToLruHead( &it->second.metadata );

			return &it->second.value;
		}

		if ( it != m_cache.end() )
		{
			removeFromLru( &it->second.metadata );

			m_cache.erase( it );
		}

		return nullptr;
	}

	//----------------------------------------------
	// Modification operations
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline bool LruCache<TKey, TValue>::remove( const TKey& key )
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		auto it = m_cache.find( key );
		if ( it != m_cache.end() )
		{
			removeFromLru( &it->second.metadata );
			m_cache.erase( it );
			return true;
		}

		return false;
	}

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::clear()
	{
		std::lock_guard<std::mutex> lock{ m_mutex };
		m_cache.clear();
		m_lruHead = nullptr;
		m_lruTail = nullptr;
	}

	template <typename TKey, typename TValue>
	inline std::size_t LruCache<TKey, TValue>::size() const
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		return m_cache.size();
	}

	//----------------------------------------------
	// State inspection
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline bool LruCache<TKey, TValue>::isEmpty() const
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		return m_cache.empty();
	}

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::cleanupExpired()
	{
		std::lock_guard<std::mutex> lock{ m_mutex };

		auto it = m_cache.begin();
		while ( it != m_cache.end() )
		{
			if ( it->second.metadata.isExpired() )
			{
				removeFromLru( &it->second.metadata );
				it = m_cache.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	//----------------------------------------------
	// Internal data structures
	//----------------------------------------------

	template <typename TKey, typename TValue>
	LruCache<TKey, TValue>::CachedItem::CachedItem( TValue val, CacheEntry meta )
		: value{ std::move( val ) },
		  metadata{ std::move( meta ) }
	{
	}

	//----------------------------------------------
	// LRU list management
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::addToLruHead( CacheEntry* entry ) noexcept
	{
		entry->lruNext = m_lruHead;
		entry->lruPrev = nullptr;

		if ( m_lruHead != nullptr )
		{
			m_lruHead->lruPrev = entry;
		}
		else
		{
			m_lruTail = entry;
		}

		m_lruHead = entry;
	}

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::removeFromLru( CacheEntry* entry ) noexcept
	{
		if ( entry->lruPrev != nullptr )
		{
			entry->lruPrev->lruNext = entry->lruNext;
		}
		else
		{
			m_lruHead = entry->lruNext;
		}

		if ( entry->lruNext != nullptr )
		{
			entry->lruNext->lruPrev = entry->lruPrev;
		}
		else
		{
			m_lruTail = entry->lruPrev;
		}

		entry->lruNext = nullptr;
		entry->lruPrev = nullptr;
	}

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::moveToLruHead( CacheEntry* entry ) noexcept
	{
		if ( entry == m_lruHead )
		{
			return;
		}

		removeFromLru( entry );
		addToLruHead( entry );
	}

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::evictLeastRecentlyUsed()
	{
		if ( m_lruTail == nullptr )
		{
			return;
		}

		const TKey* keyPtr{ static_cast<const TKey*>( m_lruTail->keyPtr ) };
		if ( keyPtr != nullptr )
		{
			removeFromLru( m_lruTail );
			m_cache.erase( *keyPtr );
		}
	}

	//----------------------------------------------
	// Background cleanup implementation
	//----------------------------------------------

	template <typename TKey, typename TValue>
	inline void LruCache<TKey, TValue>::checkAndPerformBackgroundCleanup()
	{
		// Skip if background cleanup is disabled
		if ( m_options.backgroundCleanupInterval().count() <= 0 )
		{
			return;
		}

		auto now = std::chrono::steady_clock::now();
		auto timeSinceLastCleanup = now - m_lastCleanupTime;

		if ( timeSinceLastCleanup >= m_options.backgroundCleanupInterval() )
		{
			m_lastCleanupTime = now;

			// Perform incremental cleanup of expired entries
			size_t cleanedCount = 0;

			auto it = m_cache.begin();
			while ( it != m_cache.end() && cleanedCount < MAX_CLEANUP_PER_CYCLE )
			{
				if ( it->second.metadata.isExpired() )
				{
					removeFromLru( &it->second.metadata );
					it = m_cache.erase( it );
					++cleanedCount;
				}
				else
				{
					++it;
				}
			}
		}
	}
} // namespace nfx::cache
