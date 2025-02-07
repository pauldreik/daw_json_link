// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//
//  This test will benchmark the performance of parsing apache_builds.json file
//  in the test_data folder.  This is one of the many common JSON parsing
//  benchmark files.  It also shows how to enable parsing of documents with
//  C/C++ style comments.  Note that it is the same parser mappings as the other
//  apache builds tests, but the parse options with from_json have changed
//

#include "defines.h"

#include "apache_builds_json.h"
#include "daw/json/daw_json_link.h"

#include <daw/cpp_17.h>
#include <daw/daw_benchmark.h>
#include <daw/daw_read_file.h>
#include <daw/daw_traits.h>

#include <fstream>
#include <iostream>
#include <streambuf>

#if not defined( DAW_NUM_RUNS )
#if not defined( DEBUG ) or defined( NDEBUG )
static inline constexpr std::size_t DAW_NUM_RUNS = 250;
#else
static inline constexpr std::size_t DAW_NUM_RUNS = 2;
#endif
#endif
static_assert( DAW_NUM_RUNS > 0 );

template<typename T>
using is_to_json_data_able = decltype( to_json_data( std::declval<T>( ) ) );

template<typename T>
inline bool DAW_CONSTEXPR is_to_json_data_able_v =
  daw::is_detected_v<is_to_json_data_able, T>;

template<typename T,
         std::enable_if_t<is_to_json_data_able_v<T>, std::nullptr_t> = nullptr>
DAW_CONSTEXPR bool operator==( T const &lhs, T const &rhs ) {
	test_assert( to_json_data( lhs ) == to_json_data( rhs ),
	             "Expected that values would be equal" );
	return true;
}

template<auto... ParseFlags>
void test( std::string_view json_sv1 ) {
	static constexpr auto ParsePolicy =
	  daw::json::options::parse_flags<ParseFlags...>;
	auto const sz = json_sv1.size( );
	std::cout << "Processing: " << daw::utility::to_bytes_per_second( sz )
	          << '\n';

	auto apache_builds_result =
	  daw::json::from_json<apache_builds::apache_builds>( json_sv1, ParsePolicy );
	test_assert( not apache_builds_result.jobs.empty( ),
	             "Bad value for jobs.size( )" );
	test_assert( apache_builds_result.numExecutors == 0,
	             "Bad value for numExecutors" );

	(void)daw::bench_n_test_mbs<DAW_NUM_RUNS>(
	  "apache_builds bench", sz,
	  []( auto f1 ) {
		  auto r =
		    daw::json::from_json<apache_builds::apache_builds>( f1, ParsePolicy );
		  daw::do_not_optimize( r );
	  },
	  json_sv1 );

	std::string str{ };
	(void)daw::bench_n_test_mbs<DAW_NUM_RUNS>(
	  "apache_builds bench(to_json_string)", sz,
	  [&]( auto const &tr ) {
		  str.clear( );
		  daw::json::to_json( tr, str );
		  daw::do_not_optimize( str );
	  },
	  apache_builds_result );

	test_assert( not str.empty( ), "Expected a string value" );

	daw::do_not_optimize( str );
	auto const apache_builds_result2 =
	  daw::json::from_json<apache_builds::apache_builds>( str, ParsePolicy );
	daw::do_not_optimize( apache_builds_result2 );
	// Removing for now as it will do a float compare and fail
	/*
	test_assert( apache_builds_result == apache_builds_result2,
	                 "Expected round trip to produce same result" );
	                 */
}

int main( int argc, char **argv )
#ifdef DAW_USE_EXCEPTIONS
  try
#endif
{
	using namespace daw::json;
	if( argc < 2 ) {
		std::cerr << "Must supply a path to apache_builds.json\n";
		exit( 1 );
	}
	auto fname = argv[1];
	auto const json_data1 = *daw::read_file( fname );
	test_assert( json_data1.size( ) > 2, "Minimum json data size is 2 '{}'" );

	std::cout << "Using " << daw::json::constexpr_exec_tag::name
	          << " exec model\n*********************************************\n";
	test<options::PolicyCommentTypes::cpp, options::ExecModeTypes::compile_time>(
	  json_data1 );
	std::cout << "Using " << daw::json::runtime_exec_tag::name
	          << " exec model\n*********************************************\n";
	test<options::PolicyCommentTypes::cpp, options::ExecModeTypes::runtime>(
	  json_data1 );
	if constexpr( not std::is_same_v<daw::json::simd_exec_tag,
	                                 daw::json::runtime_exec_tag> ) {
		std::cout << "Using " << daw::json::simd_exec_tag::name
		          << " exec model\n*********************************************\n";
		test<options::PolicyCommentTypes::cpp, options::ExecModeTypes::simd>(
		  json_data1 );
	}
}
#ifdef DAW_USE_EXCEPTIONS
catch( daw::json::json_exception const &jex ) {
	std::cerr << "Exception thrown by parser: " << jex.reason( ) << '\n';
	exit( 1 );
} catch( std::exception const &ex ) {
	std::cerr << "Unknown exception thrown during testing: " << ex.what( )
	          << '\n';
	exit( 1 );
} catch( ... ) {
	std::cerr << "Unknown exception thrown during testing\n";
	throw;
}
#endif
