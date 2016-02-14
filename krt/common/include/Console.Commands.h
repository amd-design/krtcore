#pragma once

#include <functional>
#include <list>

#include <sstream>

#include <ProgramArguments.h>

#include <utils/Apply.h>
#include <utils/MakeFunction.h>

#include <utils/IgnoreCaseLess.h>

#include <shared_mutex>

namespace krt
{
// console execution context
struct ConsoleExecutionContext
{
	const ProgramArguments arguments;
	std::stringstream errorBuffer;

	inline ConsoleExecutionContext(const ProgramArguments&& arguments)
	    : arguments(arguments)
	{
	}
};

class ConsoleCommandManager
{
  private:
	using THandler = std::function<bool(ConsoleExecutionContext& context)>;

  public:
	ConsoleCommandManager();

	~ConsoleCommandManager();

	void Register(const std::string& name, const THandler& handler);

	void Invoke(const std::string& commandName, const ProgramArguments& arguments);

	void Invoke(const std::string& commandString);

  private:
	struct Entry
	{
		std::string name;
		THandler function;

		inline Entry(const std::string& name, const THandler& function)
		    : name(name), function(function)
		{
		}
	};

  private:
	std::multimap<std::string, Entry, IgnoreCaseLess> m_entries;

	std::shared_timed_mutex m_mutex;

  public:
	static inline ConsoleCommandManager* GetInstance()
	{
		return ms_instance;
	}

  private:
	static ConsoleCommandManager* ms_instance;
};

template <typename TArgument, typename TConstraint = void>
struct ConsoleArgumentType
{
	static bool Parse(const std::string& input, TArgument* out)
	{
		static_assert(false, "Unknown ConsoleArgumentType parse handler (try defining one?)");
	}
};

template <>
struct ConsoleArgumentType<std::string>
{
	static bool Parse(const std::string& input, std::string* out)
	{
		*out = input;
		return true;
	}
};

template <typename TArgument>
struct ConsoleArgumentType<TArgument, std::enable_if_t<std::is_integral<TArgument>::value>>
{
	static bool Parse(const std::string& input, TArgument* out)
	{
		try
		{
			*out = std::stoi(input);

			// no way to know if an integer is valid this lazy way, sadly :(
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

namespace internal
{
template <typename TArgument>
bool ParseArgument(const std::string& input, TArgument* out)
{
	return ConsoleArgumentType<TArgument>::Parse(input, out);
}

template <class TFunc>
struct ConsoleCommandFunction
{
};

template <typename... Args>
struct ConsoleCommandFunction<std::function<void(Args...)>>
{
	using TFunc = std::function<void(Args...)>;

	using ArgTuple = std::tuple<Args...>;

	static bool Call(TFunc func, ConsoleExecutionContext& context)
	{
		// check if the argument count matches
		if (sizeof...(Args) != context.arguments.Count())
		{
			context.errorBuffer << "Argument count mismatch (passed " << std::to_string(context.arguments.Count()) << ", wanted " << std::to_string(sizeof...(Args)) << ")" << std::endl;
			return false;
		}

		// invoke the recursive template argument tree for parsing our arguments
		return CallInternal<0, std::tuple<>>(func, context, std::tuple<>());
	}

	// non-terminator iterator
	template <size_t Iterator, typename TupleType>
	static std::enable_if_t<(Iterator < sizeof...(Args)), bool> CallInternal(TFunc func, ConsoleExecutionContext& context, TupleType tuple)
	{
		// the type of the current argument
		using ArgType = std::tuple_element_t<Iterator, ArgTuple>;

		std::decay_t<ArgType> argument;
		if (ParseArgument(context.arguments[Iterator], &argument))
		{
			return CallInternal<Iterator + 1>(
			    func,
			    context,
			    std::tuple_cat(std::move(tuple), std::forward_as_tuple(std::forward<ArgType>(argument))));
		}

		context.errorBuffer << "Could not convert argument " << std::to_string(Iterator) << " (" << context.arguments[Iterator] << ") to " << typeid(ArgType).name() << std::endl;

		return false;
	}

	// terminator
	template <size_t Iterator, typename TupleType>
	static std::enable_if_t<(Iterator == sizeof...(Args)), bool> CallInternal(TFunc func, ConsoleExecutionContext& context, TupleType tuple)
	{
		apply(func, std::move(tuple));

		return true;
	}
};
}

class ConsoleCommand
{
  public:
	template <typename TFunction>
	ConsoleCommand(const std::string& name, TFunction function)
	{
		auto functionRef = detail::make_function(function);

		ConsoleCommandManager::GetInstance()->Register(name, [=](ConsoleExecutionContext& context) {
			return internal::ConsoleCommandFunction<decltype(functionRef)>::Call(functionRef, context);
		});
	}
};
}