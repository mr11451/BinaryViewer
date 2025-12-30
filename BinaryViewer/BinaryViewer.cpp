// BinaryViewer.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "BinaryViewer.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <commdlg.h>

#define MAX_LOADSTRING 100
#define BYTES_PER_LINE 16

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// バイナリデータ
std::vector<BYTE> g_binaryData;
std::vector<bool> g_byteInitialized;  // 各バイトが初期化済みかを追跡
HFONT g_hFont = nullptr;
int g_charWidth = 0;
int g_charHeight = 0;
int g_scrollPos = 0;
int g_editPos = 0;  // 編集中の位置
bool g_editHighNibble = true;  // 上位4ビット編集中か
COLORREF g_cursorColor = RGB(255, 255, 0);  // カーソル色（デフォルト: 黄色）
int g_fontSize = 16;  // フォントサイズ（デフォルト: 16）
HWND g_hMainWnd = nullptr;  // メインウィンドウハンドル

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void OpenFile(HWND hWnd);
void SaveFile(HWND hWnd);
void UpdateScrollBar(HWND hWnd);
void DrawBinaryView(HDC hdc, RECT* pRect, HWND hWnd);
void ChooseCursorColor(HWND hWnd);
void EnsureCursorVisible(HWND hWnd);
void ChangeFontSize(HWND hWnd);
void RecreateFont(HWND hWnd);
void AdjustWindowSizeToFont(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BINARYVIEWER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BINARYVIEWER));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_DBLCLKS;  // ちらつき防止: CS_HREDRAW | CS_VREDRAWを削除
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BINARYVIEWER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = nullptr;  // WM_ERASEBKGNDでちらつき防止
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_BINARYVIEWER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VSCROLL,
      CW_USEDEFAULT, 0, 900, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   g_hMainWnd = hWnd;  // グローバル変数に保存

   // 等幅フォントを作成
   g_hFont = CreateFontW(g_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
       DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

   // フォントメトリクスを初期化
   HDC hdc = GetDC(hWnd);
   HFONT oldFont = (HFONT)SelectObject(hdc, g_hFont);
   TEXTMETRICW tm;
   GetTextMetricsW(hdc, &tm);
   g_charWidth = tm.tmAveCharWidth;
   g_charHeight = tm.tmHeight;
   SelectObject(hdc, oldFont);
   ReleaseDC(hWnd, hdc);

   // 初期状態は空（g_binaryDataは空のまま）

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_OPEN:
                OpenFile(hWnd);
                break;
            case IDM_SAVE:
                SaveFile(hWnd);
                break;
            case IDM_CURSOR_COLOR:
                ChooseCursorColor(hWnd);
                break;
            case IDM_FONT_SIZE:
                ChangeFontSize(hWnd);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_ERASEBKGND:
        // 背景消去をスキップしてちらつきを防止
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            // ダブルバッファリング用のメモリDCを作成
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            
            // メモリDCに描画
            DrawBinaryView(memDC, &clientRect, hWnd);
            
            // メモリDCから画面DCへ一度に転送
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);
            
            // クリーンアップ
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SIZE:
        UpdateScrollBar(hWnd);
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    case WM_VSCROLL:
        {
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
            GetScrollInfo(hWnd, SB_VERT, &si);
            int oldPos = si.nPos;

            switch (LOWORD(wParam))
            {
            case SB_LINEUP: si.nPos -= 1; break;
            case SB_LINEDOWN: si.nPos += 1; break;
            case SB_PAGEUP: si.nPos -= si.nPage; break;
            case SB_PAGEDOWN: si.nPos += si.nPage; break;
            case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
            }

            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            GetScrollInfo(hWnd, SB_VERT, &si);

            if (si.nPos != oldPos)
            {
                g_scrollPos = si.nPos;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (g_binaryData.empty())
                break;  // 空の場合はキー操作をスキップ

            bool needRedraw = false;
            switch (wParam)
            {
            case VK_LEFT:
                if (g_editPos > 0 || !g_editHighNibble)
                {
                    if (g_editHighNibble)
                    {
                        if (g_editPos > 0)
                        {
                            g_editPos--;
                            g_editHighNibble = false;
                            needRedraw = true;
                        }
                    }
                    else
                    {
                        g_editHighNibble = true;
                        needRedraw = true;
                    }
                }
                break;
            case VK_RIGHT:
                if (g_editPos < (int)g_binaryData.size())
                {
                    if (!g_editHighNibble)
                    {
                        if (g_editPos < (int)g_binaryData.size() - 1)
                        {
                            g_editPos++;
                            g_editHighNibble = true;
                            needRedraw = true;
                        }
                    }
                    else
                    {
                        g_editHighNibble = false;
                        needRedraw = true;
                    }
                }
                break;
            case VK_UP:
                if (g_editPos >= BYTES_PER_LINE)
                {
                    g_editPos -= BYTES_PER_LINE;
                    needRedraw = true;
                }
                break;
            case VK_DOWN:
                if (g_editPos + BYTES_PER_LINE < (int)g_binaryData.size())
                {
                    g_editPos += BYTES_PER_LINE;
                    needRedraw = true;
                }
                break;
            case VK_PRIOR:  // PageUp
                {
                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    int visibleLines = rect.bottom / g_charHeight;
                    int moveLines = visibleLines > 1 ? visibleLines - 1 : 1;
                    int newPos = g_editPos - (moveLines * BYTES_PER_LINE);
                    if (newPos < 0) newPos = 0;
                    g_editPos = newPos;
                    needRedraw = true;
                }
                break;
            case VK_NEXT:  // PageDown
                {
                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    int visibleLines = rect.bottom / g_charHeight;
                    int moveLines = visibleLines > 1 ? visibleLines - 1 : 1;
                    int newPos = g_editPos + (moveLines * BYTES_PER_LINE);
                    if (newPos >= (int)g_binaryData.size())
                        newPos = (int)g_binaryData.size() - 1;
                    g_editPos = newPos;
                    needRedraw = true;
                }
                break;
            case VK_HOME:
                if (GetKeyState(VK_CONTROL) & 0x8000)
                {
                    // Ctrl+Home: 先頭へ
                    g_editPos = 0;
                }
                else
                {
                    // Home: 行の先頭へ
                    g_editPos = (g_editPos / BYTES_PER_LINE) * BYTES_PER_LINE;
                }
                g_editHighNibble = true;
                needRedraw = true;
                break;
            case VK_END:
                if (GetKeyState(VK_CONTROL) & 0x8000)
                {
                    // Ctrl+End: 最後へ
                    if (!g_binaryData.empty())
                        g_editPos = (int)g_binaryData.size() - 1;
                }
                else
                {
                    // End: 行の最後へ
                    int lineStart = (g_editPos / BYTES_PER_LINE) * BYTES_PER_LINE;
                    int lineEnd = lineStart + BYTES_PER_LINE - 1;
                    if (lineEnd >= (int)g_binaryData.size())
                        lineEnd = (int)g_binaryData.size() - 1;
                    g_editPos = lineEnd;
                }
                g_editHighNibble = true;
                needRedraw = true;
                break;
            case VK_DELETE:
                if (g_editPos < (int)g_binaryData.size())
                {
                    g_binaryData.erase(g_binaryData.begin() + g_editPos);
                    g_byteInitialized.erase(g_byteInitialized.begin() + g_editPos);
                    if (!g_binaryData.empty() && g_editPos >= (int)g_binaryData.size())
                    {
                        g_editPos = (int)g_binaryData.size() - 1;
                    }
                    else if (g_binaryData.empty())
                    {
                        g_editPos = 0;
                    }
                    UpdateScrollBar(hWnd);
                    needRedraw = true;
                }
                break;
            case VK_INSERT:
                if (g_editPos <= (int)g_binaryData.size())
                {
                    g_binaryData.insert(g_binaryData.begin() + g_editPos, 0x00);
                    g_byteInitialized.insert(g_byteInitialized.begin() + g_editPos, false);
                    UpdateScrollBar(hWnd);
                    needRedraw = true;
                }
                break;
            }
            
            if (needRedraw)
            {
                EnsureCursorVisible(hWnd);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        break;
    case WM_CHAR:
        {
            wchar_t ch = (wchar_t)wParam;
            int nibble = -1;

            if (ch >= L'0' && ch <= L'9')
                nibble = ch - L'0';
            else if (ch >= L'A' && ch <= L'F')
                nibble = ch - L'A' + 10;
            else if (ch >= L'a' && ch <= L'f')
                nibble = ch - L'a' + 10;

            if (nibble >= 0)
            {
                // データが空の場合は新しいバイトを追加
                if (g_binaryData.empty())
                {
                    g_binaryData.push_back(0x00);
                    g_byteInitialized.push_back(false);
                    g_editPos = 0;
                    g_editHighNibble = true;
                    UpdateScrollBar(hWnd);
                }

                // 範囲外の場合は新しいバイトを追加
                if (g_editPos >= (int)g_binaryData.size())
                {
                    g_binaryData.push_back(0x00);
                    g_byteInitialized.push_back(false);
                    UpdateScrollBar(hWnd);
                }

                if (g_editHighNibble)
                {
                    g_binaryData[g_editPos] = (nibble << 4);
                    g_byteInitialized[g_editPos] = false;  // まだ下位ニブルが未入力
                    g_editHighNibble = false;
                }
                else
                {
                    g_binaryData[g_editPos] = (g_binaryData[g_editPos] & 0xF0) | nibble;
                    g_byteInitialized[g_editPos] = true;  // 両方のニブル入力完了
                    g_editPos++;
                    g_editHighNibble = true;
                    
                    // 最後のバイトまで到達したら新しいバイトを追加
                    if (g_editPos >= (int)g_binaryData.size())
                    {
                        g_binaryData.push_back(0x00);
                        g_byteInitialized.push_back(false);
                        UpdateScrollBar(hWnd);
                    }
                }
                EnsureCursorVisible(hWnd);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        {
            if (g_binaryData.empty())
                break;

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // フォントサイズを取得
            HDC hdc = GetDC(hWnd);
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFont);
            TEXTMETRICW tm;
            GetTextMetricsW(hdc, &tm);
            g_charWidth = tm.tmAveCharWidth;
            g_charHeight = tm.tmHeight;
            SelectObject(hdc, oldFont);
            ReleaseDC(hWnd, hdc);

            int line = y / g_charHeight + g_scrollPos;
            int hexStart = 10 * g_charWidth;
            int hexEnd = hexStart + 48 * g_charWidth;

            if (x >= hexStart && x < hexEnd)
            {
                int col = (x - hexStart) / (3 * g_charWidth);
                if (col >= BYTES_PER_LINE) col = BYTES_PER_LINE - 1;
                int pos = line * BYTES_PER_LINE + col;
                if (pos < (int)g_binaryData.size())
                {
                    g_editPos = pos;
                    g_editHighNibble = true;
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
            }
        }
        break;
    case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int lines = delta / WHEEL_DELTA * 3;  // 3行ずつスクロール
            
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
            GetScrollInfo(hWnd, SB_VERT, &si);
            
            si.nPos -= lines;
            if (si.nPos < si.nMin) si.nPos = si.nMin;
            if (si.nPos > si.nMax - (int)si.nPage + 1) si.nPos = si.nMax - (int)si.nPage + 1;
            
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            GetScrollInfo(hWnd, SB_VERT, &si);
            
            g_scrollPos = si.nPos;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_DESTROY:
        if (g_hFont)
            DeleteObject(g_hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void ChangeFontSize(HWND hWnd)
{
    // フォントサイズ選択用のポップアップメニュー
    HMENU hMenu = CreatePopupMenu();
    const int sizes[] = { 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72 };
    for (int i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
    {
        wchar_t menuText[32];
        swprintf_s(menuText, L"%d pt%s", sizes[i], sizes[i] == g_fontSize ? L" ✓" : L"");
        AppendMenuW(hMenu, MF_STRING, 1000 + i, menuText);
    }
    
    POINT pt;
    GetCursorPos(&pt);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, 
                             pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
    
    if (cmd >= 1000 && cmd < 1000 + (int)(sizeof(sizes) / sizeof(sizes[0])))
    {
        int newSize = sizes[cmd - 1000];
        
        if (newSize != g_fontSize && newSize >= 8 && newSize <= 72)
        {
            g_fontSize = newSize;
            RecreateFont(hWnd);
            AdjustWindowSizeToFont(hWnd);
            UpdateScrollBar(hWnd);
            EnsureCursorVisible(hWnd);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }
}

void RecreateFont(HWND hWnd)
{
    if (g_hFont)
    {
        DeleteObject(g_hFont);
    }
    
    g_hFont = CreateFontW(g_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    
    // フォントメトリクスを初期化
    HDC hdc = GetDC(hWnd);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFont);
    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);
    g_charWidth = tm.tmAveCharWidth;
    g_charHeight = tm.tmHeight;
    SelectObject(hdc, oldFont);
    ReleaseDC(hWnd, hdc);
}

void AdjustWindowSizeToFont(HWND hWnd)
{
    // 現在のウィンドウ位置を取得
    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    
    // 理想的なクライアント領域のサイズを計算
    // アドレス(10文字) + HEX(16バイト×3文字) + ASCII(16文字) + マージン
    int idealClientWidth = (10 + 48 + 16 + 4) * g_charWidth;
    
    // 表示行数を30行に設定
    int idealClientHeight = 30 * g_charHeight;
    
    // ウィンドウ枠のサイズを考慮
    RECT clientRect = { 0, 0, idealClientWidth, idealClientHeight };
    DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    DWORD exStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    BOOL hasMenu = (GetMenu(hWnd) != nullptr);
    
    AdjustWindowRectEx(&clientRect, style, hasMenu, exStyle);
    
    int newWidth = clientRect.right - clientRect.left;
    int newHeight = clientRect.bottom - clientRect.top;
    
    // 画面サイズを取得して、ウィンドウが画面からはみ出ないようにする
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &mi);
    
    int screenWidth = mi.rcWork.right - mi.rcWork.left;
    int screenHeight = mi.rcWork.bottom - mi.rcWork.top;
    
    // 最大サイズを画面の90%に制限
    if (newWidth > screenWidth * 0.9)
        newWidth = (int)(screenWidth * 0.9);
    if (newHeight > screenHeight * 0.9)
        newHeight = (int)(screenHeight * 0.9);
    
    // ウィンドウの中心位置を維持
    int centerX = (windowRect.left + windowRect.right) / 2;
    int centerY = (windowRect.top + windowRect.bottom) / 2;
    
    int newLeft = centerX - newWidth / 2;
    int newTop = centerY - newHeight / 2;
    
    // 画面内に収める
    if (newLeft < mi.rcWork.left)
        newLeft = mi.rcWork.left;
    if (newTop < mi.rcWork.top)
        newTop = mi.rcWork.top;
    if (newLeft + newWidth > mi.rcWork.right)
        newLeft = mi.rcWork.right - newWidth;
    if (newTop + newHeight > mi.rcWork.bottom)
        newTop = mi.rcWork.bottom - newHeight;
    
    // ウィンドウサイズを変更
    SetWindowPos(hWnd, nullptr, newLeft, newTop, newWidth, newHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void OpenFile(HWND hWnd)
{
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    wchar_t fileName[MAX_PATH] = L"";

    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"すべてのファイル (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
    {
        HANDLE hFile = CreateFileW(fileName, GENERIC_READ, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize = GetFileSize(hFile, nullptr);
            g_binaryData.resize(fileSize);
            g_byteInitialized.resize(fileSize, false);  // 新しいバイナリデータのサイズに合わせてリサイズ

            DWORD bytesRead;
            ReadFile(hFile, g_binaryData.data(), fileSize, &bytesRead, nullptr);
            CloseHandle(hFile);

            // 最後のバイトの下位4ビットを編集可能に設定
            if (!g_binaryData.empty())
            {
                g_editPos = fileSize;  // 編集位置をファイルサイズに設定
                g_editHighNibble = true;  // 上位4ビットを編集中に設定
                g_byteInitialized.push_back(false);  // 新しいバイトの初期化状態を追加
            }

            UpdateScrollBar(hWnd);
            InvalidateRect(hWnd, nullptr, TRUE);
        }
    }
}

void SaveFile(HWND hWnd)
{
    if (g_binaryData.empty())
        return;

    // 未初期化のバイトをチェック
    bool hasUninitializedBytes = false;
    for (size_t i = 0; i < g_byteInitialized.size(); i++)
    {
        if (!g_byteInitialized[i])
        {
            hasUninitializedBytes = true;
            break;
        }
    }

    if (hasUninitializedBytes)
    {
        int result = MessageBoxW(hWnd, 
            L"未初期化のバイトが含まれています。これらは0x00として保存されます。\n続行しますか？",
            L"警告", MB_YESNO | MB_ICONWARNING);
        if (result != IDYES)
            return;
    }

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    wchar_t fileName[MAX_PATH] = L"";

    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"すべてのファイル (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&ofn))
    {
        HANDLE hFile = CreateFileW(fileName, GENERIC_WRITE, 0,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD bytesWritten;
            WriteFile(hFile, g_binaryData.data(), (DWORD)g_binaryData.size(), &bytesWritten, nullptr);
            CloseHandle(hFile);
        }
    }
}

void UpdateScrollBar(HWND hWnd)
{
    if (g_binaryData.empty())
    {
        SetScrollRange(hWnd, SB_VERT, 0, 0, TRUE);
        return;
    }

    // g_charHeightが0の場合は処理をスキップ
    if (g_charHeight == 0)
        return;

    RECT rect;
    GetClientRect(hWnd, &rect);

    int totalLines = ((int)g_binaryData.size() + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
    int visibleLines = rect.bottom / g_charHeight;

    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = totalLines - 1;
    si.nPage = visibleLines;
    si.nPos = g_scrollPos;

    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
}

void DrawBinaryView(HDC hdc, RECT* pRect, HWND hWnd)
{
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFont);

    // 背景を塗りつぶし
    HBRUSH bgBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hdc, pRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(hdc, TRANSPARENT);  // 背景モードを透明に

    int y = 0;
    int startLine = g_scrollPos;
    int visibleLines = (pRect->bottom / g_charHeight);
    int endLine = startLine + visibleLines + 1;
    
    // データが空の場合でも最低1行は表示
    int totalLines = g_binaryData.empty() ? 1 : ((int)g_binaryData.size() + BYTES_PER_LINE - 1) / BYTES_PER_LINE;

    if (endLine > totalLines)
        endLine = totalLines;

    for (int line = startLine; line < endLine; line++)
    {
        std::wstringstream ss;
        
        // アドレス表示（常に表示）
        ss << std::uppercase << std::setfill(L'0') << std::setw(8) << std::hex << (line * BYTES_PER_LINE) << L":  ";
        std::wstring addrStr = ss.str();
        TextOutW(hdc, 0, y, addrStr.c_str(), (int)addrStr.length());

        // データがある場合のみHEX表示とASCII表示
        if (!g_binaryData.empty())
        {
            for (int i = 0; i < BYTES_PER_LINE; i++)
            {
                int pos = line * BYTES_PER_LINE + i;
                if (pos < (int)g_binaryData.size())
                {
                    // 編集中の位置をハイライト
                    if (pos == g_editPos)
                    {
                        RECT highlightRect;
                        highlightRect.left = 10 * g_charWidth + i * 3 * g_charWidth;
                        highlightRect.top = y;
                        highlightRect.right = highlightRect.left + 2 * g_charWidth;
                        highlightRect.bottom = y + g_charHeight;
                        
                        HBRUSH hBrush = CreateSolidBrush(g_cursorColor);
                        FillRect(hdc, &highlightRect, hBrush);
                        DeleteObject(hBrush);
                    }

                    // 上位ニブルだけでも入力されていれば表示
                    bool isInitialized = (pos < (int)g_byteInitialized.size() && g_byteInitialized[pos]);
                    bool isCurrentEditing = (pos == g_editPos && !g_editHighNibble);
                    
                    if (isInitialized || isCurrentEditing)
                    {
                        // HEX値を描画
                        std::wstringstream hexss;
                        hexss << std::uppercase << std::setfill(L'0') << std::setw(2) << std::hex << (int)g_binaryData[pos];
                        std::wstring hexStr = hexss.str();
                        TextOutW(hdc, 10 * g_charWidth + i * 3 * g_charWidth, y, hexStr.c_str(), 2);

                        // ASCII文字を描画（完全に初期化されている場合のみ）
                        if (isInitialized)
                        {
                            wchar_t ch = g_binaryData[pos];
                            wchar_t asciiChar = (ch >= 32 && ch < 127) ? ch : L'.';
                            TextOutW(hdc, 10 * g_charWidth + 48 * g_charWidth + i * g_charWidth, y, &asciiChar, 1);
                        }
                    }
                }
            }
        }

        y += g_charHeight;
    }

    SelectObject(hdc, oldFont);
}

void ChooseCursorColor(HWND hWnd)
{
    static COLORREF customColors[16] = { 0 }; // カスタムカラーの保存用

    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };
    cc.hwndOwner = hWnd;
    cc.rgbResult = RGB(0, 0, 0);  // デフォルトを黒に設定
    cc.lpCustColors = customColors;
    cc.Flags = CC_FULLOPEN;  // CC_RGBINITを削除して現在の色を初期値にしない

    if (ChooseColorW(&cc))
    {
        g_cursorColor = cc.rgbResult;
        InvalidateRect(hWnd, nullptr, FALSE);
    }
}

void EnsureCursorVisible(HWND hWnd)
{
    if (g_binaryData.empty())
        return;

    // g_charHeightが0の場合は処理をスキップ
    if (g_charHeight == 0)
        return;

    RECT rect;
    GetClientRect(hWnd, &rect);

    // カーソル位置の行を計算
    int cursorLine = g_editPos / BYTES_PER_LINE;
    int visibleLines = rect.bottom / g_charHeight;
    
    SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
    GetScrollInfo(hWnd, SB_VERT, &si);

    bool needScroll = false;
    int newScrollPos = g_scrollPos;

    // カーソルが画面上部より上にある場合
    if (cursorLine < g_scrollPos)
    {
        newScrollPos = cursorLine;
        needScroll = true;
    }
    // カーソルが画面下部より下にある場合
    else if (cursorLine >= g_scrollPos + visibleLines)
    {
        newScrollPos = cursorLine - visibleLines + 1;
        needScroll = true;
    }

    if (needScroll)
    {
        // スクロール範囲をチェック
        if (newScrollPos < si.nMin)
            newScrollPos = si.nMin;
        if (newScrollPos > si.nMax - (int)si.nPage + 1)
            newScrollPos = si.nMax - (int)si.nPage + 1;

        g_scrollPos = newScrollPos;
        si.fMask = SIF_POS;
        si.nPos = newScrollPos;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    }
}
