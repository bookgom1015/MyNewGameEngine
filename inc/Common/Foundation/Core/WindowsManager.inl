#ifndef __WINDOWSMANAGER_INL__
#define __WINDOWSMANAGER_INL__

constexpr BOOL Common::Foundation::Core::WindowsManager::AppPaused() const { return mbAppPaused; }

constexpr BOOL Common::Foundation::Core::WindowsManager::Minimized() const { return mbMinimized; }

constexpr BOOL Common::Foundation::Core::WindowsManager::Maximized() const { return mbMaximized; }

constexpr BOOL Common::Foundation::Core::WindowsManager::Resizing() const { return mbResizing; }

constexpr BOOL Common::Foundation::Core::WindowsManager::Fullscreen() const { return mbFullscreenState; }

constexpr BOOL Common::Foundation::Core::WindowsManager::Destroyed() const { return mbDestroyed; }

constexpr HINSTANCE Common::Foundation::Core::WindowsManager::InstanceHandle() const { return mhInst; }

constexpr HWND Common::Foundation::Core::WindowsManager::MainWindowHandle() const { return mhMainWnd; }

#endif // __WINDOWSMANAGER_INL__