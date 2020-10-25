// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_json_assert.h"
#include "daw_json_parse_common.h"
#include "daw_json_parse_policy_policy_details.h"
#include "daw_not_const_ex_functions.h"

#include <daw/daw_function_table.h>
#include <daw/daw_hide.h>

#include <ciso646>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace daw::json {
	template<bool DocumentIsMinified>
	struct BasicNoCommentSkippingPolicy final {
		/***
		 * The document has no whitespace between members(minified)
		 */
		static constexpr bool document_is_minified = DocumentIsMinified;
		template<typename Range>
		DAW_ATTRIBUTE_FLATTEN static constexpr void
		trim_left_checked( Range &rng ) {
			if constexpr( not document_is_minified ) {
				// SIMD here was much slower, most JSON has very minimal whitespace
				char const *first = rng.first;
				char const *const last = rng.last;
				while( DAW_JSON_LIKELY( first < last ) and
				       static_cast<unsigned char>( *first ) <= 0x20U ) {
					++first;
				}
				rng.first = first;
			}
		}

		template<typename Range>
		DAW_ATTRIBUTE_FLATTEN static constexpr void
		trim_left_unchecked( Range &rng ) {
			if constexpr( not document_is_minified ) {
				char const *first = rng.first;
				while( static_cast<unsigned char>( *first ) <= 0x20 ) {
					++first;
				}
				rng.first = first;
			}
		}

		template<char... keys, typename Range>
		DAW_ATTRIBUTE_FLATTEN static constexpr void move_to_next_of( Range &rng ) {
			static_assert( sizeof...( keys ) <= 16 );

			if constexpr( not std::is_same_v<typename Range::exec_tag_t,
			                                 constexpr_exec_tag> ) {
				rng.first =
				  json_details::mem_move_to_next_of<Range::is_unchecked_input, keys...>(
				    Range::exec_tag, rng.first, rng.last );
			} else {
				char const *first = rng.first;
				char const *const last = rng.last;
				daw_json_assert_weak( first < last, ErrorReason::UnexpectedEndOfData,
				                      rng );
				while( not parse_policy_details::in<keys...>( *first ) ) {
					++first;
					daw_json_assert_weak( first < last, ErrorReason::UnexpectedEndOfData,
					                      rng );
				}
				rng.first = first;
			}
		}

		DAW_ATTRIBUTE_FLATTEN static constexpr bool is_literal_end( char c ) {
			return c == '\0' or c == ',' or c == ']' or c == '}';
		}

		template<char PrimLeft, char PrimRight, char SecLeft, char SecRight,
		         typename Range>
		DAW_ATTRIBUTE_FLATTEN static constexpr Range
		skip_bracketed_item_checked( Range &rng ) {
			// Not checking for Left as it is required to be skipped already
			auto result = rng;
			std::size_t cnt = 0;
			std::uint32_t prime_bracket_count = 1;
			std::uint32_t second_bracket_count = 0;
			char const *ptr_first = rng.first;
			char const *const ptr_last = rng.last;

			if( DAW_JSON_UNLIKELY( ptr_first >= ptr_last ) ) {
				return result;
			}
			if( *ptr_first == PrimLeft ) {
				++ptr_first;
			}
			while( DAW_JSON_LIKELY( ptr_first < ptr_last ) ) {
				switch( *ptr_first ) {
				case '\\':
					++ptr_first;
					break;
				case '"':
					++ptr_first;
					if constexpr( not std::is_same_v<typename Range::exec_tag_t,
					                                 constexpr_exec_tag> ) {
						ptr_first = json_details::mem_skip_until_end_of_string<
						  Range::is_unchecked_input>( Range::exec_tag, ptr_first,
						                              rng.last );
					} else {
						while( DAW_JSON_LIKELY( ptr_first < ptr_last ) and
						       *ptr_first != '"' ) {
							if( *ptr_first == '\\' ) {
								if( ptr_first + 1 < ptr_last ) {
									ptr_first += 2;
									continue;
								} else {
									ptr_first = ptr_last;
									break;
								}
							}
							++ptr_first;
						}
					}
					daw_json_assert( ptr_first < ptr_last and *ptr_first == '"',
					                 ErrorReason::UnexpectedEndOfData, rng );
					break;
				case ',':
					if( DAW_JSON_UNLIKELY( ( prime_bracket_count == 1 ) &
					                       ( second_bracket_count == 0 ) ) ) {
						++cnt;
					}
					break;
				case PrimLeft:
					++prime_bracket_count;
					break;
				case PrimRight:
					--prime_bracket_count;
					if( prime_bracket_count == 0 ) {
						++ptr_first;
						daw_json_assert( second_bracket_count == 0,
						                 ErrorReason::InvalidBracketing, rng );
						result.last = ptr_first;
						result.counter = cnt;
						rng.first = ptr_first;
						return result;
					}
					break;
				case SecLeft:
					++second_bracket_count;
					break;
				case SecRight:
					--second_bracket_count;
					break;
				}
				++ptr_first;
			}
			daw_json_assert( ( prime_bracket_count == 0 ) &
			                   ( second_bracket_count == 0 ),
			                 ErrorReason::InvalidBracketing, rng );
			// We include the close primary bracket in the range so that subsequent
			// parsers have a terminator inside their range
			result.last = ptr_first;
			result.counter = cnt;
			rng.first = ptr_first;
			return result;
		}

		template<char PrimLeft, char PrimRight, char SecLeft, char SecRight,
		         typename Range>
		DAW_ATTRIBUTE_FLATTEN static constexpr Range
		skip_bracketed_item_unchecked( Range &rng ) {
			// Not checking for Left as it is required to be skipped already
			auto result = rng;
			std::size_t cnt = 0;
			std::uint32_t prime_bracket_count = 1;
			std::uint32_t second_bracket_count = 0;
			char const *ptr_first = rng.first;

			if( *ptr_first == PrimLeft ) {
				++ptr_first;
			}
			while( true ) {
				switch( *ptr_first ) {
				case '\\':
					++ptr_first;
					break;
				case '"':
					++ptr_first;
					if constexpr( not std::is_same_v<typename Range::exec_tag_t,
					                                 constexpr_exec_tag> ) {
						ptr_first = json_details::mem_skip_until_end_of_string<
						  Range::is_unchecked_input>( Range::exec_tag, ptr_first,
						                              rng.last );
					} else {
						while( *ptr_first != '"' ) {
							if( *ptr_first == '\\' ) {
								++ptr_first;
							}
							++ptr_first;
						}
					}
					break;
				case ',':
					if( DAW_JSON_UNLIKELY( ( prime_bracket_count == 1 ) &
					                       ( second_bracket_count == 0 ) ) ) {
						++cnt;
					}
					break;
				case PrimLeft:
					++prime_bracket_count;
					break;
				case PrimRight:
					--prime_bracket_count;
					if( prime_bracket_count == 0 ) {
						++ptr_first;
						// We include the close primary bracket in the range so that
						// subsequent parsers have a terminator inside their range
						result.last = ptr_first;
						result.counter = cnt;
						rng.first = ptr_first;
						return result;
					}
					break;
				case SecLeft:
					++second_bracket_count;
					break;
				case SecRight:
					--second_bracket_count;
					break;
				}
				++ptr_first;
			}
			// Should never get here, only loop exit is when PrimaryRight is found and
			// count == 0
			DAW_JSON_UNREACHABLE( );
		}
	};

	using NoCommentSkippingPolicy = BasicNoCommentSkippingPolicy<false>;
} // namespace daw::json
