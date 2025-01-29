#include "MBConvBlock.h"

MBConvBlock::MBConvBlock(uint32_t inChannels, uint32_t outChannels, uint32_t stride, float ratio)
	: m_expandConv(torch::nn::Conv2dOptions(inChannels, inChannels *ratio, 1).bias(false)),
	m_expandBn(inChannels *ratio),
	m_depthwiseConv(torch::nn::Conv2dOptions(inChannels *ratio, inChannels *ratio, 3)
		.stride(stride)
		.padding(1)
		.groups(inChannels *ratio)
		.bias(false)),
	m_depthwiseBn(inChannels *ratio),
	m_projectConv(torch::nn::Conv2dOptions(inChannels *ratio, outChannels, 1).bias(false)),
	m_projectBn(outChannels),
	m_expandRatio(ratio),
	m_isSkip(inChannels == outChannels && stride == 1)
{
	register_module("m_expandConv", m_expandConv);
	register_module("m_expandBn", m_expandBn);
	register_module("m_depthwiseConv", m_depthwiseConv);
	register_module("m_depthwiseBn", m_depthwiseBn);
	register_module("m_projectConv", m_projectConv);
	register_module("m_projectBn", m_projectBn);
}

torch::Tensor MBConvBlock::forward(torch::Tensor x)
{
	const torch::Tensor identity = x;

	x = torch::relu(m_expandBn(m_expandConv(x)));
	x = torch::relu(m_depthwiseBn(m_depthwiseConv(x)));
	x = m_projectBn(m_projectConv(x));

	if (m_isSkip)
	{
		x += identity;
	}

	return x;
}