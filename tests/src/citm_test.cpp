// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//
//  This test will benchmark the performance of parsing citm_catalog.json file
//  in the test_data folder.  This is one of the many common JSON parsing
//  benchmark files and was included in the nativejson benchmark
//

#include "defines.h"

#include "citm_test_json.h"

#include <daw/daw_benchmark.h>
#include <daw/daw_read_file.h>
#include <daw/json/daw_from_json.h>
#include <daw/json/daw_to_json.h>

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

using namespace daw::json::options;

template<ExecModeTypes ExecMode>
void test( std::string_view json_sv1, bool do_asserts ) {
	std::cout << "Using " << to_string( ExecMode )
	          << " exec model\n*********************************************\n";
	auto const sz = json_sv1.size( );
	{
		auto citm_result2 = daw::bench_n_test_mbs<DAW_NUM_RUNS>(
		  "citm_catalog bench(checked)", sz,
		  []( auto f1 ) {
			  return daw::json::from_json<daw::citm::citm_object_t>(
			    f1, parse_flags<ExecMode> );
		  },
		  json_sv1 );
		daw::do_not_optimize( citm_result2 );
		if( do_asserts ) {
			test_assert( citm_result2, "Missing value" );
			test_assert( not citm_result2->areaNames.empty( ), "Expected values" );
			test_assert( citm_result2->areaNames.count( 205706005 ) == 1,
			             "Expected value" );
			test_assert( citm_result2->areaNames[205706005] == "1er balcon jardin",
			             "Incorrect value" );
		}
	}
	{
		auto citm_result2 = daw::bench_n_test_mbs<DAW_NUM_RUNS>(
		  "citm_catalog bench(unchecked)", sz,
		  []( auto f1 ) {
			  return daw::json::from_json<daw::citm::citm_object_t>(
			    f1, parse_flags<CheckedParseMode::no, ExecMode> );
		  },
		  json_sv1 );
		daw::do_not_optimize( citm_result2 );
		if( do_asserts ) {
			test_assert( citm_result2, "Missing value" );
			test_assert( not citm_result2->areaNames.empty( ), "Expected values" );
			test_assert( citm_result2->areaNames.count( 205706005 ) == 1,
			             "Expected value" );
			test_assert( citm_result2->areaNames[205706005] == "1er balcon jardin",
			             "Incorrect value" );
		}
	}
}

int main( int argc, char **argv )
#ifdef DAW_USE_EXCEPTIONS
  try
#endif
{
	using namespace daw::json;
	if( argc < 2 ) {
		std::cerr << "Must supply a filenames to open\n";
		exit( 1 );
	}
	bool const do_asserts = [&] {
		if( argc > 2 ) {
			std::string_view arg2 = argv[2];
			return arg2 != "noassert";
		}
		return true;
	}( );

	auto const json_data1 = *daw::read_file( argv[1] );
	auto const json_sv1 =
	  std::string_view( json_data1.data( ), json_data1.size( ) );

	auto const sz = json_sv1.size( );
	std::cout << "Processing: " << daw::utility::to_bytes_per_second( sz )
	          << '\n';
	test<ExecModeTypes::compile_time>( json_sv1, do_asserts );
	test<ExecModeTypes::runtime>( json_sv1, do_asserts );
	if constexpr( not std::is_same_v<daw::json::simd_exec_tag,
	                                 daw::json::runtime_exec_tag> ) {
		test<ExecModeTypes::simd>( json_sv1, do_asserts );
	}

	std::cout
	  << "to_json testing\n*********************************************\n";
	auto const citm_result =
	  daw::json::from_json<daw::citm::citm_object_t>( json_sv1 );

	std::string str{ };
	str.reserve( json_sv1.size( ) );
	(void)daw::bench_n_test_mbs<DAW_NUM_RUNS>(
	  "citm bench(to_json_string)", sz,

	  [&]( daw::citm::citm_object_t const &tr ) {
		  str.clear( );
		  daw::json::to_json( tr, str );
		  daw::do_not_optimize( str );
	  },
	  citm_result );
	test_assert( not str.empty( ), "Expected a string value" );
	daw::do_not_optimize( str );
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