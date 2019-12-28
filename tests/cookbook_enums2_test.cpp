// The MIT License (MIT)
//
// Copyright (c) 2019 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "daw/json/daw_json_link.h"

#include <daw/daw_memory_mapped_file.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace daw::cookbook_enums2 {
	enum class Colours : uint8_t { red, green, blue, black };

	struct MyClass1 {
		std::vector<Colours> member0;
	};

#ifdef __cpp_nontype_template_parameter_class
	auto json_data_contract_for( MyClass1 const & ) {
		using namespace daw::json;
		return json_data_contract<json_array<"member0", Colours>>{};
	}
#else
	namespace symbols_MyClass1 {
		static inline constexpr char const member0[] = "member0";
	} // namespace symbols_MyClass1

	auto json_data_contract_for( MyClass1 const & ) {
		using namespace daw::json;
		return json_data_contract<json_array<symbols_MyClass1::member0, Colours>>{};
	}
#endif
	auto to_json_data( MyClass1 const &value ) {
		return std::forward_as_tuple( value.member0 );
	}

	bool operator==( MyClass1 const &lhs, MyClass1 const &rhs ) {
		return to_json_data( lhs ) == to_json_data( rhs );
	}
} // namespace daw::cookbook_enums2

int main( int argc, char **argv ) {
	if( argc <= 1 ) {
		puts( "Must supply path to cookbook_enums2.json file\n" );
		exit( EXIT_FAILURE );
	}
	auto data = daw::filesystem::memory_mapped_file_t<>( argv[1] );

	auto const cls = daw::json::from_json<daw::cookbook_enums2::MyClass1>(
	  std::string_view( data.data( ), data.size( ) ) );

	daw_json_assert( cls.member0[0] == daw::cookbook_enums2::Colours::red,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[1] == daw::cookbook_enums2::Colours::green,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[2] == daw::cookbook_enums2::Colours::blue,
	                 "Unexpected value" );
	daw_json_assert( cls.member0[3] == daw::cookbook_enums2::Colours::black,
	                 "Unexpected value" );
	auto const str = daw::json::to_json( cls );
	puts( str.c_str( ) );

	auto const cls2 = daw::json::from_json<daw::cookbook_enums2::MyClass1>(
	  std::string_view( str.data( ), str.size( ) ) );

	daw_json_assert( cls == cls2, "Unexpected round trip error" );
}
