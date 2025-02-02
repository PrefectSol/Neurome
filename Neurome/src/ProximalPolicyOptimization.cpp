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

//class PPOTrainer {
//private:
//    std::shared_ptr<Agent> actor;
//    std::shared_ptr<Agent> critic;
//    std::unique_ptr<torch::optim::Adam> actor_optimizer;
//    std::unique_ptr<torch::optim::Adam> critic_optimizer;
//
//    const float gamma = 0.99f;
//    const float epsilon = 0.2f;
//    const int epochs = 3;
//    const int buffer_size = 32; 
//
//    struct Experience {
//        torch::Tensor state;
//        torch::Tensor action;
//        float reward;
//        torch::Tensor next_state;
//        float done;
//        torch::Tensor old_log_prob;
//    };
//
//    std::vector<Experience> buffer;
//    std::string model_path;

//public:
//    PPOTrainer() {}
//
//    bool init(uint32_t hidden_size, const std::string &path = "") {
//        model_path = path;
//
//        try {
//            if (!path.empty() && std::filesystem::exists(path + "_actor.pt") &&
//                std::filesystem::exists(path + "_critic.pt")) {
//
//                actor = std::make_shared<Agent>();
//                critic = std::make_shared<Agent>();
//
//                try {
//                    actor = torch::jit::load(path + "_actor.pt").get_module<Agent>();
//                    critic = torch::jit::load(path + "_critic.pt").get_module<Agent>();
//                }
//                catch (const c10::Error &e) {
//                    std::cerr << "Error loading models: " << e.msg() << std::endl;
//                    return false;
//                }
//            }
//            else {
//                actor = std::make_shared<Agent>(hidden_size);
//                critic = std::make_shared<Agent>(hidden_size);
//
//                if (!path.empty()) {
//                    torch::jit::save(*actor, path + "_actor.pt");
//                    torch::jit::save(*critic, path + "_critic.pt");
//                }
//            }
//
//            actor->to(torch::kCUDA);
//            critic->to(torch::kCUDA);
//
//            actor_optimizer = std::make_unique<torch::optim::Adam>(
//                actor->parameters(), 3e-4);
//            critic_optimizer = std::make_unique<torch::optim::Adam>(
//                critic->parameters(), 3e-4);
//
//            buffer.reserve(buffer_size);
//            return true;
//        }
//        catch (const std::exception &e) {
//            std::cerr << "Initialization error: " << e.what() << std::endl;
//            return false;
//        }
//    }
//
//    torch::Tensor get_action(torch::Tensor state) {
//        actor->eval();
//        torch::NoGradGuard no_grad;
//        auto action_probs = actor->forward(state);
//        return action_probs;
//    }
//
//    void store_experience(torch::Tensor state, torch::Tensor action, float reward,
//        torch::Tensor next_state, float done, torch::Tensor old_log_prob) {
//        buffer.push_back({ state, action, reward, next_state, done, old_log_prob });
//
//        if (buffer.size() >= buffer_size) {
//            train_step();
//            buffer.clear();
//        }
//    }
//
//    void train_step() {
//        if (buffer.empty()) return;
//
//        actor->train();
//        critic->train();
//
//        std::vector<torch::Tensor> states, actions, next_states, old_log_probs;
//        std::vector<float> rewards, dones;
//
//        for (const auto &exp : buffer) {
//            states.push_back(exp.state);
//            actions.push_back(exp.action);
//            rewards.push_back(exp.reward);
//            next_states.push_back(exp.next_state);
//            dones.push_back(exp.done);
//            old_log_probs.push_back(exp.old_log_prob);
//        }
//
//        auto states_batch = torch::stack(states);
//        auto actions_batch = torch::stack(actions);
//        auto rewards_tensor = torch::tensor(rewards).to(torch::kCUDA);
//        auto next_states_batch = torch::stack(next_states);
//        auto dones_tensor = torch::tensor(dones).to(torch::kCUDA);
//        auto old_log_probs_batch = torch::stack(old_log_probs);
//
//        for (int epoch = 0; epoch < epochs; epoch++) {
//            auto current_values = critic->forward(states_batch);
//            auto next_values = critic->forward(next_states_batch);
//            auto advantages = compute_advantages(rewards_tensor, current_values,
//                next_values, dones_tensor);
//
//            auto new_action_probs = actor->forward(states_batch);
//            auto ratio = torch::exp(new_action_probs - old_log_probs_batch);
//            auto surr1 = ratio * advantages;
//            auto surr2 = torch::clamp(ratio, 1.0f - epsilon, 1.0f + epsilon) * advantages;
//            auto actor_loss = -torch::min(surr1, surr2).mean();
//
//            actor_optimizer->zero_grad();
//            actor_loss.backward();
//            actor_optimizer->step();
//
//            auto value_pred = critic->forward(states_batch);
//            auto returns = rewards_tensor + gamma * next_values * (1.0f - dones_tensor);
//            auto critic_loss = torch::mse_loss(value_pred, returns.detach());
//
//            critic_optimizer->zero_grad();
//            critic_loss.backward();
//            critic_optimizer->step();
//        }
//
//        if (!model_path.empty()) {
//            torch::jit::save(*actor, model_path + "_actor.pt");
//            torch::jit::save(*critic, model_path + "_critic.pt");
//        }
//    }
//
//private:
//    torch::Tensor compute_advantages(torch::Tensor rewards, torch::Tensor values,
//        torch::Tensor next_values, torch::Tensor dones) {
//        return rewards + gamma * next_values * (1.0f - dones) - values;
//    }
//};