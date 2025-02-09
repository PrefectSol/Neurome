#ifndef __MULTI_HEAD_ATTENTION_H
#define __MULTI_HEAD_ATTENTION_H

#include <cstdint>

#include <torch/torch.h>

class MultiHeadAttention : public torch::nn::Module
{
public:
	explicit MultiHeadAttention(uint32_t dim, uint32_t numHeads);

	torch::Tensor forward(torch::Tensor x);

private:
	const uint32_t m_dim; 
	const uint32_t m_numHeads;

	const uint32_t m_headDim;
	const double m_scale;

	torch::nn::Linear m_qFc;
	torch::nn::Linear m_kFc;
	torch::nn::Linear m_vFc;

	torch::nn::Linear m_fc;
};
#endif // !__MULTI_HEAD_ATTENTION_H

