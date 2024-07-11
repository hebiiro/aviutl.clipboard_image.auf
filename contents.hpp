#pragma once

inline struct
{
	inline static constexpr struct CommandID {
		inline static constexpr UINT c_erase_current_bitmap = 1000;
		inline static constexpr UINT c_clear_all_bitmaps = 1001;
		inline static constexpr UINT c_zoom_in = 1002;
		inline static constexpr UINT c_zoom_out = 1003;
		inline static constexpr UINT c_zoom_reset = 1004;
		inline static constexpr UINT c_backward = 1005;
		inline static constexpr UINT c_forward = 1006;
	} c_command_id;

	inline static constexpr struct Zoom {
		inline static constexpr int c_default = 100;
		inline static constexpr int c_min = 25;
		inline static constexpr int c_max = 800;
	} c_zoom;

	//
	// クリップボードから取得した画像のコレクションです。
	//
	std::vector<my::gdi::shared_ptr<HBITMAP>> bitmaps;

	//
	// コンテンツウィンドウで現在表示している画像のインデックスです。
	//
	int current_bitmap_index = 0;

	//
	// 画像の表示倍率です。
	//
	int zoom = c_zoom.c_default;

	//
	// コンテンツウィンドウです。
	//
	HWND window = nullptr;

	//
	// 描画に使用するテーマハンドルです。
	//
	my::theme::unique_ptr<> theme;

	//
	// 現在の画像のインデックスを設定して正規化します。
	//
	void set_current_bitmap_index(int new_current_bitmap_index)
	{
		current_bitmap_index = new_current_bitmap_index;
		current_bitmap_index = std::max(current_bitmap_index, 0);
		current_bitmap_index = std::min(current_bitmap_index, (int)bitmaps.size() - 1);
	}

	//
	// 拡大率を設定して正規化します。
	//
	void set_zoom(int new_zoom)
	{
		zoom = new_zoom;
		zoom = std::max(zoom, c_zoom.c_min);
		zoom = std::min(zoom, c_zoom.c_max);
	}

	//
	// スクロールバーを更新します。
	//
	BOOL update_scrollbar(HWND hwnd)
	{
		if (bitmaps.empty()) return FALSE;

		// クライアント領域のサイズを取得します。
		RECT rc; ::GetClientRect(hwnd, &rc);
		auto w = rc.right - rc.left;
		auto h = rc.bottom - rc.top;

		// 画像を取得します。
		auto bitmap = bitmaps[current_bitmap_index];

		// 画像のサイズを取得します。
		BITMAP bm = {}; ::GetObject(bitmap.get(), sizeof(bm), &bm);
		auto bm_w = ::MulDiv(bm.bmWidth, zoom, 100);
		auto bm_h = ::MulDiv(bm.bmHeight, zoom, 100);

		// 現在のスクロール位置を取得します。
		auto horz_pos = ::GetScrollPos(hwnd, SB_HORZ);
		auto vert_pos = ::GetScrollPos(hwnd, SB_VERT);

		// スクロールバーを更新します。
		SCROLLINFO si = { sizeof(si) };
		si.fMask = SIF_DISABLENOSCROLL | SIF_ALL;
		si.nMin = 0;
		si.nMax = bm_w;
		si.nPage = w + 1;
		si.nPos = horz_pos;
		::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
		si.nMin = 0;
		si.nMax = bm_h;
		si.nPage = h + 1;
		si.nPos = vert_pos;
		::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

		return TRUE;
	}

	//
	// コンテンツを更新します。
	//
	BOOL update(HWND hwnd)
	{
		// コンテンツウィンドウを再描画します。
		::InvalidateRect(hwnd, nullptr, FALSE);

		if (bitmaps.empty())
		{
			// 親ウィンドウのウィンドウ名を更新します。
			::SetWindowTextA(::GetParent(hwnd), std::format("{} - {}% - (0/0)", plugin_name, zoom).c_str());

			return FALSE;
		}

		// 親ウィンドウのウィンドウ名を更新します。
		::SetWindowTextA(::GetParent(hwnd), std::format("{} - {}% - ({}/{})",
			plugin_name, zoom, current_bitmap_index + 1, bitmaps.size()).c_str());

		update_scrollbar(hwnd);

		return TRUE;
	}

	//
	// コンテンツウィンドウのWM_PAINTを処理します。
	//
	BOOL on_paint(HWND hwnd)
	{
		// クライアント領域のサイズを取得します。
		RECT rc; ::GetClientRect(hwnd, &rc);
		auto w = rc.right - rc.left;
		auto h = rc.bottom - rc.top;

		my::PaintDC pdc(hwnd);
		my::UxDC dc(pdc, &rc);

		// 背景を塗りつぶします。
		::DrawThemeBackground(theme.get(), dc, WP_DIALOG, 0, &rc, nullptr);

		// 画像が存在しない場合はここで処理を終了します。
		if (bitmaps.empty()) return FALSE;

		// 画像を取得します。
		auto bitmap = bitmaps[current_bitmap_index];

		// 画像のサイズを取得します。
		BITMAP bm = {}; ::GetObject(bitmap.get(), sizeof(bm), &bm);
		auto bm_w = ::MulDiv(bm.bmWidth, zoom, 100);
		auto bm_h = ::MulDiv(bm.bmHeight, zoom, 100);

		// 現在のスクロール位置を取得します。
		auto horz_pos = ::GetScrollPos(hwnd, SB_HORZ);
		auto vert_pos = ::GetScrollPos(hwnd, SB_VERT);

		{
			my::dc::unique_ptr<> mdc(::CreateCompatibleDC(dc));
			my::gdi::selector bitmap_selector(mdc.get(), bitmap.get());

			::SetStretchBltMode(dc, HALFTONE);
			::StretchBlt(dc, -horz_pos, -vert_pos, bm_w, bm_h,
				mdc.get(), 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		}

		return TRUE;
	}

	//
	// コンテンツウィンドウのWM_CONTEXTMENUを処理します。
	//
	BOOL on_context_menu(HWND hwnd)
	{
		my::menu::unique_ptr<> menu(::CreatePopupMenu());
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_backward, L"戻る");
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_forward, L"進む");
		::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_zoom_in, L"拡大");
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_zoom_out, L"縮小");
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_zoom_reset, L"拡大率をリセット");
		::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_erase_current_bitmap, L"現在の画像を削除");
		::AppendMenuW(menu.get(), MF_STRING, c_command_id.c_clear_all_bitmaps, L"すべての画像を削除");

		if (current_bitmap_index <= 0)
			::EnableMenuItem(menu.get(), c_command_id.c_backward, MF_DISABLED);

		if (current_bitmap_index >= (int)bitmaps.size() - 1)
			::EnableMenuItem(menu.get(), c_command_id.c_forward, MF_DISABLED);

		if (zoom <= c_zoom.c_min)
			::EnableMenuItem(menu.get(), c_command_id.c_zoom_out, MF_DISABLED);

		if (zoom >= c_zoom.c_max)
			::EnableMenuItem(menu.get(), c_command_id.c_zoom_in, MF_DISABLED);

		if (zoom >= c_zoom.c_default)
			::EnableMenuItem(menu.get(), c_command_id.c_zoom_reset, MF_DISABLED);

		if (bitmaps.empty())
		{
			::EnableMenuItem(menu.get(), c_command_id.c_erase_current_bitmap, MF_DISABLED);
			::EnableMenuItem(menu.get(), c_command_id.c_clear_all_bitmaps, MF_DISABLED);
		}

		POINT point; ::GetCursorPos(&point);
		auto id = (UINT)::TrackPopupMenuEx(menu.get(),
			TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, hwnd, nullptr);
		if (!id) return FALSE;

		switch (id)
		{
		case c_command_id.c_erase_current_bitmap:
			{
				// 現在の画像をコレクションから削除します。
				if (current_bitmap_index < (int)bitmaps.size())
					bitmaps.erase(bitmaps.begin() + current_bitmap_index);
				set_current_bitmap_index(current_bitmap_index);

				break;
			}
		case c_command_id.c_clear_all_bitmaps:
			{
				// コレクション内のすべての画像を消去します。
				bitmaps.clear();
				set_current_bitmap_index(0);

				break;
			}
		case c_command_id.c_zoom_in:
			{
				// 拡大率を倍にします。
				set_zoom(zoom * 2);

				break;
			}
		case c_command_id.c_zoom_out:
			{
				// 拡大率を半分にします。
				set_zoom(zoom / 2);

				break;
			}
		case c_command_id.c_zoom_reset:
			{
				// 拡大率をリセットします。
				set_zoom(c_zoom.c_default);

				break;
			}
		case c_command_id.c_backward:
			{
				set_current_bitmap_index(current_bitmap_index - 1);

				break;
			}
		case c_command_id.c_forward:
			{
				set_current_bitmap_index(current_bitmap_index + 1);

				break;
			}
		}

		update(hwnd);

		return TRUE;
	}

	//
	// コンテンツウィンドウのWM_HSCROLL、WM_VSCROLLを処理します。
	//
	BOOL on_scroll(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		auto sb = (message == WM_HSCROLL) ? SB_HORZ : SB_VERT;

		SCROLLINFO si = { sizeof(si) };
		si.fMask = SIF_DISABLENOSCROLL | SIF_ALL;
		::GetScrollInfo(hwnd, sb, &si);
				
		switch (LOWORD(wparam))
		{
		case SB_LEFT:
			{
				si.nPos = si.nMin;

				break;
			}
		case SB_RIGHT:
			{
				si.nPos = si.nMax;

				break;
			}
		case SB_LINELEFT:
			{
				si.nPos--;

				break;
			}
		case SB_LINERIGHT:
			{
				si.nPos++;

				break;
			}
		case SB_PAGELEFT:
			{
				si.nPos -= 100;

				break;
			}
		case SB_PAGERIGHT:
			{
				si.nPos += 100;

				break;
			}
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			{
				si.nPos = HIWORD(wparam);

				break;
			}
		}

		::SetScrollPos(hwnd, sb, si.nPos, TRUE);
		::InvalidateRect(hwnd, nullptr, FALSE);

		return TRUE;
	}

	//
	// コンテンツウィンドウのWM_CLIPBOARDUPDATEを処理します。
	//
	BOOL on_clipboard_update(HWND hwnd)
	{
		// クリップボードを開きます。
		if (!::OpenClipboard(hwnd)) return FALSE;

		// クリップボードの画像を取得し、複製します。
		auto bitmap = (HBITMAP)::OleDuplicateData(
			::GetClipboardData(CF_BITMAP), CF_BITMAP, 0);

		// クリップボードを閉じます。
		::CloseClipboard(); 

		// 画像が取得できなかった場合は何もしません。
		if (!bitmap) return FALSE;

		// 画像をコレクションに追加します。
		bitmaps.emplace_back(bitmap);

		// 追加した画像をカレントに設定します。
		set_current_bitmap_index((int)bitmaps.size() - 1);

		// コンテンツを更新します。
		update(hwnd);

		return TRUE;
	}

	//
	// コンテンツウィンドウのウィンドウプロシージャです。
	//
	LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_CREATE:
			{
				// テーマを開きます。
				theme.reset(::OpenThemeData(hwnd, VSCLASS_WINDOW));

				// クリップボードの監視を開始します。
				::AddClipboardFormatListener(hwnd);

				break;
			}
		case WM_DESTROY:
			{
				// クリップボードの監視を終了します。
				::RemoveClipboardFormatListener(hwnd);

				// テーマを閉じます。
				theme.reset();

				break;
			}
		case WM_SIZE:
			{
				update_scrollbar(hwnd);

				break;
			}
		case WM_PAINT:
			{
				on_paint(hwnd);

				break;
			}
		case WM_CONTEXTMENU:
			{
				on_context_menu(hwnd);

				break;
			}
		case WM_HSCROLL:
		case WM_VSCROLL:
			{
				on_scroll(hwnd, message, wparam, lparam);

				break;
			}
		case WM_APPCOMMAND:
			{
				switch (GET_APPCOMMAND_LPARAM(lparam))
				{
				case APPCOMMAND_BROWSER_BACKWARD:
					{
						set_current_bitmap_index(current_bitmap_index - 1);

						update(hwnd);

						break;
					}
				case APPCOMMAND_BROWSER_FORWARD:
					{
						set_current_bitmap_index(current_bitmap_index + 1);

						update(hwnd);

						break;
					}
				}

				break;
			}
		case WM_CLIPBOARDUPDATE:
			{
				on_clipboard_update(hwnd);

				break;
			}
		}

		return ::DefWindowProc(hwnd, message, wparam, lparam);
	}

	//
	// コンテンツの初期化処理です。
	//
	BOOL init(AviUtl::FilterPlugin* fp)
	{
		WNDCLASSW wc = {};
		wc.lpfnWndProc =
			[](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
			{ return contents.wnd_proc(hwnd, message, wparam, lparam); };
		wc.hInstance = fp->dll_hinst;
		wc.lpszClassName = L"clipboard_image";
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		::RegisterClass(&wc);

		window = ::CreateWindowExW(
			0,
			wc.lpszClassName,
			wc.lpszClassName,
			WS_VISIBLE | WS_CHILD |
			WS_HSCROLL | WS_VSCROLL |
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 0, 0,
			fp->hwnd, nullptr, fp->dll_hinst, nullptr);

		return window != nullptr;
	}

	//
	// コンテンツの後始末処理です。
	//
	BOOL exit(AviUtl::FilterPlugin* fp)
	{
		::DestroyWindow(window), window = nullptr;

		return TRUE;
	}
} contents;
