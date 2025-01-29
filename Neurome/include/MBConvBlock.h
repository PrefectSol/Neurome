#ifndef __MBCONV_BLOCK_H
#define __MBCONV_BLOCK_H

#include <cstdint>

#include <torch/torch.h>
#include <torch/script.h>

class MBConvBlock : public torch::nn::Module
{
public:
	 MBConvBlock(uint32_t inChannels, uint32_t outChannels, uint32_t stride = 1, float ratio = 4.0f);

	torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Conv2d m_expandConv;
	torch::nn::BatchNorm2d m_expandBn;

	torch::nn::Conv2d m_depthwiseConv;
	torch::nn::BatchNorm2d m_depthwiseBn;

	torch::nn::Conv2d m_projectConv;
	torch::nn::BatchNorm2d m_projectBn;

	float m_expandRatio;

	bool m_isSkip;
};

#endif // !__MBCONV_BLOCK_H
