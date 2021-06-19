// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_json_link_types.h"
#include "impl/to_daw_json_string.h"

namespace daw::json {
	inline namespace DAW_JSON_VER {
		namespace json_details {

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Bool>,
			                                         OutputIterator out_it ) {
				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"bool")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Custom>,
			                                         OutputIterator out_it ) {
				// TODO allow a trait to describe the valid literal types or if it
				// matches one of the other predefined types
				static_assert( JsonMember::custom_json_type ==
				               CustomJsonTypes::String );
				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"string")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Date>,
			                                         OutputIterator out_it ) {

				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator(
				  out_it, R"("type":"string","format":"date-time")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Real>,
			                                         OutputIterator out_it ) {

				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"number")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Signed>,
			                                         OutputIterator out_it ) {

				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"integer")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::StringEscaped>,
			                OutputIterator out_it ) {

				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"string")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::StringRaw>,
			                OutputIterator out_it ) {

				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it = utils::copy_to_iterator( out_it, R"("type":"string")" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::Unsigned>,
			                OutputIterator out_it ) {
				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it =
				  utils::copy_to_iterator( out_it, R"("type":"integer","minimum":0)" );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			/***
			 * Output the schema of a json_class and it's members
			 * @tparam JsonMember A json_class type
			 * @tparam is_root Is this the root item in the schema.
			 * @tparam OutputIterator An iterator type that allows for assigning to
			 * the result of operator* and pre/post-fix incrementing
			 * @param out_it Current OutputIterator
			 * @return the last value of out_it
			 */
			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Class>,
			                                         OutputIterator out_it );

			/***
			 * Output the schema of a json_array and it's element type
			 * @tparam JsonMember A json_array type
			 * @tparam is_root Is this the root item in the schema.
			 * @tparam OutputIterator An iterator type that allows for assigning to
			 * the result of operator and pre/post-fix incrementing
			 * @param out_it Current OutputIterator
			 * @return the last value of out_it
			 */
			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Array>,
			                                         OutputIterator out_it );

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::KeyValue>,
			                OutputIterator out_it );

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::KeyValueArray>,
			                OutputIterator out_it );

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::Variant>,
			                OutputIterator out_it );

			template<typename JsonMember, bool is_root = false,
			         typename OutputIterator>
			constexpr OutputIterator
			to_json_schema( ParseTag<JsonParseTypes::VariantTagged>,
			                OutputIterator out_it );

			template<typename, typename>
			struct json_class_processor;

			template<typename OutputIterator, typename... JsonMembers>
			struct json_class_processor<OutputIterator,
			                            json_member_list<JsonMembers...>> {

				static constexpr OutputIterator process( OutputIterator out_it ) {
					out_it = utils::copy_to_iterator(
					  out_it, R"("type":"object","properties":{)" );
					out_it = output_member_types( out_it );
					out_it = utils::copy_to_iterator( out_it, R"(},"required":[)" );
					out_it = output_required_members( out_it );
					*out_it++ = ']';
					return out_it;
				}

			private:
				static constexpr auto indexer =
				  std::index_sequence_for<JsonMembers...>{ };

				template<typename JsonMember>
				static constexpr OutputIterator
				output_member_type( OutputIterator &out_it, bool &is_first ) {
					if( not is_first ) {
						*out_it++ = ',';
					} else {
						is_first = false;
					}
					*out_it++ = '"';
					out_it = utils::copy_to_iterator( out_it, JsonMember::name );
					out_it = utils::copy_to_iterator( out_it, R"(":)" );
					out_it = to_json_schema<JsonMember>(
					  ParseTag<JsonMember::base_expected_type>{ }, out_it );
					return out_it;
				}

				static constexpr OutputIterator
				output_member_types( OutputIterator &out_it ) {
					bool is_first = true;
					return static_cast<OutputIterator>(
					  ( output_member_type<JsonMembers>( out_it, is_first ), ... ) );
				}

				template<typename JsonMember>
				static constexpr OutputIterator
				output_required_member( OutputIterator &out_it, bool &is_first ) {
					if constexpr( JsonMember::nullable == JsonNullable::MustExist ) {
						if( not is_first ) {
							*out_it++ = ',';
						} else {
							is_first = false;
						}
						*out_it++ = '"';
						out_it = utils::copy_to_iterator( out_it, JsonMember::name );
						*out_it++ = '"';
					}
					return out_it;
				}

				static constexpr OutputIterator
				output_required_members( OutputIterator &out_it ) {
					bool is_first = true;
					return ( output_required_member<JsonMembers>( out_it, is_first ),
					         ... );
				}
			};

			template<typename OutputIterator, typename... JsonMembers>
			struct json_class_processor<OutputIterator,
			                            json_ordered_member_list<JsonMembers...>> {

				static constexpr OutputIterator process( OutputIterator out_it ) {
					out_it = utils::copy_to_iterator(
					  out_it, R"("type":"object","properties":{)" );
					out_it = output_member_types( out_it );
					out_it = utils::copy_to_iterator( out_it, R"(},"required":[)" );
					out_it = output_required_members( out_it );
					*out_it++ = ']';
					return out_it;
				}
			};
			template<typename JsonMember, bool is_root, typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Class>,
			                                         OutputIterator out_it ) {
				if constexpr( not is_root ) {
					*out_it++ = '{';
				}

				using json_class_processor_t = json_class_processor<
				  OutputIterator,
				  json_data_contract_trait_t<typename JsonMember::base_type>>;

				out_it = json_class_processor_t::process( out_it );

				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

			template<typename JsonMember, bool is_root, typename OutputIterator>
			constexpr OutputIterator to_json_schema( ParseTag<JsonParseTypes::Array>,
			                                         OutputIterator out_it ) {
				if constexpr( not is_root ) {
					*out_it++ = '{';
				}
				out_it =
				  utils::copy_to_iterator( out_it, R"("type":"array","items":)" );
				using element_t = typename JsonMember::json_element_t;
				out_it = to_json_schema<element_t>(
				  ParseTag<element_t::base_expected_type>{ }, out_it );
				if constexpr( not is_root ) {
					*out_it++ = '}';
				}
				return out_it;
			}

		} // namespace json_details
	}   // namespace DAW_JSON_VER
} // namespace daw::json

namespace daw::json {
	inline namespace DAW_JSON_VER {
		template<typename T, typename OutputIterator>
		constexpr OutputIterator to_json_schema( OutputIterator out_it,
		                                         std::string_view id,
		                                         std::string_view title ) {
			*out_it++ = '{';
			out_it = utils::copy_to_iterator(
			  out_it,
			  R"("$schema":"https://json-schema.org/draft/2020-12/schema",)" );
			out_it = utils::copy_to_iterator( out_it, R"("$id":")" );
			out_it = utils::copy_to_iterator( out_it, id );
			out_it = utils::copy_to_iterator( out_it, R"(","title":")" );
			out_it = utils::copy_to_iterator( out_it, title );
			out_it = utils::copy_to_iterator( out_it, R"(",)" );

			using json_type = json_link_no_name<T>;
			out_it = json_details::to_json_schema<json_type, true>(
			  ParseTag<json_type::base_expected_type>{ }, out_it );
			*out_it++ = '}';
			return out_it;
		}

		template<typename T>
		std::string to_json_schema( std::string_view id, std::string_view title ) {
			auto result = std::string( );
			(void)to_json_schema<T>( std::back_inserter( result ), id, title );
			return result;
		}

	} // namespace DAW_JSON_VER
} // namespace daw::json
