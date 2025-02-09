#ifndef __CRITIC_H
#define __CRITIC_H

#include <cstdint>

#include <torch/torch.h>
#include <torch/script.h>

class Critic : public torch::nn::Module
{
public:
	explicit Critic(uint32_t hiddenSize);

	torch::Tensor forward(torch::Tensor x);

private:
	const uint32_t m_hiddenSize;

	torch::nn::Conv2d m_conv1;
	torch::nn::BatchNorm2d m_bn1;

	torch::nn::Conv2d m_conv2;
	torch::nn::BatchNorm2d m_bn2;

	torch::nn::Conv2d m_conv3;
	torch::nn::BatchNorm2d m_bn3;

	torch::nn::MultiheadAttention m_attention;
	torch::nn::LSTM m_lstm;

	torch::nn::Linear m_fc;

	void initWeights();
};
#endif // !__CRITIC_H
