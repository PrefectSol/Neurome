#ifndef __AGENT_H
#define __AGENT_H

#include <torch/torch.h>
#include <torch/script.h>

#include "MBConvBlock.h"

class Agent : public torch::nn::Module
{
public:
	explicit Agent(uint32_t hiddenSize);

	torch::Tensor forward(torch::Tensor x);

private:
    torch::nn::Conv2d m_stemConv;
    torch::nn::BatchNorm2d m_stemBn;

    std::shared_ptr<MBConvBlock> m_block1;
    std::shared_ptr<MBConvBlock> m_block2;
    std::shared_ptr<MBConvBlock> m_block3;

    torch::nn::AdaptiveAvgPool2d m_pool;

    torch::nn::GRU m_gru;

    torch::nn::Linear m_fcCoordinates;
    torch::nn::Linear m_fcActions;
};

#endif // !__AGENT_H
