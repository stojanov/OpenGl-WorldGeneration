#pragma once

#include "prism/Components/ILayer.h"
#include "prism/Math/PerlinNoise.h"
#include "prism/Renderer/DynamicMesh.h"
#include "prism/Renderer/PerspectiveCamera.h"
#include "prism/Voxels/Chunk.h"

using namespace Prism;

class WorldGen : public ILayer
{
public:
	struct ChunkData
	{
		glm::vec3 offset;
	};
	
	WorldGen(Core::SharedContextRef ctx, const std::string& name);
	virtual ~WorldGen();

	void GenerateWorld(int BlockSize, int ChunkSize, int ChunkXCount, int ChunkYCount);
	void OnAttach() override;
	void OnDetach() override;
	void OnDraw() override;
	void OnSystemEvent(Event& e) override;
	void OnGuiDraw() override;
	void OnUpdate(float dt) override;
private:
	std::future<void> m_MeshGen;
	bool m_CursorOverGui{ false };

	// Hardcoded width and height for now
	Renderer::PerspectiveCamera m_Camera{ 90, 1280, 720, 0.1f, 2048.f };
	Ref<Gl::Shader> m_Shader;
	Math::PerlinNoise m_Noise;
	VectorPtr<Voxel::Chunk> m_Chunks;
	std::vector<ChunkData> m_ChunkData;
	bool m_CameraLocked{ true };
	glm::vec3 m_LightPosition{ 0.f, -200.f, 200.f };
	glm::vec3 m_LightClr{ 0.1f, 0.9f, 0.6f };
	float m_LightIntensity{ 1.f };
	bool m_IsGenerating{ false }; 
	bool m_GenerateWorldBtn{ false };
	bool m_ShowChunkCtrls{ true };
	bool m_ShowControls{ true };
	bool m_ShowBaseCtrls{ false };
	bool m_ShowSystemControls{ false };
	float m_NoiseMulti{ 1.f };
	float m_NoiseScale{ 0.025f };
	float m_NoiseXOffset{ 0.f };
	float m_NoiseYOffset{ 0.f };
	int m_BlockSize{ 4 };
	int m_ChunkSize{ 32 };
	int m_ChunkCount{ 25 };
	float m_MouseSens{ 0.3 };
	float m_MoveSpeed{ 35 };
	int m_MoveSpeedMultiplier{ 1 };
};
