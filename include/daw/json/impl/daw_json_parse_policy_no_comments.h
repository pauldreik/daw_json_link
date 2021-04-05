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
#include "namespace.h"

#include <daw/daw_function_table.h>
#include <daw/daw_hide.h>
#include <daw/daw_traits.h>

#include <ciso646>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace DAW_JSON_NS {
	template<bool DocumentIsMinified>
	struct BasicNoCommentSkippingPolicy final {
		/***
		 * The document has no whitespace between members(minified)
		 */
		static constexpr bool document_is_minified = DocumentIsMinified;
		template<typename ParseState>
		DAW_ATTRIBUTE_FLATTEN static constexpr void
		trim_left_checked( ParseState &parse_state ) {
			if constexpr( not document_is_minified ) {
				// SIMD here was much slower, most JSON has very minimal whitespace
				char const *first = parse_state.first;
				char const *const last = parse_state.last;
				if constexpr( ParseState::is_zero_terminated_string ) {
					// Ensure that zero terminator isn't included in skipable value
					while( ( static_cast<unsigned char>( *first ) - 1U ) < 0x20U ) {
						++first;
					}
				} else {
					while( DAW_JSON_LIKELY( first < last ) and
					       static_cast<unsigned char>( *first ) <= 0x20U ) {
						++first;
					}
				}
				parse_state.first = first;
			}
		}

		template<typename ParseState>
		DAW_ATTRIBUTE_FLATTEN static constexpr void
		trim_left_unchecked( ParseState &parse_state ) {
			if constexpr( not document_is_minified ) {
				char const *first = parse_state.first;
				while( static_cast<unsigned char>( *first ) <= 0x20 ) {
					++first;
				}
				parse_state.first = first;
			}
		}

		template<char... keys, typename ParseState>
		DAW_ATTRIBUTE_FLATTEN static constexpr void
		move_to_next_of( ParseState &parse_state ) {
			static_assert( sizeof...( keys ) <= 16 );

			if constexpr( traits::not_same_v<typename ParseState::exec_tag_t,
			                                 constexpr_exec_tag> ) {
				parse_state.first =
				  json_details::mem_move_to_next_of<ParseState::is_unchecked_input,
				                                    keys...>(
				    ParseState::exec_tag, parse_state.first, parse_state.last );
			} else {
				char const *first = parse_state.first;
				char const *const last = parse_state.last;
				if( ParseState::is_zero_terminated_string ) {
					daw_json_assert_weak( first < last and *first != '\0',
					                      ErrorReason::UnexpectedEndOfData, parse_state );
					while( not parse_policy_details::in<keys...>( *first ) ) {
						++first;
					}
				} else {
					daw_json_assert_weak( first < last, ErrorReason::UnexpectedEndOfData,
					                      parse_state );
					while( not parse_policy_details::in<keys...>( *first ) ) {
						++first;
						daw_json_assert_weak(
						  first < last, ErrorReason::UnexpectedEndOfData, parse_state );
					}
				}
				parse_state.first = first;
			}
		}

		DAW_ATTRIBUTE_FLATTEN static constexpr bool is_literal_end( char c ) {
			return c == '\0' or c == ',' or c == ']' or c == '}';
		}

		template<char PrimLeft, char PrimRight, char SecLeft, char SecRight,
		         typename ParseState>
		DAW_ONLY_FLATTEN static constexpr ParseState
		skip_bracketed_item_checked( ParseState &parse_state ) {
			// Not checking for Left as it is required to be skipped already
			auto result = parse_state;
			std::size_t cnt = 0;
			std::uint32_t prime_bracket_count = 1;
			std::uint32_t second_bracket_count = 0;
			char const *ptr_first = parse_state.first;
			char const *const ptr_last = parse_state.last;

			if( DAW_JSON_UNLIKELY( ptr_first >= ptr_last ) ) {
				return result;
			}
			if( *ptr_first == PrimLeft ) {
				++ptr_first;
			}
			if constexpr( ParseState::is_zero_terminated_string ) {
				while( *ptr_first != '\0' ) {
					switch( *ptr_first ) {
					case '\\':
						++ptr_first;
						break;
					case '"':
						++ptr_first;
						if constexpr( traits::not_same_v<typename ParseState::exec_tag_t,
						                                 constexpr_exec_tag> ) {
							ptr_first = json_details::mem_skip_until_end_of_string<
							  ParseState::is_unchecked_input>( ParseState::exec_tag,
							                                   ptr_first, parse_state.last );
						} else {
							while( ( *ptr_first != '\0' ) & ( *ptr_first != '"' ) ) {
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
						daw_json_assert( ptr_first < ptr_last and *ptr_first != '\0' and
						                   *ptr_first == '"',
						                 ErrorReason::UnexpectedEndOfData, parse_state );
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
							                 ErrorReason::InvalidBracketing, parse_state );
							result.last = ptr_first;
							result.counter = cnt;
							parse_state.first = ptr_first;
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
			} else {
				while( DAW_JSON_LIKELY( ptr_first < ptr_last ) ) {
					switch( *ptr_first ) {
					case '\\':
						++ptr_first;
						break;
					case '"':
						++ptr_first;
						if constexpr( traits::not_same_v<typename ParseState::exec_tag_t,
						                                 constexpr_exec_tag> ) {
							ptr_first = json_details::mem_skip_until_end_of_string<
							  ParseState::is_unchecked_input>( ParseState::exec_tag,
							                                   ptr_first, parse_state.last );
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
						                 ErrorReason::UnexpectedEndOfData, parse_state );
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
							                 ErrorReason::InvalidBracketing, parse_state );
							result.last = ptr_first;
							result.counter = cnt;
							parse_state.first = ptr_first;
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
			}
			daw_json_assert( ( prime_bracket_count == 0 ) &
			                   ( second_bracket_count == 0 ),
			                 ErrorReason::InvalidBracketing, parse_state );
			// We include the close primary bracket in the range so that subsequent
			// parsers have a terminator inside their range
			result.last = ptr_first;
			result.counter = cnt;
			parse_state.first = ptr_first;
			return result;
		}

		template<char PrimLeft, char PrimRight, char SecLeft, char SecRight,
		         typename ParseState>
		DAW_ONLY_FLATTEN static constexpr ParseState
		skip_bracketed_item_unchecked( ParseState &parse_state ) {
			// Not checking for Left as it is required to be skipped already
			auto result = parse_state;
			std::size_t cnt = 0;
			std::uint32_t prime_bracket_count = 1;
			std::uint32_t second_bracket_count = 0;
			char const *ptr_first = parse_state.first;

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
					if constexpr( traits::not_same_v<typename ParseState::exec_tag_t,
					                                 constexpr_exec_tag> ) {
						ptr_first = json_details::mem_skip_until_end_of_string<
						  ParseState::is_unchecked_input>( ParseState::exec_tag, ptr_first,
						                                   parse_state.last );
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
						parse_state.first = ptr_first;
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
			DAW_UNREACHABLE( );
		}
	};

	using NoCommentSkippingPolicy = BasicNoCommentSkippingPolicy<false>;
} // namespace DAW_JSON_NS
