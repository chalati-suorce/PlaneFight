#include "embedded_assets.h"

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>

#include "resource.h"

static bool ResolveAssetResourceId(const char* path, int& resourceId) {
    if (path == nullptr) {
        return false;
    }

    if (std::strcmp(path, "laserBlue.png") == 0) {
        resourceId = IDR_ASSET_LASER_BLUE;
        return true;
    }
    if (std::strcmp(path, "laserRed.png") == 0) {
        resourceId = IDR_ASSET_LASER_RED;
        return true;
    }
    if (std::strcmp(path, "playerShip_blue.png") == 0) {
        resourceId = IDR_ASSET_PLAYER_BLUE;
        return true;
    }
    if (std::strcmp(path, "playerShip_red.png") == 0) {
        resourceId = IDR_ASSET_PLAYER_RED;
        return true;
    }
    if (std::strcmp(path, "purple.png") == 0) {
        resourceId = IDR_ASSET_PURPLE_BG;
        return true;
    }

    return false;
}
#endif

bool GetEmbeddedAssetData(const char* path, const unsigned char*& outData, int& outSize) {
    outData = nullptr;
    outSize = 0;

#if defined(_WIN32)
    int resourceId = 0;
    if (!ResolveAssetResourceId(path, resourceId)) {
        return false;
    }

    HRSRC resInfo = FindResourceA(nullptr, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
    if (resInfo == nullptr) {
        return false;
    }

    HGLOBAL resData = LoadResource(nullptr, resInfo);
    if (resData == nullptr) {
        return false;
    }

    DWORD size = SizeofResource(nullptr, resInfo);
    if (size == 0) {
        return false;
    }

    void* ptr = LockResource(resData);
    if (ptr == nullptr) {
        return false;
    }

    outData = static_cast<const unsigned char*>(ptr);
    outSize = static_cast<int>(size);
    return true;
#else
    (void)path;
    return false;
#endif
}
