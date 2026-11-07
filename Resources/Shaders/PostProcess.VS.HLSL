struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

// ���_�o�b�t�@(C++����̃f�[�^)�͎󂯎�炸�A���_�ԍ�(VertexID)�������󂯎��
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // ���_�ԍ�(0, 1, 2)����A��ʂ𕢂�����ȎO�p�`��UV���W(0,0 �` 2,2)���v�Z���鏬�Z
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);

    // UV���W�����ʏ�̈ʒu(X, Y)���v�Z
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}
