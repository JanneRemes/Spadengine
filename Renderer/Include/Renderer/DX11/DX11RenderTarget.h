#ifdef DIRECTX11

#pragma once

#include <d3d11.h>

#include "Renderer/RenderTarget.h"

namespace sge
{
    struct DX11RenderTarget
    {
        RenderTarget header;

        ID3D11RenderTargetView** views;
    };
}

#endif