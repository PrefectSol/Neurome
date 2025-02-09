#include "ProximalPolicyOptimization.h"

ProximalPolicyOptimization::ProximalPolicyOptimization()
    : m_channels(1), m_modelPath(), m_epochs(0), m_bufferIndex(0), m_bufferSize(0),
    m_statesBatch(), m_actionsBatch(), m_nextStatesBatch(), m_oldLogProbsBatch(), m_rewardsBatch(), m_donesBatch(),
    m_gamma(0.0f), m_epsilon(0.0f), m_width(0), m_height(0), m_gradClip(0.0f),
    m_movementNoise(0.0f), m_actionNoise(0.0f),
    m_actor(), m_critic(), m_actorOptimizer(), m_criticOptimizer(),
    m_actorLoss(0.0f), m_criticLoss(0.0f) {}

bool ProximalPolicyOptimization::init(std::string modelPath, uint32_t hiddenSize,
                                      uint32_t width, uint32_t height,
                                      float gamma, float epsilon, float lambda, float gradClip,
                                      float movementNoise, float actionNoise,
                                      uint32_t epochs, uint32_t bufferSize, float actorLr, float criticLr)
{
    if (gamma <= 0.0f || epsilon <= 0.0f || 
        lambda <= 0.0f || lambda >= 1.0f ||
        epochs == 0 || bufferSize == 0 ||
        width == 0 || height == 0 ||
        gradClip <= 0.0f ||
        movementNoise <= 0.0f || actionNoise <= 0.0f ||
        actorLr <= 0.0f || criticLr <= 0.0f)
    {
        return false;
    }

    m_modelPath = modelPath;
    m_gamma = gamma;
    m_epsilon = epsilon;
    m_epochs = epochs;
    m_width = width;
    m_lambda = lambda;
    m_height = height;
    m_bufferSize = bufferSize;
    m_gradClip = gradClip;
    m_movementNoise = movementNoise;
    m_actionNoise = actionNoise;
    
    m_criticLoss = 0.0f;
    m_actorLoss = 0.0f;
    m_bufferIndex = 0;

    try
    {
        torch::set_num_threads(std::thread::hardware_concurrency());

        m_actor = std::make_shared<Actor>(hiddenSize);
        m_critic = std::make_shared<Critic>(hiddenSize);

        m_actor->to(torch::kCUDA);
        m_critic->to(torch::kCUDA);

        m_actorOptimizer = std::make_unique<torch::optim::Adam>(m_actor->parameters(), actorLr);
        m_criticOptimizer = std::make_unique<torch::optim::Adam>(m_critic->parameters(), criticLr);

        m_statesBatch = torch::zeros({ bufferSize, m_channels, m_width, m_height }, torch::kCUDA);
        m_actionsBatch = torch::zeros({ bufferSize, 3 }, torch::kCUDA);
        m_nextStatesBatch = torch::zeros({ bufferSize, m_channels, m_width, m_height }, torch::kCUDA);
        m_oldLogProbsBatch = torch::zeros({ bufferSize, 3 }, torch::kCUDA);
        m_rewardsBatch = torch::zeros({ bufferSize }, torch::kCUDA);
        m_donesBatch = torch::zeros({ bufferSize }, torch::kCUDA);
    } 
    catch (...)
    {
        return false;
    }

    return true;
}

int ProximalPolicyOptimization::load()
{
    if (m_modelPath.empty() ||
        !std::filesystem::exists(m_modelPath + "_actor.pt") ||
        !std::filesystem::exists(m_modelPath + "_critic.pt"))
    {
        return 0;
    }

    try
    {
        torch::load(m_actor, m_modelPath + "_actor.pt");
        torch::load(m_critic, m_modelPath + "_critic.pt");

        m_actor->to(torch::kCUDA);
        m_critic->to(torch::kCUDA);
    }
    catch (...)
    {
        return -1;
    }

    return 1;
}

int ProximalPolicyOptimization::save()
{
    if (m_modelPath.empty() ||
        !std::filesystem::exists(m_modelPath + "_actor.pt") ||
        !std::filesystem::exists(m_modelPath + "_critic.pt"))
    {
        return 0;
    }
    
    try
    {
        torch::save(m_actor, m_modelPath + "_actor.pt");
        torch::save(m_critic, m_modelPath + "_critic.pt");
    }
    catch (...) 
    {
        return -1;
    }

    return 1;
}

float ProximalPolicyOptimization::getActorLoss() const
{
    return m_actorLoss;
}

float ProximalPolicyOptimization::getCriticLoss() const
{
    return m_criticLoss;
}

