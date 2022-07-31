#include "pch-il2cpp.h"
#include "includes.h"
#include "kiero/minhook/include/MinHook.h"
#include <iostream>

using namespace app;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

// Set the name of your log file here
extern const LPCWSTR LOG_FILE = L"il2cpp-log.txt";

bool isRunning = true;
bool isShowMenu = false;

bool godMode = false;
bool autoGuard = false;
bool infiniteFever = false;
bool infiniteJump = false;
bool infiniteDash = false;
float moveSpeed = 1;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.MouseDrawCursor = true;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (isShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (isShowMenu) {
		ImGui::Begin("ImGui Window");

		ImGui::Checkbox("God Mode", &godMode);
		ImGui::Checkbox("Auto Guard", &autoGuard);
		ImGui::Checkbox("Infinite Fever", &infiniteFever);
		ImGui::Checkbox("Infinite Jump", &infiniteJump);
		ImGui::Checkbox("Infinite Dash", &infiniteDash);
		ImGui::SliderFloat("Speed", &moveSpeed, 1, 5);

		ImGui::End();
	}

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI CheatMenuThread(LPVOID lpReserved)
{
	while (isRunning)
	{
		Sleep(10);

		if (GetAsyncKeyState(VK_END) & 1) {
			isRunning = false;
		}

		if (GetAsyncKeyState(VK_HOME) & 1) {
			isShowMenu = !isShowMenu;
		}
	}

	kiero::shutdown();

	return TRUE;
}

// GameManager_CreatePlayer hook
typedef void(__stdcall* oGameManagerCreatePlayer)(GameManager*, MethodInfo*);
static oGameManagerCreatePlayer oGameManager_CreatePlayer = NULL;
GameManager* gameManager = nullptr;
void hkGameManager_CreatePlayer(GameManager* __this, MethodInfo* method) {
	if (__this) {
		gameManager = __this;
		std::cout << "Found GameManager: " << std::hex << __this << std::endl;
	}
	else {
		gameManager = nullptr;
	}
	return oGameManager_CreatePlayer(__this, method);
}

// CharacterBehaviour_IsDash hook
typedef bool(__stdcall* oCharacterBehaviourIsDash)(CharacterBehaviour*, MethodInfo*);
static oCharacterBehaviourIsDash oCharacterBehaviour_IsDash = NULL;
bool hkCharacterBehaviour_IsDash(CharacterBehaviour* __this, MethodInfo* method) {
	if (infiniteDash) return false;
	return oCharacterBehaviour_IsDash(__this, method);
}

DWORD WINAPI HookThread(LPVOID lpReserved)
{
	MH_Initialize();

	MH_CreateHook(GameManager_CreatePlayer, &hkGameManager_CreatePlayer, (void**)&oGameManager_CreatePlayer);
	MH_CreateHook(CharacterBehaviour_IsDash, &hkCharacterBehaviour_IsDash, (void**)&oCharacterBehaviour_IsDash);

	MH_EnableHook(MH_ALL_HOOKS);

	while (isRunning) Sleep(1000);

	MH_DisableHook(MH_ALL_HOOKS);

	MH_Uninitialize();

	return TRUE;
}

DWORD WINAPI CheatThread(LPVOID lpReserved)
{
	while (isRunning)
	{
		Sleep(10);
		if (gameManager == nullptr) continue;

		PlayerController* playerController = gameManager->fields.mPlayerCtrl;
		if (playerController == nullptr) continue;

		PlayerCharaData* playerCharaData = gameManager->fields.mPlayerCharaData;
		if (playerCharaData == nullptr) continue;

		BattleCharaData* battleCharaData = playerController->fields.Character;
		if (battleCharaData == nullptr) continue;

		CharacterBehaviour* characterBehaviour = battleCharaData->fields.Character;
		if (characterBehaviour == nullptr) continue;

		// godMode
		BattleCharaData_SetNoHit(battleCharaData, godMode, nullptr);

		// autoGuard
		BattleCharaData_SetAutoGurad(battleCharaData, autoGuard, nullptr);

		// infiniteFever
		if (infiniteFever) {
			playerCharaData->fields.FeverGauge = playerCharaData->fields.FeverGaugeMax;
		}

		// infiniteJump
		if (infiniteJump) {
			characterBehaviour->fields.mAirJumpCnt = 0;
		}

		// moveSpeed
		CharacterBehaviour_SetUserMoveSpeed(characterBehaviour, moveSpeed, nullptr);

		/*system("cls");
		std::cout << "GetUserMoveSpeed: " << CharacterBehaviour_GetUserMoveSpeed(characterBehaviour, nullptr) << std::endl;*/
	}

	return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);

		init_il2cpp();
		il2cpp_thread_attach(il2cpp_domain_get());

		// If you would like to write to a log file, specify the name above and use il2cppi_log_write()
		// il2cppi_log_write("Startup");

		// If you would like to output to a new console window, use il2cppi_new_console() to open one and redirect stdout
		il2cppi_new_console();

		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);

		CreateThread(nullptr, 0, HookThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, CheatMenuThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, CheatThread, hMod, 0, nullptr);

		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}