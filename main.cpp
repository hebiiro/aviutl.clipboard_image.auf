#include "include.hpp"
#include "contents.hpp"

BOOL on_init(AviUtl::FilterPlugin* fp)
{
	// このウィンドウにカテゴリ名を設定します。
	::SetPropW(fp->hwnd, L"aviutl.plugin.category_name", (HANDLE)L"蛇色");

	// ウィンドウスタイルを変更します。
	// 子ウィンドウをクリップするようにします。
	auto style = ::GetWindowLong(fp->hwnd, GWL_STYLE);
	::SetWindowLong(fp->hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	// コンテンツの初期化を実行します。
	contents.init(fp);

	{
		// ファイルにアクセスする準備をします。
		auto module_file_name = get_module_file_name(fp->dll_hinst);
		auto ini_file_name = module_file_name.replace_extension(L".ini");

		// iniファイルから設定を読み込みます。
		contents.set_zoom(::GetPrivateProfileIntW(L"contents", L"zoom", contents.zoom, ini_file_name.wstring().c_str()));

		contents.update(contents.window);
	}

	return TRUE;
}

BOOL on_exit(AviUtl::FilterPlugin* fp)
{
	{
		// ファイルにアクセスする準備をします。
		auto module_file_name = get_module_file_name(fp->dll_hinst);
		auto ini_file_name = module_file_name.replace_extension(L".ini");

		// iniファイルに設定を書き込みます。
		::WritePrivateProfileStringW(L"contents", L"zoom", std::format(L"{}", contents.zoom).c_str(), ini_file_name.wstring().c_str());
	}

	// コンテンツの後始末を実行します。
	contents.exit(fp);

	return TRUE;
}

BOOL on_size(HWND hwnd)
{
	// プラグインウィンドウのクライアント矩形を取得します。
	RECT rc; ::GetClientRect(hwnd, &rc);
	auto w = rc.right - rc.left;
	auto h = rc.bottom - rc.top;

	// コンテンツウィンドウをクライアント領域全体に広げます。
	::SetWindowPos(contents.window, nullptr,
		rc.left, rc.top, w, h, SWP_NOZORDER);

	return TRUE;
}

BOOL func_wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	switch (message)
	{
	case AviUtl::FilterPlugin::WindowMessage::Init:
		{
			on_init(fp);

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::Exit:
		{
			on_exit(fp);

			break;
		}
	case WM_SIZE:
		{
			on_size(hwnd);

			break;
		}
	}

	return FALSE;
}

EXTERN_C AviUtl::FilterPluginDLL* WINAPI GetFilterTable()
{
	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
			AviUtl::FilterPluginDLL::Flag::AlwaysActive |
			AviUtl::FilterPluginDLL::Flag::DispFilter |
			AviUtl::FilterPluginDLL::Flag::WindowSize |
			AviUtl::FilterPluginDLL::Flag::WindowThickFrame |
			AviUtl::FilterPluginDLL::Flag::ExInformation,
		.x = 600,
		.y = 400,
		.name = plugin_name,
		.func_WndProc = func_wnd_proc,
		.information = plugin_information,
	};

	return &filter;
}
