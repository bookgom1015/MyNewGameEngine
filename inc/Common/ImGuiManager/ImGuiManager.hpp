#pragma once

#ifdef _DLLEXPORT
	#ifndef ImGuiManagerAPI
	#define ImGuiManagerAPI __declspec(dllexport)
	#endif
#else
	#ifndef ImGuiManagerAPI
	#define ImGuiManagerAPI __declspec(dllimport)
	#endif
#endif

struct ImGuiContext;

namespace Common::Debug {
	struct LogFile;
}

namespace Common::Foundation::Core {
	class WindowsManager;
}

namespace Common {
	namespace Foundation {
		struct Light;
	}

	namespace Render::ShadingArgument {
		struct ShadingArgumentSet;
	}
}

namespace Common::ImGuiManager {
	class ImGuiManager {
	public:
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

	public:
		ImGuiManagerAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd);
		ImGuiManagerAPI virtual void CleanUp();

		ImGuiManagerAPI void HookMsgCallback(
			Common::Foundation::Core::WindowsManager* const pWndManager);

	protected:
		ImGuiManagerAPI void FrameRateText(UINT clientWidth, UINT clientHeight);
		ImGuiManagerAPI void RaytraycingEnableCheckBox(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		ImGuiManagerAPI void LightHeader(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
			Common::Foundation::Light* lights[],
			UINT numLights,
			std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights);
		ImGuiManagerAPI void ShadingObjectHeader(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);

	private:
		void ShadowTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void GammaCorrectionTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void ToneMappingTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void TAATree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void AOTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void VolumetricLightTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void SSCSTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void MotionBlurTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void BloomTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void DOFTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		void ChromaticAberrationTree(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);

	protected:
		BOOL mbIsWin32Initialized{};

		Common::Debug::LogFile* mpLogFile{};

		ImGuiContext* mpContext{};

		FLOAT mFrameTimes[3000]{};
		UINT mFrameOffset{};
	};
}