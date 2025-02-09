#include "MultiHeadAttention.h"

MultiHeadAttention::MultiHeadAttention(uint32_t dim, uint32_t numHeads)
	: m_dim(std::max(dim, 1U)), m_numHeads(std::max(numHeads, 1U)),
	m_headDim(m_dim / m_numHeads), m_scale(std::sqrt(m_headDim)),
	m_qFc(torch::nn::Linear(m_dim, m_dim)),
	m_kFc(torch::nn::Linear(m_dim, m_dim)),
	m_vFc(torch::nn::Linear(m_dim, m_dim)),
	m_fc(torch::nn::Linear(m_dim, m_dim))

{
	register_module("m_qFc", m_qFc);
	register_module("m_kFc", m_kFc);
	register_module("m_vFc", m_vFc);
	register_module("m_fc", m_fc);
}

torch::Tensor MultiHeadAttention::forward(torch::Tensor x)
{
    const uint32_t batchSize = x.size(0);
    const uint32_t seqSize = x.size(1);

    const torch::Tensor q = m_qFc(x).view({ batchSize, -1, m_numHeads, m_headDim }).transpose(1, 2);
    const torch::Tensor k = m_kFc(x).view({ batchSize, -1, m_numHeads, m_headDim }).transpose(1, 2);
    const torch::Tensor v = m_vFc(x).view({ batchSize, -1, m_numHeads, m_headDim }).transpose(1, 2);

    const torch::Tensor scores = torch::matmul(q, k.transpose(-2, -1)) / m_scale;
    const torch::Tensor attention = torch::softmax(scores, -1);

    const torch::Tensor out = torch::matmul(attention, v).transpose(1, 2).contiguous().view({ batchSize, -1, m_dim });

    return m_fc(out);
}