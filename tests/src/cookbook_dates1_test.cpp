// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//
// See cookbook/dates.md for the 1st example
//

#include "defines.h"

#include <daw/daw_read_file.h>

#include "daw/json/daw_json_link.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

namespace daw::cookbook_dates1 {
	struct MyClass1 {
		std::string name;
		std::chrono::time_point<std::chrono::system_clock,
		                        std::chrono::milliseconds>
		  timestamp;
	};

	bool operator==( MyClass1 const &lhs, MyClass1 const &rhs ) {
		return std::tie( lhs.name, lhs.timestamp ) ==
		       std::tie( rhs.name, rhs.timestamp );
	}
} // namespace daw::cookbook_dates1

namespace daw::json {
	template<>
	struct json_data_contract<daw::cookbook_dates1::MyClass1> {
#if defined( DAW_JSON_CNTTP_JSON_NAME )
		using type = json_member_list<json_string<"name">, json_date<"timestamp">>;
#else
		static constexpr char const name[] = "name";
		static constexpr char const timestamp[] = "timestamp";
		using type = json_member_list<json_string<name>, json_date<timestamp>>;
#endif
		static inline auto
		to_json_data( daw::cookbook_dates1::MyClass1 const &value ) {
			return std::forward_as_tuple( value.name, value.timestamp );
		}
	};
} // namespace daw::json

int main( int argc, char **argv )
#ifdef DAW_USE_EXCEPTIONS
  try
#endif
{
	if( argc <= 1 ) {
		puts( "Must supply path to cookbook_dates1.json file\n" );
		exit( EXIT_FAILURE );
	}
	auto data = *daw::read_file( argv[1] );

	auto const cls = daw::json::from_json<daw::cookbook_dates1::MyClass1>(
	  std::string_view( data.data( ), data.size( ) ) );

	test_assert( cls.name == "Toronto", "Unexpected value" );

	std::string const str = daw::json::to_json( cls );
	puts( str.c_str( ) );

	auto const cls2 = daw::json::from_json<daw::cookbook_dates1::MyClass1>( str );

	test_assert( cls == cls2, "Unexpected round trip error" );
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