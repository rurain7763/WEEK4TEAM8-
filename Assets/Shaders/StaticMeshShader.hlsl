cbuffer StaticMeshObjectConstants : register(b0) 
{
    row_major matrix Model;
    float4 Color;
    int UseVertexColor;
    int HasTexture;
    int padding[2];
}

cbuffer StaticMeshViewConstants : register(b1)
{
    row_major matrix ViewProjection;
}

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

Texture2D main_texture : register(t0);
SamplerState default_sampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    const float4 LocalPosition = float4(input.Position, 1.0f);
    const float4 WorldPosition = mul(LocalPosition, Model);
    
    output.Position = mul(WorldPosition, ViewProjection);
    output.Normal = normalize(mul(float4(input.Normal, 0.0f), Model).xyz);
    output.Color = (UseVertexColor != 0) ? input.Color : Color;
    output.UV = input.UV;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 final_color = input.Color;
    if (HasTexture != 0)
    {
        final_color *= main_texture.Sample(default_sampler, input.UV);
    }
	
    return final_color;
}
