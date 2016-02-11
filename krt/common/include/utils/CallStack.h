#pragma once

namespace krt
{
class CallStack
{
public:
	class StackFrameData
	{
	public:
		virtual ~StackFrameData() = default;
	};

	class StackFrame
	{
	private:
		uintptr_t m_address;
		uintptr_t m_moduleBase;

		std::shared_ptr<StackFrameData> m_resolvedData;

	public:
		StackFrame(uintptr_t address);

		void Resolve();

		std::string FormatLine();
	};

private:
	static void InitializeSymbols();

public:
	static std::unique_ptr<CallStack> Build(int maxSteps, int skipDepth, void* startingPoint);

private:
	std::vector<StackFrame> m_frames;

private:
	inline void AddFrame(const StackFrame& frame)
	{
		m_frames.push_back(frame);
	}

public:
	inline auto begin()
	{
		return m_frames.begin();
	}

	inline auto end()
	{
		return m_frames.end();
	}

	inline size_t GetLength()
	{
		return m_frames.size();
	}

	inline StackFrame* GetFrame(int index)
	{
		return &m_frames[index];
	}
};
}