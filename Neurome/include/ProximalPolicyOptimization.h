#ifndef __PROXIMAL_POLICY_OPTIMIZATION_H
#define __PROXIMAL_POLICY_OPTIMIZATION_H

#include <string>
#include <vector>
#include <filesystem>
#include <thread>

#include <torch/torch.h>
#include <torch/script.h>

#include "Actor.h"
#include "Critic.h"

#define EPS 1e-8f

class ProximalPolicyOptimization
{
public:
	explicit ProximalPolicyOptimization();

	bool init(std::string modelPath, uint32_t hiddenSize,
			  uint32_t width, uint32_t height,
			  float gamma, float epsilon, float lambda, float gradClip,
			  float movementNoise, float actionNoise,
			  uint32_t epochs, uint32_t bufferSize, float actorLr, float criticLr);

	int load();
	
	int save();

	float getActorLoss() const;

	float getCriticLoss() const;

	torch::Tensor inference(torch::Tensor state);

	void storeExperience(torch::Tensor state, torch::Tensor action, float reward,
						 torch::Tensor nextState, float done, torch::Tensor oldLogProb);
private:
	const uint32_t m_channels;

	std::string m_modelPath;
	
	uint32_t m_width;
	uint32_t m_height;

	uint32_t m_epochs;
	uint32_t m_bufferSize;
	uint32_t m_bufferIndex;

	torch::Tensor m_statesBatch;
	torch::Tensor m_actionsBatch;
	torch::Tensor m_nextStatesBatch;
	torch::Tensor m_oldLogProbsBatch;
	torch::Tensor m_rewardsBatch;
	torch::Tensor m_donesBatch;

	float m_gamma;
	float m_epsilon;
	float m_lambda;
	float m_gradClip;

	float m_movementNoise;
	float m_actionNoise;

	std::shared_ptr<Actor> m_actor;
	std::shared_ptr<Critic> m_critic;

	std::unique_ptr<torch::optim::Adam> m_actorOptimizer;
	std::unique_ptr<torch::optim::Adam> m_criticOptimizer;

	float m_actorLoss;
	float m_criticLoss;

	void train();

	torch::Tensor computeGAE(const torch::Tensor &rewards,
							 const torch::Tensor &values,
						 	 const torch::Tensor &nextValues,
						 	 const torch::Tensor &dones) const;

	torch::Tensor computeLogProbs(const torch::Tensor &movement,
								  const torch::Tensor &confidence,
								  const torch::Tensor &actions) const;
};
#endif // !__PROXIMAL_POLICY_OPTIMIZATION_H