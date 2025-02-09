#include "Critic.h"

Critic::Critic(uint32_t hiddenSize)
    : m_hiddenSize(std::max(hiddenSize, 1U)),
    m_conv1(torch::nn::Conv2dOptions(1, 32, 3).stride(2).padding(1)),
    m_bn1(32),
    m_conv2(torch::nn::Conv2dOptions(32, 64, 3).stride(2).padding(1)),
    m_bn2(64),
    m_conv3(torch::nn::Conv2dOptions(64, 128, 3).stride(2).padding(1)),
    m_bn3(128),
    m_attention(torch::nn::MultiheadAttention(128, 8)),
    m_lstm(torch::nn::LSTMOptions(128, m_hiddenSize).num_layers(2).batch_first(true)),
    m_fc(torch::nn::Linear(m_hiddenSize, 1))
{
    register_module("m_conv1", m_conv1);
    register_module("m_bn1", m_bn1);
    register_module("m_conv2", m_conv2);
    register_module("m_bn2", m_bn2);
    register_module("m_conv3", m_conv3);
    register_module("m_bn3", m_bn3);
    register_module("m_attention", m_attention);
    register_module("m_lstm", m_lstm);
    register_module("m_fc", m_fc);

    initWeights();
}

torch::Tensor Critic::forward(torch::Tensor x)
{
    x = torch::relu(m_bn1(m_conv1(x)));
    x = torch::relu(m_bn2(m_conv2(x)));
    x = torch::relu(m_bn3(m_conv3(x)));

    const uint32_t batch_size = x.size(0);
    const uint32_t channels = x.size(1);
    x = x.view({ batch_size, channels, -1 });
    x = x.permute({ 2, 0, 1 });

    const auto attention_out = m_attention->forward(x, x, x);
    x = std::get<0>(attention_out);

    x = x.permute({ 1, 0, 2 });
    const auto lstmOut = m_lstm->forward(x);
    x = std::get<0>(lstmOut).select(1, -1);

    return m_fc(x);
}

void Critic::initWeights()
{
    torch::nn::init::xavier_uniform_(m_conv1->weight);
    torch::nn::init::xavier_uniform_(m_conv2->weight);
    torch::nn::init::xavier_uniform_(m_conv3->weight);

    torch::nn::init::normal_(m_conv1->bias, 0.0, 0.01);
    torch::nn::init::normal_(m_conv2->bias, 0.0, 0.01);
    torch::nn::init::normal_(m_conv3->bias, 0.0, 0.01);

    torch::nn::init::kaiming_uniform_(m_fc->weight, std::sqrt(5));

    torch::nn::init::uniform_(m_fc->bias, -0.1, 0.1);
}