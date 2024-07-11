#pragma once

#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <windows.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#include "aviutl.hpp"
#include "exedit.hpp"

#undef min
#undef max

constexpr auto plugin_name = "クリップボード画像表示";
constexpr auto plugin_information = "クリップボード画像表示 r1 by 蛇色 / originator sugichu / X";

namespace my
{
	namespace menu
	{
		inline const auto deleter = [](HMENU x){ ::DestroyMenu(x); };

		template <typename T = HMENU>
		using unique_ptr = std::unique_ptr<typename std::remove_pointer<T>::type, decltype(deleter)>;

		template <typename T = HMENU>
		struct shared_ptr : std::shared_ptr<typename std::remove_pointer<T>::type>
		{
			shared_ptr() {}
			shared_ptr(T x) : std::shared_ptr<typename std::remove_pointer<T>::type>(x, deleter) {}
			auto reset(T x) { return __super::reset(x, deleter); }
		};

		template <typename T = HMENU>
		using weak_ptr = std::weak_ptr<typename std::remove_pointer<T>::type>;
	}

	namespace theme
	{
		inline const auto deleter = [](HTHEME x){ ::CloseThemeData(x); };

		template <typename T = HTHEME>
		using unique_ptr = std::unique_ptr<typename std::remove_pointer<T>::type, decltype(deleter)>;

		template <typename T = HTHEME>
		struct shared_ptr : std::shared_ptr<typename std::remove_pointer<T>::type>
		{
			shared_ptr() {}
			shared_ptr(T x) : std::shared_ptr<typename std::remove_pointer<T>::type>(x, deleter) {}
			auto reset(T x) { return __super::reset(x, deleter); }
		};

		template <typename T = HTHEME>
		using weak_ptr = std::weak_ptr<typename std::remove_pointer<T>::type>;
	}

	namespace gdi
	{
		inline const auto deleter = [](HGDIOBJ x){ ::DeleteObject(x); };

		template <typename T = HGDIOBJ>
		using unique_ptr = std::unique_ptr<typename std::remove_pointer<T>::type, decltype(deleter)>;

		template <typename T = HGDIOBJ>
		struct shared_ptr : std::shared_ptr<typename std::remove_pointer<T>::type>
		{
			shared_ptr() {}
			shared_ptr(T x) : std::shared_ptr<typename std::remove_pointer<T>::type>(x, deleter) {}
			auto reset(T x) { return __super::reset(x, deleter); }
		};

		template <typename T = HGDIOBJ>
		using weak_ptr = std::weak_ptr<typename std::remove_pointer<T>::type>;
	}

	namespace dc
	{
		inline const auto deleter = [](HDC x){ ::DeleteDC(x); };

		template <typename T = HDC>
		using unique_ptr = std::unique_ptr<typename std::remove_pointer<T>::type, decltype(deleter)>;

		template <typename T = HDC>
		struct shared_ptr : std::shared_ptr<typename std::remove_pointer<T>::type>
		{
			shared_ptr() {}
			shared_ptr(T x) : std::shared_ptr<typename std::remove_pointer<T>::type>(x, deleter) {}
			auto reset(T x) { return __super::reset(x, deleter); }
		};

		template <typename T = HDC>
		using weak_ptr = std::weak_ptr<typename std::remove_pointer<T>::type>;
	}

	struct GdiObjSelector
	{
		HDC dc = 0;
		HGDIOBJ obj = 0;

		GdiObjSelector(HDC dc, HGDIOBJ obj) : dc(dc), obj(::SelectObject(dc, obj)) {}
		~GdiObjSelector() { ::SelectObject(dc, obj); }
	};

	struct ClientDC
	{
		HWND hwnd = 0;
		HDC dc = 0;

		ClientDC(HWND hwnd) : hwnd(hwnd), dc(::GetDC(hwnd)) {}
		~ClientDC() { if (dc) ::ReleaseDC(hwnd, dc); }
		operator HDC() { return dc; }
	};

	struct WindowDC
	{
		HWND hwnd = 0;
		HDC dc = 0;

		WindowDC(HWND hwnd) : hwnd(hwnd), dc(::GetWindowDC(hwnd)) {}
		WindowDC(HWND hwnd, HRGN rgn, DWORD flags) : hwnd(hwnd), dc(::GetDCEx(hwnd, rgn, flags)) {}
		~WindowDC() { if (dc) ::ReleaseDC(hwnd, dc); }
		operator HDC() { return dc; }
	};

	struct PaintDC
	{
		PAINTSTRUCT ps = {};
		HWND hwnd = 0;
		HDC dc = 0;

		PaintDC(HWND hwnd) : hwnd(hwnd), dc(::BeginPaint(hwnd, &ps)) {}
		~PaintDC() { if (dc) ::EndPaint(hwnd, &ps); }
		operator HDC() { return dc; }
	};
#ifdef _UXTHEME_H_
	struct UxDC
	{
		BP_PAINTPARAMS pp = { sizeof(pp) };
		HDC dc = 0;
		HPAINTBUFFER pb = 0;

		UxDC(HDC dc, LPCRECT rc, BP_BUFFERFORMAT format = BPBF_COMPATIBLEBITMAP) : pb(::BeginBufferedPaint(dc, rc, format, &this->pp, &this->dc)) {}
		~UxDC() { if (pb) ::EndBufferedPaint(pb, TRUE); }
		BOOL is_valid() { return !!pb; }
		operator HDC() { return dc; }
	};
#endif // _UXTHEME_H_
	namespace gdi
	{
		using selector = GdiObjSelector;
	}
}

std::filesystem::path get_module_file_name(HINSTANCE instance)
{
	WCHAR file_name[MAX_PATH] = {};
	::GetModuleFileNameW(instance, file_name, MAX_PATH);
	return file_name;
}