torch::Tensor ProximalPolicyOptimization::inference(torch::Tensor state)
{
    torch::NoGradGuard noGrad;
    m_actor->eval();

    const torch::Tensor output = m_actor->forward(state);
    const torch::Tensor movement = output.slice(1, 0, 2);
    const torch::Tensor action = output.slice(1, 2, 3);

    const torch::Tensor movementNoise = torch::randn_like(movement) * m_movementNoise;
    const torch::Tensor actionNoise = torch::randn_like(action) * m_actionNoise;

    const torch::Tensor finalMovement = torch::tanh(movement + movementNoise);
    const torch::Tensor finalAction = torch::sigmoid(action + actionNoise);

    return torch::cat({ finalMovement, finalAction }, -1);
}

void ProximalPolicyOptimization::storeExperience(torch::Tensor state, torch::Tensor action, float reward,
                                                 torch::Tensor nextState, float done, torch::Tensor oldLogProb)
{
    m_statesBatch[m_bufferIndex] = state.squeeze();
    m_actionsBatch[m_bufferIndex] = action;
    m_nextStatesBatch[m_bufferIndex] = nextState.squeeze();
    m_oldLogProbsBatch[m_bufferIndex] = oldLogProb.squeeze();
    m_rewardsBatch[m_bufferIndex] = reward;
    m_donesBatch[m_bufferIndex] = done;

    ++m_bufferIndex;

    if (m_bufferIndex == m_bufferSize)
    {
        train();
        m_bufferIndex = 0;
    }
}

void ProximalPolicyOptimization::train()
{
    if (m_bufferIndex != m_bufferSize)
    {
        return;
    }

    m_actor->train();
    m_critic->train();

    m_criticLoss = 0.0f;
    m_actorLoss = 0.0f;
    for (uint32_t epoch = 0; epoch < m_epochs; ++epoch) 
    {
        const torch::Tensor values = m_critic->forward(m_statesBatch).squeeze();
        const torch::Tensor nextValues = m_critic->forward(m_nextStatesBatch).squeeze();

        torch::Tensor advantages = computeGAE(m_rewardsBatch, values, nextValues, m_donesBatch).unsqueeze(1);
        const torch::Tensor returns = advantages + values;

        advantages = (advantages - advantages.mean()) / (advantages.std() + EPS);

        const torch::Tensor action = m_actor->forward(m_statesBatch);

        const torch::Tensor movementSpeed = action.slice(1, 0, 2);
        const torch::Tensor confidence = action.slice(1, 2, 3);

        const torch::Tensor ratio = torch::exp(computeLogProbs(movementSpeed, confidence, m_actionsBatch) - m_oldLogProbsBatch);

        const torch::Tensor surr1 = ratio * advantages;
        const torch::Tensor surr2 = torch::clamp(ratio, 1.0f - m_epsilon, 1.0f + m_epsilon) * advantages;

        const torch::Tensor actorLoss = -torch::min(surr1, surr2).mean();
        const torch::Tensor criticLoss = 0.5f * (returns - values).pow(2).mean();

        m_actorOptimizer->zero_grad();
        actorLoss.backward({}, true);
        torch::nn::utils::clip_grad_norm_(m_actor->parameters(), m_gradClip);
        m_actorOptimizer->step();

        m_criticOptimizer->zero_grad();
        criticLoss.backward();
        torch::nn::utils::clip_grad_norm_(m_critic->parameters(), m_gradClip);
        m_criticOptimizer->step();

        m_actorLoss += actorLoss.item<float>();
        m_criticLoss += criticLoss.item<float>();
    }

    m_criticLoss /= m_epochs;
    m_actorLoss /= m_epochs;
}

torch::Tensor ProximalPolicyOptimization::computeGAE(const torch::Tensor &rewards,
                                                     const torch::Tensor &values,
                                                     const torch::Tensor &nextValues,
                                                     const torch::Tensor &dones) const
{
    const torch::Tensor deltas = rewards + m_gamma * nextValues * (1.0f - dones) - values;
    torch::Tensor advantages = torch::zeros_like(rewards);

    const float gaeLambda = m_gamma * m_lambda;
    float runningGae = 0.0f;

    for (int32_t t = m_bufferSize - 1; t >= 0; --t) 
    {
        runningGae = deltas[t].item<float>() + gaeLambda * (1.0f - dones[t].item<float>()) * runningGae;
        advantages[t] = runningGae;
    }

    return advantages;
}

torch::Tensor ProximalPolicyOptimization::computeLogProbs(const torch::Tensor &movement,
                                                          const torch::Tensor &confidence,
                                                          const torch::Tensor &actions) const
{
    const torch::Tensor movementLogProbs = -0.5 * ((actions.slice(1, 0, 2) - movement).pow(2) / 0.02f - 1.20182013164f);
    const torch::Tensor confidenceLogProbs = confidence * actions.slice(1, 2, 3).log() + (1 - confidence) * (1 - actions.slice(1, 2, 3)).log();

    return torch::cat({ movementLogProbs, confidenceLogProbs }, -1);
}