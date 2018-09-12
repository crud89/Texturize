#pragma once

#include "texturize.hpp"

#include <functional>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Infrastructure		                                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

/// \namespace Texturize
/// \brief The root namespace, that contains all framework classes.
namespace Texturize {

	/// \brief A class that can be used to report events.
	/// \tparam TResult The return type of a function, events can be reported to.
	/// \tparam TArgs The parameters of a function, events can be reported to.
	///
	/// The `EventDispatcher` class is designed to be used by objects in order to provide callback functionality on certain events. Typically an object
	/// provides an `EventDispatcher` instance to other objects, that can register callback functions on it. The arguments and return value of those
	/// callbacks are stated by the `TArgs` and `TResult` parameters.
	///
	/// **Example**
	/// \include EventDispatcher.cpp
	///
	/// The example demonstrates how a `Producer` class provides an `EventDispatcher` instance, that consumers can add event listeners to. Event listeners
	/// can be provided as functions, methods or lambdas. In the example, the boolean return value of an event listener can be used to cancel the 
	/// producers workflow by returning `false`.
	///
	/// \see Texturize::PyramidSynthesisSettings::ProgressHandler
	template<typename TResult, typename... TArgs>
	class TEXTURIZE_API EventDispatcher {
	public:
		/// \brief Defines a function suitable to be used as a callback.
		typedef std::function<TResult(TArgs...)> Callback;

	private:
		std::vector<Callback> _callbacks;

	public:
		/// \brief Creates a new event dispatcher instance.
		EventDispatcher() = default;
		~EventDispatcher() = default;

		/// \brief Creates a new event dispatcher instance.
		/// \param callback A callback to initialize the instance with.
		EventDispatcher(Callback callback) :
			_callbacks(callback)
		{
		}

		/// \brief Creates a new event dispatcher instance.
		/// \param callbacks A set of callbacks to initialize the instance with.
		EventDispatcher(const std::vector<Callback>& callbacks) :
			_callbacks(callbacks)
		{
		}

		/// \brief Creates a new event dispatcher instance.
		/// \param callbacks A set of callbacks to initialize the instance with.
		///
		/// This constructor uses the `std::initializer_list` to provide multiple callbacks using the simplified syntax `EventDispatcher({ CallbackA, CallbackB, CallbackC })`.
		EventDispatcher(std::initializer_list<Callback> callbacks) :
			_callbacks(callbacks)
		{
		}

	public:
		/// \brief Adds a callback to the event dispatcher.
		/// \tparam TFunctor A function definition that must be compatible to the \ref `Callback` type.
		/// \param functor The callback function to add to the dispatcher.
		template <typename TFunctor>
		void add(TFunctor functor)
		{
			_callbacks.push_back(functor);
		}

		/// \brief Adds a callback to the event dispatcher.
		/// \param callback The callback function to add to the dispatcher.
		void add(const Callback callback)
		{
			_callbacks.push_back(callback);
		}

		/// \brief Removes a callback from the event dispatcher.
		/// \param callback The callback to remove.
		void remove(const Callback callback)
		{
			_callbacks.erase(callback);
		}

		/// \brief Sends the arguments to all callbacks.
		/// \param args The arguments to send to all callbacks.
		///
		/// This function is only provided, if `TResult` is defined `void`.
		template <typename = typename std::enable_if<std::is_void<TResult>::value>::type>
		void execute(TArgs... args) const
		{
			for each (Callback cb in _callbacks)
				cb(args...);
		}

		/// \brief Sends the arguments to all callbacks.
		/// \param args The arguments to send to all callbacks.
		///
		/// This function is only provided, if `TResult` is *not* defined `void`.
		template <typename = typename std::enable_if<!std::is_void<TResult>::value>::type>
		void execute(std::vector<TResult>& results, TArgs... args) const
		{
			results.clear();

			for each (Callback cb in _callbacks)
				results.push_back(cb(args...));
		}
	};

	/// \brief A trivial event dispatcher that accepts no parameters and returns nothing.
	typedef EventDispatcher<void> TrivialSignalDispatcher;
};