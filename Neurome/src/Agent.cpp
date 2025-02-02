#include "Agent.h"

Agent::Agent(uint32_t hiddenSize) 
    : m_stemConv(torch::nn::Conv2dOptions(3, 32, 3).stride(2).padding(1)),
    m_stemBn(32),
    m_block1(std::make_shared<MBConvBlock>(32, 64, 2)),
    m_block2(std::make_shared<MBConvBlock>(64, 128, 2)),
    m_block3(std::make_shared<MBConvBlock>(128, 256, 2)),
    m_pool(torch::nn::AdaptiveAvgPool2dOptions(1)),
    m_gru(torch::nn::GRUOptions(256, hiddenSize).num_layers(1).batch_first(true)),
    m_fcCoordinates(hiddenSize, 2),
    m_fcActions(hiddenSize, 2)
{
    register_module("m_stemConv", m_stemConv);
    register_module("m_stemBn", m_stemBn);
    register_module("m_block1", m_block1);
    register_module("m_block2", m_block2);
    register_module("m_block3", m_block3);
    register_module("m_pool", m_pool);
    register_module("m_gru", m_gru);
    register_module("m_fcCoordinates", m_fcCoordinates);
    register_module("m_fcActions", m_fcActions);
}

torch::Tensor Agent::forward(torch::Tensor x) 
{
    x = m_stemConv(x);
    x = m_stemBn(x);
    x = torch::relu(x);
    x = m_block1->forward(x);
    x = m_block2->forward(x);
    x = m_block3->forward(x);

    x = m_pool(x);
    
    x = x.view({ x.size(0), -1 }).unsqueeze(1);
    x = std::get<0>(m_gru->forward(x)).select(1, -1);

    const torch::Tensor coordinates = torch::sigmoid(m_fcCoordinates(x));
    const torch::Tensor actions = torch::softmax(m_fcActions(x), -1);

    return torch::cat({ coordinates, actions }, -1);
}