#include "nyads.h"
#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <iostream>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace Microsoft::WRL;

HINSTANCE hInst;
nyads ds;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static wil::com_ptr<ICoreWebView2Controller> webviewController;
static wil::com_ptr<ICoreWebView2> webviewWindow;
static std::wstring startPage = std::filesystem::current_path().wstring() + L"\\start.html";
static std::wstring corePage = std::filesystem::current_path().wstring() + L"\\core.html";

std::wstring Str2Wstr(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
#endif

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"nyads";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex)) return 1;

	hInst = hInstance;

	int x = (GetSystemMetrics(SM_CXSCREEN) - 600) / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) - 600) / 2;

	HWND hWnd = CreateWindow(
		L"nyads",
		L"NyaDS",
		WS_POPUP | WS_SYSMENU | WS_OVERLAPPEDWINDOW,
		x,
		y,
		600, 
		600,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd) return 1;

	SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101)));

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");

	CreateCoreWebView2EnvironmentWithOptions(nullptr, L"c:/windows/temp", options.Get(),
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewController = controller;
							webviewController->get_CoreWebView2(&webviewWindow);
						}

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(hWnd, &bounds);
						webviewController->put_Bounds(bounds);

						webviewWindow->Navigate(startPage.data());

						EventRegistrationToken token;
						webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								PWSTR message;
								args->TryGetWebMessageAsString(&message);
								std::wstring wstr(message);

								json data = json::parse(std::string(wstr.begin(), wstr.end()));
								json res = {};

								std::string event = data["event"];

								if (event == "connect") {
									if (ds.connect(data["data"]["address"].get<std::string>(), data["data"]["port"].get<int>())) {
										res = {
											{"event", "connection"},
											{"data", {{"success", true}}}
										};

										webview->Navigate(corePage.data());
									}
									else {
										res = {
											{"event", "connection"},
											{"data", {{"success", false }}}
										};
									}
								}

								if (event == "update") {
									res = {};

									ds.set_alliance(data["data"]["alliance"].get<std::string>());
									ds.set_mode(data["data"]["mode"].get<std::string>());
									ds.update();
								}

								// All auto commands
								if (event == "page") {
									if (data["data"]["page"] == "autoCreate") {
										webview->AddScriptToExecuteOnDocumentCreated(Str2Wstr("window.socket = new WebSocket('ws://" + (ds.url) + ":9072');").data(), nullptr);
									}
									webview->Navigate((std::filesystem::current_path().wstring() + L"\\" + Str2Wstr(data["data"]["page"]) + L".html").data());
								}

								if (event.starts_with("auto_")) {
								}

								std::string str = res.dump();
								std::wstring wres(str.begin(), str.end());

								webview->PostWebMessageAsJson(wres.data());

								CoTaskMemFree(message);
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webviewController->put_Bounds(bounds);
		};
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}