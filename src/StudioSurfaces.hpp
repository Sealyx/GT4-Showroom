#pragma once

#include "PreviewOcclusion.hpp"
#include "RegionalSurfaces.hpp"

namespace gt4
{
    class StudioSurfaces
    {
        RegionalSurfaces regional;
        IDirect3DPixelShader9* shader = nullptr;
        bool attempted = false;

    public:
        StudioSurfaces() = default;
        StudioSurfaces(const StudioSurfaces&) = delete;
        StudioSurfaces& operator=(const StudioSurfaces&) = delete;
        ~StudioSurfaces() { reset(); }

        void reset()
        {
            regional.reset();
            if (shader) { shader->Release(); }
            shader = nullptr;
            attempted = false;
        }

        bool available(IDirect3DDevice9* device)
        {
            if (attempted) { return shader != nullptr; }
            attempted = true;
            D3DCAPS9 caps{};
            if (FAILED(device->GetDeviceCaps(&caps)) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
                return false;
            }
            HMODULE library = loadPreviewShaderCompiler();
            if (!library) { return false; }
            const auto compile = reinterpret_cast<CompilePreviewShader>(GetProcAddress(library, "D3DCompile"));
            if (compile) {
                static constexpr char source[] = R"hlsl(
float4 surface : register(c0);  // style, cove shift, lamp phase, opacity
float4 view : register(c1);     // pixels tall, aspect ratio, wall texture

float hash(float2 p)
{
    float3 q = frac(float3(p.xyx) * float3(.1031,.11369,.13787));
    q += dot(q, q.yzx + 19.19);
    return frac((q.x + q.y) * q.z);
}
float field(float2 p)
{
    float2 i=floor(p), f=frac(p);
    f=f*f*(3-2*f);
    return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
                lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
}
float oval(float2 p, float2 center, float2 radii)
{
    float2 q=(p-center)/radii;
    return exp2(-dot(q,q)*1.442695);
}
float panels(float2 p, float spacing, float seed)
{
    float index=floor(p.x*spacing+seed);
    float a=hash(float2(index,seed));
    float b=hash(float2(index,seed+5));
    float x=frac(p.x*spacing+seed);
    float side=smoothstep(.035,.105,x)*(1-smoothstep(.89,.98,x));
    float start=-.10+.34*b;
    float end=start+.19+.30*a;
    float height=smoothstep(start,start+.065,p.y)*(1-smoothstep(end,end+.09,p.y));
    return (a-.52)*side*height;
}
float4 main(float2 uv:TEXCOORD0,float2 pixel:VPOS):COLOR0
{
    float2 p=float2((uv.x-.5)*view.y,uv.y-surface.y);
    float cove=smoothstep(.47,.67,p.y);
    float3 wall, floorTint;
    float hue=surface.x;
    float lamp=surface.z;
    [branch] if (hue < -.5)
    {
        // Keep the wall fixed to the screen and place the cove and floor lights at ground level
        float2 wallPoint=float2(p.x,uv.y);
        float wallLight=30-5*smoothstep(.20,.86,uv.y);
        wallLight+=105*oval(wallPoint,float2(.005,-.075),float2(.425,.50));
        wallLight+=17*oval(wallPoint,float2(.49,.39),float2(.57,.34));
        wallLight-=6*oval(wallPoint,float2(-.65,.46),float2(.33,.31));
        float2 cloud=wallPoint*float2(8.3,10.8)+float2(7.9,19.1);
        float2 warp=float2(field(cloud*.48),field(cloud*.45+13.7))-.5;
        float plaster=(field(cloud+warp*1.55)-.5)*8.7;
        plaster+=(field(cloud*2.45-warp*.9)-.5)*3.5;
        plaster+=(field(cloud*5.3+4.1)-.5)*1.25;
        wallLight+=plaster*view.z*(.55+.45*oval(wallPoint,float2(.08,.32),float2(.59,.44)));
        float floorLight=24+26*oval(p,float2(-.04,.575),float2(.99,.111));
        floorLight+=68*oval(p,float2(-.027,.578),float2(.255,.037));
        floorLight+=12*oval(p,float2(.075,.567),float2(.54,.069));
        cove=smoothstep(.435,.515,p.y);
        wall=wallLight;
        floorTint=floorLight;
    }
    else if (hue < .5)
    {
        wall=float3(21,27,31)+float3(77,83,88)*oval(p,float2(-.10,.43),float2(.71,.32));
        wall+=float3(12,16,20)*oval(p,float2(.47,.29),float2(.54,.20));
        floorTint=float3(31,38,44)+float3(112,115,119)*oval(p,float2(-.035,.65),float2(.82,.23));
    }
    else if (hue < 1.5)
    {
        wall=float3(13,16,18)+float3(68,70,72)*oval(p,float2(-.16+lamp,.47),float2(.69,.29));
        wall+=float3(21,21,23)*oval(p,float2(.54,.18),float2(.68,.20));
        float across=p.y+.61*p.x;
        float along=p.x-.61*p.y;
        float grooving=field(float2(across*186,along*4.1))-.5;
        float filament=(field(float2(across*468,along*8))-.5)*saturate(view.x/1440);
        float lobe=oval(p,float2(.16,.37),float2(1.04,.39));
        wall+=(grooving*8+filament*4+3*(field(float2(across*32,along*2.5))-.5))*lobe*view.z;
        floorTint=float3(22,24,28)+float3(117,119,122)*oval(p,float2(.02+lamp,.64),float2(.80,.19));
    }
    else if (hue < 2.5)
    {
        wall=float3(10,13,19)+float3(43,47,57)*oval(p,float2(.13+lamp,.43),float2(.70,.28));
        wall+=float3(18,18,18)*oval(p,float2(-.40,.37),float2(.62,.29));
        float mist=field(p*float2(9,10)+2*field(p*3.9))-.5;
        mist+=.29*(field(p*float2(28,24))-.5);
        wall+=mist*10*oval(p,float2(.03,.39),float2(.92,.44))*view.z;
        floorTint=float3(17,22,30)+float3(109,112,124)*oval(p,float2(.07+lamp,.64),float2(.88,.19));
    }
    else
    {
        wall=float3(20,19,18)+float3(80,73,66)*oval(p,float2(-.08+lamp,.45),float2(.80,.33));
        wall+=float3(17,15,12)*oval(p,float2(.42,.34),float2(.51,.29));
        float tiles=panels(p,14.2,12.9)*11+panels(p+float2(.02,.12),24.3,2.6)*6;
        tiles+=(field(p*float2(59,6))-.5)*4;
        wall+=tiles*float3(1,.88,.72)*oval(p,float2(0,.30),float2(1.03,.46))*view.z;
        floorTint=float3(27,25,23)+float3(122,115,107)*oval(p,float2(-.055,.64),float2(.84,.21));
    }
    // Stationary sub-byte ordered dither. It does not shimmer during rotation
    float2 lo=frac(floor(pixel)*.5)*2;
    float2 hi=frac(floor(pixel*.5)*.5)*2;
    float index=(1-abs(lo.x-lo.y))*8+lo.y*4+(1-abs(hi.x-hi.y))*2+hi.y;
    float3 value=floor(clamp(lerp(wall,floorTint,cove),0,255)+(index+.5)/16);
    return float4(value/255,surface.w);
}
)hlsl";
                ID3DBlob* bytecode = nullptr;
                if (SUCCEEDED(compile(source, sizeof(source) - 1, "StudioSurfaces", nullptr, nullptr, "main",
                                      "ps_3_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &bytecode, nullptr)) && bytecode) {
                    device->CreatePixelShader(static_cast<const DWORD*>(bytecode->GetBufferPointer()), &shader);
                }
                if (bytecode) { bytecode->Release(); }
            }
            FreeLibrary(library);
            return shader != nullptr;
        }

        bool draw(IDirect3DDevice9* device, float width, float height, int style, float floorShift,
                  double seconds, float opacity, bool wallTexture)
        {
            if (style >= 4) { return regional.draw(device,width,height,style-4,floorShift,opacity,wallTexture); }
            if (!available(device)) { return false; }
            const float values[]{float(style), floorShift, .026f * float(std::sin(seconds * .19)), opacity,
                                 height, width / height, wallTexture ? 1.f : 0.f, 0.f};
            device->SetPixelShaderConstantF(0, values, 2);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, opacity < 1.f);
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            struct Vertex { float x, y, z, rhw, u, v; };
            const Vertex vertices[]{
                {-.5f, -.5f, 1, 1, 0, 0}, {width - .5f, -.5f, 1, 1, 1, 0},
                {-.5f, height - .5f, 1, 1, 0, 1}, {width - .5f, height - .5f, 1, 1, 1, 1}
            };
            const bool drawn = SUCCEEDED(device->SetPixelShader(shader)) &&
                SUCCEEDED(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1)) &&
                SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex)));
            device->SetPixelShader(nullptr);
            return drawn;
        }
    };
}


