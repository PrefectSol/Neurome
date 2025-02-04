#include "ProximalPolicyOptimization.h"

ProximalPolicyOptimization::ProximalPolicyOptimization()
    : m_modelPath(), m_epochs(0), m_buffer(), m_gamma(0.0f), m_epsilon(0.0f),
    m_actor(), m_critic(), m_actorOptimizer(), m_criticOptimizer() {}

bool ProximalPolicyOptimization::init(std::string modelPath, uint32_t hiddenSize, float gamma, float epsilon,
                                 uint32_t epochs, uint32_t bufferSize, float actorLr, float criticLr)
{
    if (gamma <= 0 || epsilon <= 0 ||
        epochs == 0 || bufferSize == 0 ||
        actorLr <= 0 || criticLr <= 0)
    {
        return false;
    }

    m_modelPath = modelPath;
    m_gamma = gamma;
    m_epsilon = epsilon;
    m_epochs = epochs;
    m_bufferSize = bufferSize;

    try
    {
        m_actor = std::make_shared<Agent>(hiddenSize);
        m_critic = std::make_shared<Agent>(hiddenSize);

        if (!modelPath.empty() && std::filesystem::exists(modelPath + "_actor.pt") &&
                                  std::filesystem::exists(modelPath + "_critic.pt")) 
        {
            torch::load(m_actor, modelPath + "_actor.pt");
            torch::load(m_critic, modelPath + "_critic.pt");
        }

        m_actor->to(torch::kCUDA);
        m_critic->to(torch::kCUDA);

        m_actorOptimizer = std::make_unique<torch::optim::Adam>(m_actor->parameters(), actorLr);
        m_criticOptimizer = std::make_unique<torch::optim::Adam>(m_critic->parameters(), criticLr);

        m_buffer.reserve(bufferSize);
    } 
    catch (...)
    {
        return false;
    }

    return true;
}

torch::Tensor ProximalPolicyOptimization::inference(torch::Tensor state)
{
    m_actor->eval();
    torch::NoGradGuard no_grad;

    return m_actor->forward(state);
}

void ProximalPolicyOptimization::storeExperience(torch::Tensor state, torch::Tensor action, float reward,
                                                 torch::Tensor nextState, float done, torch::Tensor oldLogProb)
{
    m_buffer.push_back({ state, action, nextState, oldLogProb, reward, done });

    if (m_buffer.size() >= m_bufferSize) 
    {
        train();
        m_buffer.clear();
    }
}

void ProximalPolicyOptimization::train()
{
    if (m_buffer.empty())
    {
        return;
    }

    m_actor->train();
    m_critic->train();

    std::vector<torch::Tensor> states, actions, next_states, oldLogProbs;
    std::vector<float> rewards, dones;

    for (const auto &exp : m_buffer) 
    {
        states.push_back(exp.state);
        actions.push_back(exp.action);
        rewards.push_back(exp.reward);
        next_states.push_back(exp.nextState);
        dones.push_back(exp.done);
        oldLogProbs.push_back(exp.oldLogProb);
    }

    const torch::Tensor statesBatch = torch::stack(states);
    const torch::Tensor actionsBatch = torch::stack(actions);
    const torch::Tensor rewardsTensor = torch::tensor(rewards).to(torch::kCUDA);
    const torch::Tensor nextStatesBatch = torch::stack(next_states);
    const torch::Tensor donesTensor = torch::tensor(dones).to(torch::kCUDA);
    const torch::Tensor oldLogProbsBatch = torch::stack(oldLogProbs);

    for (uint32_t epoch = 0; epoch < m_epochs; ++epoch) 
    {
        const torch::Tensor currentValues = m_critic->forward(statesBatch);
        const torch::Tensor nextValues = m_critic->forward(nextStatesBatch);
        const torch::Tensor advantages = rewardsTensor + m_gamma * nextValues * (1.0f - donesTensor) - currentValues;

        const torch::Tensor newActionProbs = m_actor->forward(statesBatch);
        const torch::Tensor ratio = torch::exp(newActionProbs - oldLogProbsBatch);
        const torch::Tensor surr1 = ratio * advantages;
        const torch::Tensor surr2 = torch::clamp(ratio, 1.0f - m_epsilon, 1.0f + m_epsilon) * advantages;
        const torch::Tensor actorLoss = -torch::min(surr1, surr2).mean();

        m_actorOptimizer->zero_grad();
        actorLoss.backward();
        m_actorOptimizer->step();

        const torch::Tensor valuePred = m_critic->forward(statesBatch);
        const torch::Tensor returns = rewardsTensor + m_gamma * nextValues * (1.0f - donesTensor);

        const torch::Tensor coordinatesPred = valuePred.slice(-1, 0, 2);
        const torch::Tensor actionsPred = valuePred.slice(-1, 2);
        const torch::Tensor coordinatesTarget = returns.slice(-1, 0, 2);
        const torch::Tensor actionsTarget = returns.slice(-1, 2);

        const torch::Tensor mse_loss = torch::mse_loss(coordinatesPred, coordinatesTarget.detach());
        const torch::Tensor ce_loss = torch::cross_entropy_loss(actionsPred, actionsTarget.detach());
        const torch::Tensor criticLoss = mse_loss + ce_loss;

        m_criticOptimizer->zero_grad();
        criticLoss.backward();
        m_criticOptimizer->step();
    }

    if (!m_modelPath.empty())
    {
        try
        {
            torch::save(m_actor, m_modelPath + "_actor.pt");
            torch::save(m_critic, m_modelPath + "_critic.pt");
        }
        catch (...) {}
    }
}