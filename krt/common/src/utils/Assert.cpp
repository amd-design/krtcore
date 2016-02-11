#include "StdInc.h"
#include <utils/Assert.h>

#include <Win32Resource.h>

#include <atlbase.h>
#include <atlwin.h>

#include <atlapp.h>
#include <atlframe.h>
#include <atlctrls.h>

namespace krt
{
struct AssertDialogDetails
{
	std::wstring file;
	std::wstring line;
	std::wstring module;
	std::wstring expression;
	std::wstring function;
	std::wstring revision;

	std::wstring message;

	std::vector<std::wstring> stack;
};

enum class AssertDialogResult
{
	Abort,
	IgnoreOnce,
	IgnoreAlways,
	Debug
};

class AssertDialog : public CDialogImpl<AssertDialog>, public CDialogResize<AssertDialog>
{
private:
	AssertDialogDetails m_details;
	AssertDialogResult m_result;

public:
	AssertDialog(const AssertDialogDetails& details)
	{
		LoadLibrary(L"riched20.dll");

		m_details = details;
	}

	enum { IDD = ID_ASSERTDIALOG };

BEGIN_MSG_MAP(AssertDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDC_ABORT, OnCloseCmd)
	COMMAND_ID_HANDLER(IDC_IGNOREONCE, OnCloseCmd)
	COMMAND_ID_HANDLER(IDC_IGNOREALWAYS, OnCloseCmd)
	COMMAND_ID_HANDLER(IDC_DEBUG, OnCloseCmd)
	CHAIN_MSG_MAP(CDialogResize<AssertDialog>)
END_MSG_MAP()

BEGIN_DLGRESIZE_MAP(AssertDialog)
	DLGRESIZE_CONTROL(IDC_STACK, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	DLGRESIZE_CONTROL(IDC_STATUS_LINE, DLSZ_SIZE_X | DLSZ_MOVE_Y)
	DLGRESIZE_CONTROL(IDC_EXPRESSION, DLSZ_SIZE_X)
	DLGRESIZE_CONTROL(IDC_MODULE, DLSZ_SIZE_X)
	DLGRESIZE_CONTROL(IDC_DEBUG, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	DLGRESIZE_CONTROL(IDC_IGNOREALWAYS, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	DLGRESIZE_CONTROL(IDC_IGNOREONCE, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	DLGRESIZE_CONTROL(IDC_ABORT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
END_DLGRESIZE_MAP()

	LRESULT OnCloseCmd(WPARAM, LPARAM id, HWND, BOOL&)
	{
		switch (id)
		{
			case IDC_ABORT:
				m_result = AssertDialogResult::Abort;
				break;

			case IDC_DEBUG:
				m_result = AssertDialogResult::Debug;
				break;

			case IDC_IGNOREONCE:
				m_result = AssertDialogResult::IgnoreOnce;
				break;

			case IDC_IGNOREALWAYS:
				m_result = AssertDialogResult::IgnoreAlways;
				break;
		}

		DestroyWindow();

		return 0;
	}

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
	{
		CenterWindow();

		DlgResize_Init();

		WTL::CStatic module(GetDlgItem(IDC_MODULE));
		WTL::CStatic expression(GetDlgItem(IDC_EXPRESSION));
		WTL::CStatic file(GetDlgItem(IDC_FILE));
		WTL::CStatic line(GetDlgItem(IDC_LINE));
		WTL::CStatic function(GetDlgItem(IDC_FUNCTION));
		WTL::CStatic revision(GetDlgItem(IDC_REVISION));
		WTL::CRichEditCtrl message(GetDlgItem(IDC_MESSAGE));
		WTL::CListBox stack(GetDlgItem(IDC_STACK));

		{
			WTL::CStatic icon(GetDlgItem(IDC_OICON));
			WTL::CIcon iconRef(LoadIcon(nullptr, IDI_ERROR));

			icon.SetIcon(iconRef);
		}

		module.SetWindowText(m_details.module.c_str());
		expression.SetWindowText(m_details.expression.c_str());
		file.SetWindowText(m_details.file.c_str());
		line.SetWindowText(m_details.line.c_str());
		function.SetWindowText(m_details.function.c_str());
		revision.SetWindowText(m_details.revision.c_str());
		message.AppendText(m_details.message.c_str(), FALSE);
		
		for (const auto& stackLine : m_details.stack)
		{
			stack.AddString(stackLine.c_str());
		}

		return 0;
	}

	inline AssertDialogResult GetResult()
	{
		return m_result;
	}
};

void Assert(AssertionType type, const char* assertionString, int line, const char* file, const char* function, int* ignoreCounter)
{
	// handle ignoring the assert
	if (ignoreCounter)
	{
		// if the ignore counter isn't 0...
		if (*ignoreCounter != 0)
		{
			// decrement the ignore count if not 'always'
			if (*ignoreCounter != -1)
			{
				--(*ignoreCounter);
			}

			return;
		}
	}

	AssertDialogDetails details;
	details.file = ToWide(file);
	details.function = ToWide(function);
	details.expression = ToWide(assertionString);
	details.message = ToWide(assertionString);
	details.line = std::to_wstring(line);
	details.module = L"";
	details.revision = L"";

	details.stack.push_back(L"unknown");

	std::unique_ptr<AssertDialog> dialog = std::make_unique<AssertDialog>(details);
	dialog->Create(nullptr);

	// Windows message loop
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		// TODO: copy to clipboard

		if (!IsDialogMessage(dialog->m_hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!IsWindow(dialog->m_hWnd))
		{
			break;
		}
	}

	switch (dialog->GetResult())
	{
		case AssertDialogResult::Abort:
			ExitProcess(1);
			break;

		case AssertDialogResult::Debug:
			__debugbreak();
			break;

		case AssertDialogResult::IgnoreAlways:
			if (ignoreCounter)
			{
				*ignoreCounter = -1;
			}

			break;

		case AssertDialogResult::IgnoreOnce:
			break;
	}
}
}