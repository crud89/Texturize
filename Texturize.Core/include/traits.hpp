#pragma once

#include "texturize.hpp"

/// \namespace Texturize
/// \brief The root namespace, that contains all framework classes.
namespace Texturize {

	/// \brief Generic type trait that checks if a each type in a variadic template pack is derived from a certain base class \ref `Base`.
	/// 
	/// The trait works by recursively checking the first template parameter (which is set to \ref `T`) using the `std::is_base_of` trait and 
	/// continuing with the next one. If the last type parameter in the list is reached, the `are_base_of&lt;Base, T&gt;` trait is used.
	///
	///  **Example**
	/// \code
	/// class Base { };
	///
	/// template <typename... Ts>
	/// class Set {
	///     static_assert(std::are_base_of<Base, Ts...>::value, "All arguments of Set must implement Base");
	/// };
	/// \endcode
	///
	/// \tparam Base The base class, all types should inherit from.
	/// \tparam T The first type to check against base.
	/// \tparam Ts All remaining types that are checked recursively in later iterations.
	/// \see https://stackoverflow.com/questions/22526996/using-conditional-definitions-with-variadic-templates
	template <typename Base, typename T, typename... Ts>
	struct are_base_of :
		std::conditional<std::is_base_of<Base, T>::value,		// Is the first argument `T` in the list derived from `Base`?
		are_base_of<Base, Ts...>,								// Continue checking the rest of the arguments.
		std::false_type>										// Return `std::false_type` if there's no match.
		::type
	{};

	/// \copydoc Texturize::are_base_of
	template <typename Base, typename T>
	struct are_base_of<Base, T> : 
		std::is_base_of<Base, T> 
	{};
}