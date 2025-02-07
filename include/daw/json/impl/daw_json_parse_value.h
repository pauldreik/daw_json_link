// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "version.h"

#include "daw_json_assert.h"
#include "daw_json_parse_array_iterator.h"
#include "daw_json_parse_kv_array_iterator.h"
#include "daw_json_parse_kv_class_iterator.h"
#include "daw_json_parse_name.h"
#include "daw_json_parse_real.h"
#include "daw_json_parse_std_string.h"
#include "daw_json_parse_string_need_slow.h"
#include "daw_json_parse_string_quote.h"
#include "daw_json_parse_unsigned_int.h"
#include "daw_json_parse_value_fwd.h"
#include "daw_json_traits.h"
#include "daw_json_value_fwd.h"

#include <daw/daw_algorithm.h>
#include <daw/daw_attributes.h>
#include <daw/daw_traits.h>
#include <daw/daw_utility.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <tuple>

namespace daw::json {
	inline namespace DAW_JSON_VER {
		namespace json_details {
			/***
			 * Depending on the type of literal, it may always be quoted, sometimes,
			 * or never.  This method handles the always and sometimes cases.
			 * In checked input, ensures State has more data.
			 * @tparam literal_as_string Is the literal being parsed enclosed in
			 * quotes.
			 * @tparam ParseState ParseState idiom
			 * @param parse_state Current parsing state
			 */
			template<options::LiteralAsStringOpt literal_as_string,
			         typename ParseState>
			DAW_ATTRIB_INLINE constexpr void
			skip_quote_when_literal_as_string( ParseState &parse_state ) {
				if constexpr( literal_as_string ==
				              options::LiteralAsStringOpt::Always ) {
					daw_json_assert_weak( parse_state.is_quotes_checked( ),
					                      ErrorReason::InvalidNumberUnexpectedQuoting,
					                      parse_state );
					parse_state.remove_prefix( );
				} else if constexpr( literal_as_string ==
				                     options::LiteralAsStringOpt::Maybe ) {
					daw_json_assert_weak( parse_state.has_more( ),
					                      ErrorReason::UnexpectedEndOfData, parse_state );
					if( parse_state.front( ) == '"' ) {
						parse_state.remove_prefix( );
					}
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_real( ParseState &parse_state ) {
				using constructor_t = typename JsonMember::constructor_t;
				using element_t = typename JsonMember::base_type;

				if constexpr( JsonMember::literal_as_string !=
				              options::LiteralAsStringOpt::Never ) {
					if constexpr( not KnownBounds ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
					}
					if constexpr( JsonMember::allow_number_errors ==
					                options::JsonNumberErrors::AllowInf or
					              JsonMember::allow_number_errors ==
					                options::JsonNumberErrors::AllowNanInf ) {
						element_t sign = element_t( 1.0 );
						if( parse_state.front( ) == '-' ) {
							sign = element_t( -1.0 );
							parse_state.first++;
						}
						// Looking for Inf as that will match Infinity too.
						if( parse_state.size( ) >= 4 and
						    parse_state.starts_with( "Inf" ) ) {

							parse_state.first += 3;
							if( parse_state.front( ) == '"' ) {
								parse_state.first++;
							} else if( parse_state.size( ) >= 6 and
							           parse_state.starts_with( R"(inity")" ) ) {
								parse_state.first += 6;
							} else {
								daw_json_error( ErrorReason::InvalidString, parse_state );
							}
							if constexpr( KnownBounds ) {
								daw_json_assert_weak( parse_state.empty( ),
								                      ErrorReason::InvalidNumber, parse_state );
							} else {
								daw_json_assert_weak(
								  parse_state.empty( ) or parse_policy_details::at_end_of_item(
								                            parse_state.front( ) ),
								  ErrorReason::InvalidEndOfValue, parse_state );
							}
							return daw::cxmath::copy_sign(
							  daw::numeric_limits<element_t>::infinity( ), sign );
						} else if( sign < element_t( 0 ) ) {
							parse_state.first--;
						}
					}
					if constexpr( JsonMember::allow_number_errors ==
					                options::JsonNumberErrors::AllowNaN or
					              JsonMember::allow_number_errors ==
					                options::JsonNumberErrors::AllowNanInf ) {
						if( parse_state.starts_with( "NaN" ) ) {
							parse_state.template move_to_next_of<'"'>( );
							parse_state.remove_prefix( );
							if constexpr( KnownBounds ) {
								daw_json_assert_weak( parse_state.empty( ),
								                      ErrorReason::InvalidNumber, parse_state );
							} else {
								daw_json_assert_weak(
								  parse_state.empty( ) or parse_policy_details::at_end_of_item(
								                            parse_state.front( ) ),
								  ErrorReason::InvalidEndOfValue, parse_state );
							}
							return daw::numeric_limits<element_t>::quiet_NaN( );
						}
					}
				}
				if constexpr( KnownBounds and JsonMember::literal_as_string ==
				                                options::LiteralAsStringOpt::Never ) {
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  parse_real<element_t, true>( parse_state ) );
				} else {
					daw_json_assert_weak(
					  parse_state.has_more( ) and
					    parse_policy_details::is_number_start( parse_state.front( ) ),
					  ErrorReason::InvalidNumberStart, parse_state );

					auto result = construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  parse_real<element_t, false>( parse_state ) );

					if constexpr( KnownBounds ) {
						daw_json_assert_weak(
						  parse_state.empty( ) or
						    parse_policy_details::at_end_of_item( parse_state.front( ) ),
						  ErrorReason::InvalidEndOfValue, parse_state );
					} else {
						if constexpr( JsonMember::literal_as_string !=
						              options::LiteralAsStringOpt::Never ) {
							skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
							  parse_state );
						}
						daw_json_assert_weak(
						  parse_state.empty( ) or
						    parse_policy_details::at_end_of_item( parse_state.front( ) ),
						  ErrorReason::InvalidEndOfValue, parse_state );
					}
					return result;
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_signed( ParseState &parse_state ) {
				using constructor_t = typename JsonMember::constructor_t;
				using element_t = typename JsonMember::base_type;
				using int_type =
				  typename std::conditional_t<std::is_enum_v<element_t>,
				                              std::underlying_type<element_t>,
				                              daw::traits::identity<element_t>>::type;

				static_assert( daw::is_signed_v<int_type>, "Expected signed type" );
				if constexpr( KnownBounds ) {
					daw_json_assert_weak(
					  parse_policy_details::is_number_start( parse_state.front( ) ),
					  ErrorReason::InvalidNumberStart, parse_state );
				} else {
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
					} else if constexpr( not ParseState::is_zero_terminated_string( ) ) {
						daw_json_assert_weak( parse_state.has_more( ),
						                      ErrorReason::UnexpectedEndOfData,
						                      parse_state );
					}
				}
				auto const sign = static_cast<int_type>(
				  parse_policy_details::validate_signed_first( parse_state ) );
				using uint_type =
				  typename std::conditional_t<daw::is_system_integral_v<int_type>,
				                              daw::make_unsigned<int_type>,
				                              daw::traits::identity<int_type>>::type;
				auto parsed_val = to_signed(
				  unsigned_parser<uint_type, JsonMember::range_check, KnownBounds>(
				    ParseState::exec_tag, parse_state ),
				  sign );

				if constexpr( KnownBounds ) {
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  static_cast<element_t>( parsed_val ) );
				} else {
					auto result = construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  static_cast<element_t>( parsed_val ) );
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
					}
					parse_state.trim_left( );
					daw_json_assert_weak(
					  not parse_state.has_more( ) or
					    parse_policy_details::at_end_of_item( parse_state.front( ) ),
					  ErrorReason::InvalidEndOfValue, parse_state );
					return result;
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_unsigned( ParseState &parse_state ) {
				using constructor_t = typename JsonMember::constructor_t;
				using element_t = typename JsonMember::base_type;
				using uint_type =
				  typename std::conditional_t<std::is_enum_v<element_t>,
				                              std::underlying_type<element_t>,
				                              daw::traits::identity<element_t>>::type;

				if constexpr( KnownBounds ) {
					parse_policy_details::validate_unsigned_first( parse_state );

					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  unsigned_parser<uint_type, JsonMember::range_check, KnownBounds>(
					    ParseState::exec_tag, parse_state ) );
				} else {
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
						if constexpr( not ParseState::is_zero_terminated_string( ) ) {
							daw_json_assert_weak( parse_state.has_more( ),
							                      ErrorReason::UnexpectedEndOfData,
							                      parse_state );
						}
					} else if constexpr( not ParseState::is_zero_terminated_string( ) ) {
						daw_json_assert_weak( parse_state.has_more( ),
						                      ErrorReason::UnexpectedEndOfData,
						                      parse_state );
					}
					daw_json_assert_weak(
					  parse_policy_details::is_number( parse_state.front( ) ),
					  ErrorReason::InvalidNumber, parse_state );
					auto result = construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  unsigned_parser<uint_type, JsonMember::range_check, KnownBounds>(
					    ParseState::exec_tag, parse_state ) );
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
						if constexpr( not ParseState::is_zero_terminated_string( ) ) {
							daw_json_assert_weak( parse_state.has_more( ),
							                      ErrorReason::UnexpectedEndOfData,
							                      parse_state );
						}
					}
					daw_json_assert_weak(
					  not parse_state.has_more( ) or
					    parse_policy_details::at_end_of_item( parse_state.front( ) ),
					  ErrorReason::InvalidEndOfValue, parse_state );
					return result;
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_null( ParseState &parse_state ) {

				using constructor_t = typename JsonMember::constructor_t;
				auto const construct_empty = [&] {
					if constexpr( std::is_invocable_v<
					                constructor_t,
					                concepts::construct_nullable_with_empty_t> ) {
						return construct_value(
						  template_args<typename JsonMember::wrapped_type, constructor_t>,
						  parse_state, concepts::construct_nullable_with_empty );
					} else {
						return construct_value(
						  template_args<typename JsonMember::wrapped_type, constructor_t>,
						  parse_state );
					}
				};

				using base_member_type = typename JsonMember::member_type;
				static_assert( not std::is_same_v<base_member_type, JsonMember> );
				if constexpr( KnownBounds ) {
					// skip_value will leave a null parse_state
					if( parse_state.is_null( ) ) {
						return construct_empty( );
					}
					return construct_value(
					  template_args<base_member_type, constructor_t>, parse_state,
					  parse_value<base_member_type, true>(
					    parse_state, ParseTag<base_member_type::expected_type>{ } ) );
				} else if constexpr( ParseState::is_unchecked_input ) {
					if( not parse_state.has_more( ) or
					    parse_state.is_at_token_after_value( ) ) {
						return construct_empty( );
					}
					if( parse_state.front( ) == 'n' ) {
						parse_state.remove_prefix( 4 );
						parse_state.trim_left_unchecked( );
						parse_state.remove_prefix( );
						return construct_empty( );
					}
					return construct_value(
					  template_args<base_member_type, constructor_t>, parse_state,
					  parse_value<base_member_type>(
					    parse_state, ParseTag<base_member_type::expected_type>{ } ) );
				} else {
					if( not parse_state.has_more( ) or
					    parse_state.is_at_token_after_value( ) ) {
						return construct_empty( );
					}
					if( parse_state.starts_with( "null" ) ) {
						parse_state.remove_prefix( 4 );
						daw_json_assert_weak(
						  not parse_state.has_more( ) or
						    parse_policy_details::at_end_of_item( parse_state.front( ) ),
						  ErrorReason::InvalidLiteral, parse_state );
						parse_state.trim_left_checked( );
						return construct_empty( );
					}
					using parse_to_t = typename base_member_type::parse_to_t;
					if constexpr( not std::is_move_constructible_v<parse_to_t> and
					              not std::is_copy_constructible_v<parse_to_t> ) {
						static_assert(
						  std::is_invocable_v<
						    concepts::nullable_value_traits<json_result<JsonMember>>,
						    concepts::construct_nullable_with_pointer_t, parse_to_t *> );
						return construct_value(
						  template_args<base_member_type, constructor_t>, parse_state,
						  concepts::construct_nullable_with_pointer,
						  new parse_to_t{ parse_value<base_member_type>(
						    parse_state, ParseTag<base_member_type::expected_type>{ } ) } );
					} else {
						return construct_value(
						  template_args<base_member_type, constructor_t>, parse_state,
						  parse_value<base_member_type>(
						    parse_state, ParseTag<base_member_type::expected_type>{ } ) );
					}
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_bool( ParseState &parse_state ) {
				using constructor_t = typename JsonMember::constructor_t;

				if constexpr( KnownBounds ) {
					// We have already checked if it is a true/false
					if constexpr( ParseState::is_unchecked_input ) {
						return static_cast<bool>( parse_state.counter );
					} else {
						switch( parse_state.front( ) ) {
						case 't':
							return construct_value(
							  template_args<json_result<JsonMember>, constructor_t>,
							  parse_state, true );
						case 'f':
							return construct_value(
							  template_args<json_result<JsonMember>, constructor_t>,
							  parse_state, false );
						}
						daw_json_error( ErrorReason::InvalidLiteral, parse_state );
					}
				} else {
					// Beginning quotes
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
					}
					bool result = false;
					if constexpr( ParseState::is_unchecked_input ) {
						if( parse_state.front( ) == 't' ) /* true */ {
							result = true;
							parse_state.remove_prefix( 4 );
						} else /* false */ {
							parse_state.remove_prefix( 5 );
						}
					} else {
						if( parse_state.starts_with( "true" ) ) {
							parse_state.remove_prefix( 4 );
							result = true;
						} else if( parse_state.starts_with( "false" ) ) {
							parse_state.remove_prefix( 5 );
						} else {
							daw_json_error( ErrorReason::InvalidLiteral, parse_state );
						}
					}
					// Trailing quotes
					if constexpr( JsonMember::literal_as_string !=
					              options::LiteralAsStringOpt::Never ) {
						skip_quote_when_literal_as_string<JsonMember::literal_as_string>(
						  parse_state );
					}
					parse_state.trim_left( );
					daw_json_assert_weak(
					  not parse_state.has_more( ) or
					    parse_policy_details::at_end_of_item( parse_state.front( ) ),
					  ErrorReason::InvalidEndOfValue, parse_state );
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  result );
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_string_raw( ParseState &parse_state ) {

				using constructor_t = typename JsonMember::constructor_t;
				if constexpr( KnownBounds ) {
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  std::data( parse_state ), std::size( parse_state ) );
				} else {
					if constexpr( JsonMember::allow_escape_character ==
					              options::AllowEscapeCharacter::Allow ) {
						auto const str = skip_string( parse_state );
						return construct_value(
						  template_args<json_result<JsonMember>, constructor_t>,
						  parse_state, std::data( str ), std::size( str ) );
					} else {
						parse_state.remove_prefix( );

						char const *const first = parse_state.first;
						parse_state.template move_to_next_of<'"'>( );
						char const *const last = parse_state.first;
						parse_state.remove_prefix( );
						return construct_value(
						  template_args<json_result<JsonMember>, constructor_t>,
						  parse_state, first, static_cast<std::size_t>( last - first ) );
					}
				}
			}

			/***
			 * We know that we are constructing a std::string or
			 * std::optional<std::string> We can take advantage of this and reduce
			 * the allocator time by presizing the string up front and then using a
			 * pointer to the data( ).
			 */
			template<typename JsonMember>
			inline constexpr bool can_parse_to_stdstring_fast_v = std::disjunction_v<
			  can_single_allocation_string<json_result<JsonMember>>,
			  can_single_allocation_string<json_base_type<JsonMember>>>;

			template<typename T>
			using json_member_constructor_t = typename T::constructor_t;

			template<typename T>
			using json_member_parse_to_t = typename T::parse_to_t;

			template<typename T>
			inline constexpr bool has_json_member_constructor_v =
			  daw::is_detected_v<json_member_constructor_t, T>;

			template<typename T>
			inline constexpr bool has_json_member_parse_to_v =
			  daw::is_detected_v<json_member_constructor_t, T>;

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_string_escaped( ParseState &parse_state ) {
				static_assert( has_json_member_constructor_v<JsonMember> );
				static_assert( has_json_member_parse_to_v<JsonMember> );

				using constructor_t = typename JsonMember::constructor_t;
				if constexpr( can_parse_to_stdstring_fast_v<JsonMember> ) {
					using AllowHighEightbits =
					  std::bool_constant<JsonMember::eight_bit_mode !=
					                     options::EightBitModes::DisallowHigh>;
					auto parse_state2 =
					  KnownBounds ? parse_state : skip_string( parse_state );
					// FIXME this needs std::string, fix
					if( not AllowHighEightbits::value or
					    needs_slow_path( parse_state2 ) ) {
						// There are escapes in the string
						return parse_string_known_stdstring<AllowHighEightbits::value,
						                                    JsonMember, true>(
						  parse_state2 );
					}
					// There are no escapes in the string, we can just use the ptr/size
					// ctor
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  std::data( parse_state2 ), daw::data_end( parse_state2 ) );
				} else {
					auto parse_state2 =
					  KnownBounds ? parse_state : skip_string( parse_state );
					using AllowHighEightbits =
					  std::bool_constant<JsonMember::eight_bit_mode !=
					                     options::EightBitModes::DisallowHigh>;
					if( not AllowHighEightbits::value or
					    needs_slow_path( parse_state2 ) ) {
						// There are escapes in the string
						return parse_string_known_stdstring<AllowHighEightbits::value,
						                                    JsonMember, true>(
						  parse_state2 );
					}
					// There are no escapes in the string, we can just use the ptr/size
					// ctor
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  std::data( parse_state2 ), daw::data_end( parse_state2 ) );
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_date( ParseState &parse_state ) {

				daw_json_assert_weak( parse_state.has_more( ),
				                      ErrorReason::UnexpectedEndOfData, parse_state );
				auto str = KnownBounds ? parse_state : skip_string( parse_state );
				using constructor_t = typename JsonMember::constructor_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  std::data( str ), std::size( str ) );
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_custom( ParseState &parse_state ) {

				auto const str = [&] {
					if constexpr( JsonMember::custom_json_type ==
					              options::JsonCustomTypes::String ) {
						if constexpr( KnownBounds ) {
							return parse_state;
						} else {
							return skip_string( parse_state );
						}
					} else if constexpr( JsonMember::custom_json_type ==
					                     options::JsonCustomTypes::Literal ) {
						return KnownBounds ? parse_state : skip_literal( parse_state );
					} else {
						static_assert( JsonMember::custom_json_type ==
						               options::JsonCustomTypes::Any );
						// If we are a root object, parse_state will have the quotes and
						// KnownBounds cannot be true This tells us that there is an array
						// start '[' or a member name previous to current position
						if constexpr( KnownBounds ) {
							auto result = parse_state;
							if( *( result.first - 1 ) == '"' ) {
								result.first--;
							}
							return result;
						} else {
							if( parse_state.front( ) == '"' ) {
								auto result = skip_string( parse_state );
								result.first--;
								return result;
							}
							return skip_value( parse_state );
						}
					}
				}( );
				daw_json_assert_weak(
				  str.has_more( ) and not( str.front( ) == '[' or str.front( ) == '{' ),
				  ErrorReason::InvalidStartOfValue, str );
				using constructor_t = typename JsonMember::from_converter_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  std::string_view( std::data( str ), std::size( str ) ) );
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_class( ParseState &parse_state ) {

				using element_t = typename JsonMember::wrapped_type;
				daw_json_assert_weak( parse_state.has_more( ),
				                      ErrorReason::UnexpectedEndOfData, parse_state );

				if constexpr( KnownBounds ) {
					return json_data_contract_trait_t<element_t>::parse_to_class(
					  parse_state, template_arg<JsonMember> );
				} else if constexpr( is_pinned_type_v<element_t> ) {
					auto const run_after_parse = daw::on_exit_success( [&] {
						parse_state.trim_left_checked( );
					} );
					(void)run_after_parse;
					return json_data_contract_trait_t<element_t>::parse_to_class(
					  parse_state, template_arg<JsonMember> );
				} else {
					auto result = json_data_contract_trait_t<element_t>::parse_to_class(
					  parse_state, template_arg<JsonMember> );
					parse_state.trim_left_checked( );
					return result;
				}
			}

			/**
			 * Parse a key_value pair encoded as a json object where the keys are
			 * the member names
			 * @tparam JsonMember json_key_value type
			 * @tparam ParseState Input range type
			 * @param parse_state ParseState of input to parse
			 * @return Constructed key_value container
			 */
			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_keyvalue( ParseState &parse_state ) {

				static_assert( JsonMember::expected_type == JsonParseTypes::KeyValue,
				               "Expected a json_key_value" );
				daw_json_assert_weak( parse_state.is_opening_brace_checked( ),
				                      ErrorReason::ExpectedKeyValueToStartWithBrace,
				                      parse_state );

				parse_state.remove_prefix( );
				parse_state.trim_left( );

				using iter_t =
				  json_parse_kv_class_iterator<JsonMember, ParseState,
				                               can_be_random_iterator_v<KnownBounds>>;

				using constructor_t = typename JsonMember::constructor_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  iter_t( parse_state ), iter_t( ) );
			}

			/**
			 * Parse a key_value pair encoded as a json object where the keys are
			 * the member names
			 * @tparam JsonMember json_key_value type
			 * @tparam ParseState Input ParseState type
			 * @param parse_state ParseState of input to parse
			 * @return Constructed key_value container
			 */
			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_keyvalue_array( ParseState &parse_state ) {

				static_assert( JsonMember::expected_type ==
				                 JsonParseTypes::KeyValueArray,
				               "Expected a json_key_value" );
				daw_json_assert_weak(
				  parse_state.is_opening_bracket_checked( ),
				  ErrorReason::ExpectedKeyValueArrayToStartWithBracket, parse_state );

				parse_state.remove_prefix( );

				using iter_t =
				  json_parse_kv_array_iterator<JsonMember, ParseState,
				                               can_be_random_iterator_v<KnownBounds>>;
				using constructor_t = typename JsonMember::constructor_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  iter_t( parse_state ), iter_t( ) );
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_array( ParseState &parse_state ) {
				parse_state.trim_left( );
				daw_json_assert_weak( parse_state.is_opening_bracket_checked( ),
				                      ErrorReason::InvalidArrayStart, parse_state );
				parse_state.remove_prefix( );
				parse_state.trim_left_unchecked( );
				// TODO: add parse option to disable random access iterators. This is
				// coding to the implementations

				using iterator_t =
				  json_parse_array_iterator<JsonMember, ParseState,
				                            can_be_random_iterator_v<KnownBounds>>;
				using constructor_t = typename JsonMember::constructor_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  iterator_t( parse_state ), iterator_t( ) );
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_sz_array( ParseState &parse_state ) {

				using size_member = dependent_member_t<JsonMember>;

				auto [is_found, parse_state2] = find_range<ParseState>(
				  ParseState( parse_state.class_first, parse_state.last ),
				  size_member::name );

				daw_json_ensure( is_found, ErrorReason::TagMemberNotFound,
				                 parse_state );
				auto const sz = parse_value<size_member>(
				  parse_state2, ParseTag<size_member::expected_type>{ } );

				if constexpr( KnownBounds and ParseState::is_unchecked_input ) {
					// We have the requested size and the actual size.  Let's see if they
					// match
					auto cnt = static_cast<std::ptrdiff_t>( parse_state.counter );
					daw_json_ensure( sz >= 0 and ( cnt < 0 or parse_state.counter == sz ),
					                 ErrorReason::NumberOutOfRange, parse_state );
				}
				parse_state.trim_left( );
				daw_json_assert_weak( parse_state.is_opening_bracket_checked( ),
				                      ErrorReason::InvalidArrayStart, parse_state );
				parse_state.remove_prefix( );
				parse_state.trim_left_unchecked( );
				// TODO: add parse option to disable random access iterators. This is
				// coding to the implementations
				using iterator_t =
				  json_parse_array_iterator<JsonMember, ParseState, false>;
				using constructor_t = typename JsonMember::constructor_t;
				return construct_value(
				  template_args<json_result<JsonMember>, constructor_t>, parse_state,
				  iterator_t( parse_state ), iterator_t( ),
				  static_cast<std::size_t>( sz ) );
			}

			template<JsonBaseParseTypes BPT, typename JsonMembers, bool KnownBounds,
			         typename ParseState>
			[[nodiscard]] DAW_ATTRIB_FLATINLINE static constexpr json_result<
			  JsonMembers>
			parse_variant_value( ParseState &parse_state ) {
				using element_t = typename JsonMembers::json_elements;
				using idx = daw::constant<(
				  JsonMembers::base_map::base_map[static_cast<int_fast8_t>( BPT )] )>;

				if constexpr( idx::value <
				              pack_size_v<typename element_t::element_map_t> ) {
					using JsonMember =
					  pack_element_t<idx::value, typename element_t::element_map_t>;
					return parse_value<JsonMember, KnownBounds>(
					  parse_state, ParseTag<JsonMember::expected_type>{ } );
				} else {
					daw_json_error( ErrorReason::UnexpectedJSONVariantType );
				}
			}

			template<typename JsonMember, bool KnownBounds, typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_variant( ParseState &parse_state ) {
				if constexpr( KnownBounds ) {
					// We are only in this branch when a member has been skipped.  This
					// means we can look backwards
					if( *( parse_state.first - 1 ) == '"' ) {
						// We are a string, the skipper trims them
						return parse_variant_value<JsonBaseParseTypes::String, JsonMember,
						                           KnownBounds>( parse_state );
					}
				}
				switch( parse_state.front( ) ) {
				case '{':
					return parse_variant_value<JsonBaseParseTypes::Class, JsonMember,
					                           KnownBounds>( parse_state );
				case '[':
					return parse_variant_value<JsonBaseParseTypes::Array, JsonMember,
					                           KnownBounds>( parse_state );
				case 't':
				case 'f':
					return parse_variant_value<JsonBaseParseTypes::Bool, JsonMember,
					                           KnownBounds>( parse_state );
				case '"':
					return parse_variant_value<JsonBaseParseTypes::String, JsonMember,
					                           KnownBounds>( parse_state );
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '+':
				case '-':
					return parse_variant_value<JsonBaseParseTypes::Number, JsonMember,
					                           KnownBounds>( parse_state );
				}
				if constexpr( ParseState::is_unchecked_input ) {
					DAW_UNREACHABLE( );
				} else {
					daw_json_error( ErrorReason::InvalidStartOfValue, parse_state );
				}
			}

			template<typename Result, typename TypeList, std::size_t pos = 0,
			         typename ParseState>
			DAW_ATTRIB_INLINE static constexpr Result
			parse_visit( std::size_t idx, ParseState &parse_state ) {
				if( idx == pos ) {
					using JsonMember = pack_element_t<pos, TypeList>;
					if constexpr( std::is_same_v<json_result<JsonMember>, Result> ) {
						return parse_value<JsonMember>(
						  parse_state, ParseTag<JsonMember::expected_type>{ } );
					} else {
						return Result{ parse_value<JsonMember>(
						  parse_state, ParseTag<JsonMember::expected_type>{ } ) };
					}
				}
				if constexpr( pos + 1 < pack_size_v<TypeList> ) {
					return parse_visit<Result, TypeList, pos + 1>( idx, parse_state );
				} else {
					if constexpr( ParseState::is_unchecked_input ) {
						DAW_UNREACHABLE( );
					} else {
						daw_json_error( ErrorReason::MissingMemberNameOrEndOfClass,
						                parse_state );
					}
				}
			}

			template<typename JsonMember, typename ParseState>
			static constexpr auto find_index( ParseState const & parse_state ) {
				using tag_member = typename JsonMember::tag_member;
				using class_wrapper_t = typename JsonMember::tag_member_class_wrapper;

				using switcher_t = typename JsonMember::switcher;
				auto parse_state2 =
				  ParseState( parse_state.class_first, parse_state.class_last,
				              parse_state.class_first, parse_state.class_last );
				if constexpr( is_an_ordered_member_v<tag_member> ) {
					// This is an ordered class, class must start with '['
					daw_json_assert_weak( parse_state2.is_opening_bracket_checked( ),
					                      ErrorReason::InvalidArrayStart, parse_state );
					return switcher_t{ }( std::get<0>( parse_value<class_wrapper_t>(
					  parse_state2, ParseTag<class_wrapper_t::expected_type>{ } ) ) );
				} else {
					// This is a regular class, class must start with '{'
					daw_json_assert_weak( parse_state2.is_opening_brace_checked( ),
					                      ErrorReason::InvalidClassStart, parse_state );
					return switcher_t{ }( std::get<0>(
					  parse_value<class_wrapper_t>(
					    parse_state2, ParseTag<class_wrapper_t::expected_type>{ } )
					    .members ) );
				}
			}

			template<typename JsonMember, typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_variant_tagged( ParseState &parse_state ) {
				auto const index = find_index<JsonMember>( parse_state );
				return parse_visit<json_result<JsonMember>,
				                   typename JsonMember::json_elements::element_map_t>(
				  index, parse_state );
			}

			template<typename JsonMember, typename ParseState>
			[[nodiscard]] static constexpr json_result<JsonMember>
			parse_value_variant_intrusive( ParseState &parse_state ) {
				auto const index = [&] {
					using tag_submember = typename JsonMember::tag_submember;
					using class_wrapper_t =
					  typename JsonMember::tag_submember_class_wrapper;
					auto parse_state2 = parse_state;
					using switcher_t = typename JsonMember::switcher;
					if constexpr( is_an_ordered_member_v<tag_submember> ) {
						return switcher_t{ }( std::get<0>( parse_value<class_wrapper_t>(
						  parse_state2, ParseTag<class_wrapper_t::expected_type>{ } ) ) );
					} else {
						return switcher_t{ }( std::get<0>(
						  parse_value<class_wrapper_t>(
						    parse_state2, ParseTag<class_wrapper_t::expected_type>{ } )
						    .members ) );
					}
				}( );

				return parse_visit<json_result<JsonMember>,
				                   typename JsonMember::json_elements::element_map_t>(
				  index, parse_state );
			}

			template<bool AllMembersMustExist, typename ParseState,
			         typename OldClassPos>
			DAW_ATTRIB_INLINE static constexpr void ordered_class_cleanup(
			  ParseState &parse_state,
			  OldClassPos const
			    &old_class_pos ) noexcept( not use_daw_json_exceptions_v ) {
				if constexpr( AllMembersMustExist ) {
					parse_state.trim_left( );
					daw_json_assert_weak( parse_state.front( ) == ']',
					                      ErrorReason::UnknownMember, parse_state );
					parse_state.remove_prefix( );
					parse_state.trim_left_checked( );
				} else {
					(void)parse_state.skip_array( );
				}
				parse_state.set_class_position( old_class_pos );
			}

			namespace pocm_details {
				/***
				 * Maybe skip json members
				 * @tparam ParseState see IteratorRange
				 * @param parse_state JSON data
				 * @param current_position current member index
				 * @param desired_position desired member index
				 */
				template<bool Nullable, typename ParseState>
				DAW_ATTRIB_INLINE static constexpr void
				maybe_skip_members( ParseState &parse_state,
				                    std::size_t &current_position,
				                    std::size_t desired_position ) {

					daw_json_assert_weak( current_position <= desired_position,
					                      ErrorReason::OutOfOrderOrderedMembers,
					                      parse_state );
					using skip_check_end =
					  std::bool_constant<( ParseState::is_unchecked_input and Nullable )>;
					while( ( current_position < desired_position ) &
					       ( skip_check_end::value or parse_state.front( ) != ']' ) ) {
						(void)skip_value( parse_state );
						parse_state.move_next_member_or_end( );
						++current_position;
						daw_json_assert_weak( parse_state.has_more( ),
						                      ErrorReason::UnexpectedEndOfData,
						                      parse_state );
					}
				}

#if defined( DAW_JSON_BUGFIX_MSVC_EVAL_ORDER_002 )
				template<typename ParseState>
				struct position_info {
					std::size_t index;
					ParseState state{ };

					constexpr explicit operator bool( ) const {
						return not state.is_null( );
					}
				};

				/***
				 * Maybe skip json members
				 * @tparam ParseState see IteratorRange
				 * @param parse_state JSON data
				 * @param current_position current member index
				 * @param desired_position desired member index
				 */
				template<bool Nullable, typename ParseState, std::size_t N>
				DAW_ATTRIB_INLINE static constexpr ParseState maybe_skip_members(
				  ParseState &parse_state, std::size_t &current_position,
				  std::size_t desired_position,
				  std::array<position_info<ParseState>, N> &parse_locations ) {

					auto const desired = daw::algorithm::find_if(
					  std::data( parse_locations ), daw::data_end( parse_locations ),
					  [desired_position]( position_info<ParseState> const &loc ) {
						  return loc.index == desired_position;
					  } );
					if( *desired ) {
						return desired->state;
					}
#if not defined( NDEBUG )
					daw_json_ensure( desired != daw::data_end( parse_locations ),
					                 ErrorReason::UnexpectedEndOfData, parse_state );
#endif
					using skip_check_end =
					  std::bool_constant<( ParseState::is_unchecked_input and Nullable )>;
					while( ( current_position < desired_position ) &
					       ( skip_check_end::value or parse_state.front( ) != ']' ) ) {
						auto const current = daw::algorithm::find_if(
						  std::data( parse_locations ), daw::data_end( parse_locations ),
						  [current_position]( position_info<ParseState> const &loc ) {
							  return loc.index == current_position;
						  } );
						auto state = skip_value( parse_state );
						if( current != daw::data_end( parse_locations ) ) {
							current->state = state;
						}
						parse_state.move_next_member_or_end( );
						++current_position;
						daw_json_assert_weak( parse_state.has_more( ),
						                      ErrorReason::UnexpectedEndOfData,
						                      parse_state );
					}
					return parse_state;
				}
#endif
				template<typename T>
				using has_member_index_test = decltype( T::member_index );

				template<typename T>
				inline constexpr bool has_member_index_v =
				  daw::is_detected_v<has_member_index_test, T>;

				template<typename T>
				struct member_index_t {
					static constexpr std::size_t value = T::member_index;
				};

				template<typename Idx, typename JsonMember>
				inline constexpr std::size_t member_index_v =
				  std::conditional_t<has_member_index_v<JsonMember>,
				                     member_index_t<JsonMember>, Idx>::value;
			} // namespace pocm_details

			template<typename JsonMember, bool KnownBounds, typename ParseState,
			         std::size_t... Is>
			DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_tuple_value( ParseState &parse_state, std::index_sequence<Is...> ) {
				parse_state.trim_left( );
				daw_json_assert_weak( parse_state.is_opening_bracket_checked( ),
				                      ErrorReason::InvalidArrayStart, parse_state );

				auto const old_class_pos = parse_state.get_class_position( );
				parse_state.set_class_position( );
				parse_state.remove_prefix( );
				parse_state.move_next_member_or_end( );
				using tuple_t = typename JsonMember::base_type;
				using tuple_members = typename JsonMember::sub_member_list;

#if defined( DAW_JSON_BUGFIX_MSVC_EVAL_ORDER_002 )
				using position_info_t = pocm_details::position_info<ParseState>;
				std::size_t parse_locations_last_index = 0U;
				std::array<position_info_t, sizeof...( Is )> parse_locations{
				  [&]( auto Index ) mutable -> position_info_t {
					  constexpr std::size_t index = decltype( Index )::value;
					  using member_t = std::tuple_element_t<index, tuple_members>;
					  if constexpr( is_an_ordered_member_v<member_t> ) {
						  parse_locations_last_index = member_t::member_index;
						  return { member_t::member_index };
					  } else {
						  return { parse_locations_last_index++ };
					  }
				  }( daw::constant<Is>{ } )... };
#endif
				auto const parse_value_help = [&]( auto PackIdx,
				                                   std::size_t &ClassIdx ) {
					using index_t = decltype( PackIdx );
					using CurrentMember =
					  std::tuple_element_t<index_t::value, tuple_members>;

					using json_member_t = ordered_member_subtype_t<CurrentMember>;

#if defined( DAW_JSON_BUGFIX_MSVC_EVAL_ORDER_002 )
					ParseState parse_state2 =
					  pocm_details::maybe_skip_members<is_json_nullable_v<json_member_t>>(
					    parse_state, ClassIdx, /*index_t::value*/
					    pocm_details::member_index_v<index_t, CurrentMember>,
					    parse_locations );
					if constexpr( sizeof...( Is ) > 1 ) {
						++ClassIdx;
						if( parse_state2.first == parse_state.first ) {
							if constexpr( is_pinned_type_v<
							                typename JsonMember::parse_to_t> ) {
								auto const run_after_parse = daw::on_exit_success( [&] {
									parse_state.move_next_member_or_end( );
								} );
								(void)run_after_parse;
								return parse_value<json_member_t>(
								  parse_state, ParseTag<json_member_t::expected_type>{ } );
							} else {
								auto result = parse_value<json_member_t>(
								  parse_state, ParseTag<json_member_t::expected_type>{ } );
								parse_state.move_next_member_or_end( );
								return result;
							}
						} else {
							// Known Bounds
							return parse_value<json_member_t, true>(
							  parse_state2, ParseTag<json_member_t::expected_type>{ } );
						}
					} else {
#endif
						if constexpr( is_an_ordered_member_v<CurrentMember> ) {
							pocm_details::maybe_skip_members<
							  is_json_nullable_v<json_member_t>>(
							  parse_state, ClassIdx, CurrentMember::member_index );
						} else {
							daw_json_assert_weak( parse_state.has_more( ),
							                      ErrorReason::UnexpectedEndOfData,
							                      parse_state );
						}
						++ClassIdx;
						if constexpr( is_pinned_type_v<typename JsonMember::parse_to_t> ) {
							auto const run_after_parse = daw::on_exit_success( [&] {
								parse_state.move_next_member_or_end( );
							} );
							(void)run_after_parse;
							return parse_value<json_member_t>(
							  parse_state, ParseTag<json_member_t::expected_type>{ } );
						} else {
							auto result = parse_value<json_member_t>(
							  parse_state, ParseTag<json_member_t::expected_type>{ } );
							parse_state.move_next_member_or_end( );
							return result;
						}
#if defined( DAW_JSON_BUGFIX_MSVC_EVAL_ORDER_002 )
					}
#endif
				};

				static_assert( is_a_json_type_v<JsonMember> );
				using Constructor = typename JsonMember::constructor_t;

				parse_state.trim_left( );

				std::size_t class_idx = 0;
				if constexpr( is_pinned_type_v<typename JsonMember::parse_to_t> ) {
					auto const run_after_parse = daw::on_exit_success( [&] {
						ordered_class_cleanup<json_details::all_json_members_must_exist_v<
						                        JsonMember, ParseState>,
						                      ParseState, decltype( old_class_pos )>(
						  parse_state, old_class_pos );
					} );
					(void)run_after_parse;
					if constexpr( should_construct_explicitly_v<Constructor, tuple_t,
					                                            ParseState> ) {
						return tuple_t{
						  parse_value_help( daw::constant<Is>{ }, class_idx )... };
					} else {
						return construct_value_tp<tuple_t, Constructor>(
						  parse_state, fwd_pack{ parse_value_help( daw::constant<Is>{ },
						                                           class_idx )... } );
					}
				} else {
					auto result = [&] {
						if constexpr( should_construct_explicitly_v<Constructor, tuple_t,
						                                            ParseState> ) {
							return tuple_t{
							  parse_value_help( daw::constant<Is>{ }, class_idx )... };
						} else {
							return construct_value_tp<tuple_t, Constructor>(
							  parse_state, fwd_pack{ parse_value_help( daw::constant<Is>{ },
							                                           class_idx )... } );
						}
					}( );
					if constexpr( json_details::all_json_members_must_exist_v<
					                tuple_t, ParseState> ) {
						parse_state.trim_left( );
						daw_json_assert_weak( parse_state.front( ) == ']',
						                      ErrorReason::UnknownMember, parse_state );
						parse_state.remove_prefix( );
						parse_state.trim_left( );
					} else {
						(void)parse_state.skip_array( );
					}
					parse_state.set_class_position( old_class_pos );
					return result;
				}
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			static constexpr json_result<JsonMember>
			parse_value_tuple( ParseState &parse_state ) {
				using element_pack =
				  typename JsonMember::sub_member_list; // tuple_elements_pack<tuple_t>;

				return parse_tuple_value<JsonMember, KnownBounds>(
				  parse_state,
				  std::make_index_sequence<std::tuple_size_v<element_pack>>{ } );
			}

			template<typename JsonMember, bool KnownBounds = false,
			         typename ParseState>
			DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value_unknown( ParseState &parse_state ) {
				using constructor_t = typename JsonMember::constructor_t;
				if constexpr( KnownBounds ) {
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  std::data( parse_state ), std::size( parse_state ) );
				} else {
					auto value_parse_state = skip_value( parse_state );
					return construct_value(
					  template_args<json_result<JsonMember>, constructor_t>, parse_state,
					  std::data( value_parse_state ), std::size( value_parse_state ) );
				}
			}

			template<typename JsonMember, bool KnownBounds, typename ParseState,
			         JsonParseTypes PTag>
			[[nodiscard]] DAW_ATTRIB_INLINE static constexpr json_result<JsonMember>
			parse_value( ParseState &parse_state, ParseTag<PTag> ) {
				if constexpr( PTag == JsonParseTypes::Real ) {
					return parse_value_real<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Signed ) {
					return parse_value_signed<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Unsigned ) {
					return parse_value_unsigned<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Null ) {
					return parse_value_null<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Bool ) {
					return parse_value_bool<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::StringRaw ) {
					return parse_value_string_raw<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::StringEscaped ) {
					return parse_value_string_escaped<JsonMember, KnownBounds>(
					  parse_state );
				} else if constexpr( PTag == JsonParseTypes::Date ) {
					return parse_value_date<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Custom ) {
					return parse_value_custom<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Class ) {
					return parse_value_class<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::KeyValue ) {
					return parse_value_keyvalue<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::KeyValueArray ) {
					return parse_value_keyvalue_array<JsonMember, KnownBounds>(
					  parse_state );
				} else if constexpr( PTag == JsonParseTypes::Array ) {
					return parse_value_array<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::SizedArray ) {
					return parse_value_sz_array<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Variant ) {
					return parse_value_variant<JsonMember, KnownBounds>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::VariantTagged ) {
					return parse_value_variant_tagged<JsonMember>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::VariantIntrusive ) {
					return parse_value_variant_intrusive<JsonMember>( parse_state );
				} else if constexpr( PTag == JsonParseTypes::Tuple ) {
					return parse_value_tuple<JsonMember, KnownBounds>( parse_state );
				} else /*if constexpr( PTag == JsonParseTypes::Unknown )*/ {
					static_assert( PTag == JsonParseTypes::Unknown,
					               "Unexpected JsonParseType" );
					return parse_value_unknown<JsonMember, KnownBounds>( parse_state );
				}
			}

			template<std::size_t N, typename JsonClass, bool KnownBounds,
			         typename... JsonClasses, typename ParseState>
			DAW_ATTRIB_INLINE constexpr json_result<JsonClass>
			parse_nth_class( std::size_t idx, ParseState &parse_state ) {
				// Precondition of caller to verify/ensure.
				DAW_ASSUME( idx < sizeof...( JsonClasses ) );
				using T = typename JsonClass::base_type;
				using Constructor = typename JsonClass::constructor_t;
				if constexpr( sizeof...( JsonClasses ) >= N + 8 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 2: {
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 3: {
						using cur_json_class_t = traits::nth_element<N + 3, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 4: {
						using cur_json_class_t = traits::nth_element<N + 4, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 5: {
						using cur_json_class_t = traits::nth_element<N + 5, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 6: {
						using cur_json_class_t = traits::nth_element<N + 6, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 7: {
						using cur_json_class_t = traits::nth_element<N + 7, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default:
						if constexpr( sizeof...( JsonClasses ) >= N + 8 ) {
							return parse_nth_class<N + 8, JsonClass, KnownBounds,
							                       JsonClasses...>( idx, parse_state );
						} else {
							DAW_UNREACHABLE( );
						}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 7 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 2: {
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 3: {
						using cur_json_class_t = traits::nth_element<N + 3, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 4: {
						using cur_json_class_t = traits::nth_element<N + 4, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 5: {
						using cur_json_class_t = traits::nth_element<N + 5, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default: {
						DAW_ASSUME( idx == N + 6 );
						using cur_json_class_t = traits::nth_element<N + 6, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 6 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 2: {
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 3: {
						using cur_json_class_t = traits::nth_element<N + 3, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 4: {
						using cur_json_class_t = traits::nth_element<N + 4, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default: {
						DAW_ASSUME( idx == N + 5 );
						using cur_json_class_t = traits::nth_element<N + 5, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 5 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 2: {
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 3: {
						using cur_json_class_t = traits::nth_element<N + 3, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default: {
						DAW_ASSUME( idx == N + 4 );
						using cur_json_class_t = traits::nth_element<N + 4, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 4 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 2: {
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default: {
						DAW_ASSUME( idx == N + 3 );
						using cur_json_class_t = traits::nth_element<N + 3, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 3 ) {
					switch( idx ) {
					case N + 0: {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					case N + 1: {
						using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					default: {
						DAW_ASSUME( idx == N + 2 );
						using cur_json_class_t = traits::nth_element<N + 2, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					}
				} else if constexpr( sizeof...( JsonClasses ) == N + 2 ) {
					if( idx == N ) {
						using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
						return construct_value(
						  template_args<T, Constructor>, parse_state,
						  parse_value<cur_json_class_t>(
						    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
					}
					using cur_json_class_t = traits::nth_element<N + 1, JsonClasses...>;
					return construct_value(
					  template_args<T, Constructor>, parse_state,
					  parse_value<cur_json_class_t>(
					    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
				} else {
					using cur_json_class_t = traits::nth_element<N + 0, JsonClasses...>;
					return construct_value(
					  template_args<T, Constructor>, parse_state,
					  parse_value<cur_json_class_t>(
					    parse_state, ParseTag<cur_json_class_t::expected_type>{ } ) );
				}
			}
		} // namespace json_details
	}   // namespace DAW_JSON_VER
} // namespace daw::json
