#==============================================================================
# nfx-lrucache - Top-level CMake configuration
#==============================================================================

#----------------------------------------------
# Project definition
#----------------------------------------------

cmake_minimum_required(VERSION 3.20)

project(nfx-lrucache
	VERSION 0.1.0
	DESCRIPTION "High-performance C++20 thread-safe LRU cache with sliding expiration"
	HOMEPAGE_URL "https://github.com/nfx-libs/nfx-lrucache"
	LANGUAGES CXX
)

#----------------------------------------------
# Installation prefix configuration
#----------------------------------------------

# Set default installation path to nfx
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	if(WIN32)
		set(CMAKE_INSTALL_PREFIX "C:/Program Files/nfx" CACHE PATH "Installation directory" FORCE)
	else()
		set(CMAKE_INSTALL_PREFIX "/usr/local/nfx" CACHE PATH "Installation directory" FORCE)
	endif()
	message(STATUS "Setting install prefix to '${CMAKE_INSTALL_PREFIX}'")
endif()

#----------------------------------------------
# Version configuration
#----------------------------------------------

set(NFX_LRUCACHE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} CACHE INTERNAL "nfx-lrucache major version")
set(NFX_LRUCACHE_VERSION_MINOR ${PROJECT_VERSION_MINOR} CACHE INTERNAL "nfx-lrucache minor version")
set(NFX_LRUCACHE_VERSION_PATCH ${PROJECT_VERSION_PATCH} CACHE INTERNAL "nfx-lrucache patch version")
set(NFX_LRUCACHE_VERSION       ${PROJECT_VERSION}       CACHE INTERNAL "nfx-lrucache version")

#----------------------------------------------
# Project metadata
#----------------------------------------------

set(NFX_LRUCACHE_LICENSE "MIT")
set(NFX_LRUCACHE_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

#----------------------------------------------
# Testing setup
#----------------------------------------------

enable_testing()

#----------------------------------------------
# Build options
#----------------------------------------------

# --- Build components (header-only library) ---
option(NFX_LRUCACHE_BUILD_TESTS          "Build tests"                         OFF )
option(NFX_LRUCACHE_BUILD_SAMPLES        "Build samples"                       OFF )
option(NFX_LRUCACHE_BUILD_BENCHMARKS     "Build benchmarks"                    OFF )
option(NFX_LRUCACHE_BUILD_DOCUMENTATION  "Build Doxygen documentation"         OFF )

# --- Installation ---
option(NFX_LRUCACHE_INSTALL_PROJECT      "Install project"                     OFF )

# --- Packaging ---
option(NFX_LRUCACHE_PACKAGE_SOURCE       "Enable source package generation"    OFF )
option(NFX_LRUCACHE_PACKAGE_ARCHIVE      "Enable TGZ/ZIP package generation"   OFF )
option(NFX_LRUCACHE_PACKAGE_DEB          "Enable DEB package generation"       OFF )
option(NFX_LRUCACHE_PACKAGE_RPM          "Enable RPM package generation"       OFF )
option(NFX_LRUCACHE_PACKAGE_WIX          "Enable WiX Windows installer (MSI)"  OFF )

#----------------------------------------------
# CMake modules configuration
#----------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#----------------------------------------------
# Main configuration
#----------------------------------------------

include(nfxLruCacheBuildConfig)
include(nfxLruCacheDependencies)
include(nfxLruCacheTarget)

#----------------------------------------------
# Installation and packaging
#----------------------------------------------

include(nfxLruCacheInstall)
include(nfxLruCachePackaging)

#----------------------------------------------
# Targets
#----------------------------------------------

add_subdirectory(test)
add_subdirectory(samples)
add_subdirectory(benchmark)
add_subdirectory(doc)
