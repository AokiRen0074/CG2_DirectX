struct PixelShaderOutput
{
    float32_t4 color : SV_Target;
};

PixelShaderOutput main()
{
    PixelShaderOutput output;
    output.color = float32_t4(1.0, 1.0, 1.0, 1.0);
    return output;

}