#ifndef __PROXIMAL_POLICY_OPTIMIZATION_H
#define __PROXIMAL_POLICY_OPTIMIZATION_H

#include <string>
#include <vector>
#include <filesystem>

#include <torch/torch.h>
#include <torch/script.h>

#include "Agent.h"

class ProximalPolicyOptimization
{
public:
	explicit ProximalPolicyOptimization();

	bool init(std::string modelPath, uint32_t hiddenSize, float gamma, float epsilon,
		      uint32_t epochs, uint32_t bufferSize, float actorLr, float criticLr);

	torch::Tensor inference(torch::Tensor state);

	void storeExperience(torch::Tensor state, torch::Tensor action, float reward,
						 torch::Tensor nextState, float done, torch::Tensor oldLogProb);

private:
	typedef struct Experience
	{
		torch::Tensor state;
		torch::Tensor action;
		torch::Tensor nextState;
		torch::Tensor oldLogProb;
		float reward;
		float done;
	} Experience_t;

	std::string m_modelPath;
	
	uint32_t m_epochs;
	uint32_t m_bufferSize;

	std::vector<Experience> m_buffer;

	float m_gamma;
	float m_epsilon;

	std::shared_ptr<Agent> m_actor;
	std::shared_ptr<Agent> m_critic;

	std::unique_ptr<torch::optim::Adam> m_actorOptimizer;
	std::unique_ptr<torch::optim::Adam> m_criticOptimizer;

	void train();
};

#endif // !__PROXIMAL_POLICY_OPTIMIZATION_H