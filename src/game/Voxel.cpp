#include "Voxel.h"

#include "glm/ext/matrix_transform.hpp"
#include "prism/Components/Camera/CameraEditorController.h"
#include "prism/Components/Camera/FPSCameraController.h"
#include "prism/System/ScopeTimer.h"

using namespace Prism;

WorldGen::WorldGen(Core::SharedContextRef ctx, const std::string& name)
	:
	ILayer(ctx, name)
{
	
}

WorldGen::~WorldGen()
{
	
}

void WorldGen::OnAttach()
{
	m_Camera.SetPosition({ 325.f,256.f, -85.f });
	m_Camera.SetLookAt({ 325.f,128.f, 0.f });
	
	m_Camera.AttachController<Renderer::FPSCameraController<Renderer::PerspectiveCamera>>();
	m_Camera.GetController()->SetMoveSpeed(32);
	m_Ctx->Assets.Shaders->LoadAsset("baseshader", { "res/voxel.vert", "res/voxel.frag" });
	m_Shader = m_Ctx->Assets.Shaders->Get("baseshader");

	m_Chunks = MakeVec<Voxel::Chunk>();

	GenerateWorld(m_BlockSize, m_ChunkSize, 5, 5);

	m_CameraLocked = true;
}

void WorldGen::GenerateWorld(int BlockSize, int ChunkSize, int ChunkXCount, int ChunkYCount)
{
	m_IsGenerating = true;

	m_Noise.setScale(m_NoiseScale * m_NoiseMulti);
	m_Noise.setXOffset(m_NoiseXOffset);
	m_Noise.setYOffset(m_NoiseYOffset);
	
	m_Chunks->clear();
	m_ChunkData.clear();

	for (int i = 0; i < ChunkXCount * ChunkYCount; i++)
	{
		int bx = i % ChunkYCount;
		int bz = (i / ChunkXCount) % ChunkYCount;

		m_Chunks->emplace_back(ChunkSize, BlockSize);
		int last = m_Chunks->size() - 1;

		auto s = m_Chunks->at(last).Size();

		m_ChunkData.push_back({ glm::vec3{ s.x - 8, s.y, s.z } *glm::vec3(bx, 0.f, bz) });
		m_Chunks->at(last).SetOffset(bx, bz);
	}

	for (auto& chunk : *m_Chunks)
	{
		m_Ctx->Tasks->GetWorker("bg")->QueueTask([this, &chunk]()
			{
				chunk.Allocate();
				chunk.Populate([this](int x, int y)
					{
						auto noise = m_Noise.Fractal2(x, y);
						return (noise + 1) / 2;
					});
				chunk.GenerateMesh();
			});
	}

	m_IsGenerating = false;
}

void WorldGen::OnDetach()
{

}

void WorldGen::OnSystemEvent(Event& e)
{
	EventHandler handler(e);

	m_Camera.OnSystemEvent(e);
	CLASSEVENT(handler, KeyPressedEvent)
	{
		switch (e.GetKey())
		{
		case Keyboard::F2:
			if (m_CameraLocked)
			{
				m_Camera.GetController()->ResetDelta();
				m_Ctx->SystemOptions->DisableCursor();
				m_CameraLocked = !m_CameraLocked;
			} else
			{
				m_CameraLocked = !m_CameraLocked;
				m_Ctx->SystemOptions->EnableCursor();
			}
			break;
		}
	});
}

void WorldGen::OnGuiDraw()
{
	auto io = ImGui::GetIO();
	// TEMP
	m_CursorOverGui = io.WantCaptureMouse;
	
	ImGui::BeginMainMenuBar();
	ImGui::MenuItem("Graphics", 0, &m_ShowChunkCtrls);
	ImGui::MenuItem("World Generation", 0, &m_ShowBaseCtrls);
	ImGui::MenuItem("Controls", 0, &m_ShowControls);
	ImGui::EndMainMenuBar();

	if (m_ShowControls)
	{
		ImGui::Begin("System");
		ImGui::SliderFloat("Mouse Sensitivty ", &m_MouseSens, 0, 1);
		ImGui::SliderFloat("Camera Move Speed", &m_MoveSpeed, 0, 100);
		ImGui::Text("Toggle Wireframe = F1");
		ImGui::Text("Toggle Camera = F2");
		ImGui::End();
	}
	
	if (m_ShowChunkCtrls)
	{
		ImGui::Begin("Chunks");
		ImGui::SliderFloat("Noise Scale", &m_NoiseScale, 0, 2);
		ImGui::SliderFloat("Noise Scale Multiplier", &m_NoiseMulti, 0.1, 2);
		ImGui::SliderFloat("Noise X Offset", &m_NoiseXOffset, 0, 10);
		ImGui::SliderFloat("Noise Y Offset", &m_NoiseYOffset, 0, 10);
		ImGui::SliderInt("Block Size", &m_BlockSize, 2, 16);
		ImGui::SliderInt("Chunk Size", &m_ChunkSize, 8, 64);
		ImGui::SliderInt("Chunk Count", &m_ChunkCount, 8, 128);
		m_GenerateWorldBtn = ImGui::Button("Generate World");
	
		ImGui::End();
	}
	
	if (m_ShowBaseCtrls)
	{
		ImGui::Begin("Graphics");
		ImGui::SliderFloat3("Light Position", (float*)&m_LightPosition, -200, 200);
		ImGui::ColorEdit3("Light Color", (float*)&m_LightClr);
		ImGui::SliderFloat("Light Intensity", &m_LightIntensity, 0, 5);
		ImGui::End();
	}
}

void WorldGen::OnUpdate(float dt)
{
	m_Camera.GetController()->SetRotationSpeed(m_MouseSens);
	m_Camera.GetController()->SetMoveSpeed(m_MoveSpeed);

	m_Camera.ShouldLock(m_CameraLocked);
	m_Camera.OnUpdate(dt);

	if (m_GenerateWorldBtn && !m_IsGenerating)
	{
		m_GenerateWorldBtn = false;
		int size = (int) sqrt(m_ChunkCount);
		GenerateWorld(m_BlockSize, m_ChunkSize, size, size);
	}
}

void WorldGen::OnDraw()
{
	if (m_IsGenerating)
	{
		return;
	}
	glm::mat4 m(1.f);
	for (int i = 0; i < m_Chunks->size(); i++)
	{
		if (!m_Chunks->at(i).MeshReady())
		{
			continue;
		}
		
		m_Chunks->at(i).SendToGpu();
		m_Shader->Bind();
		m_Shader->SetMat4("transform", glm::translate(m, m_ChunkData[i].offset));
		m_Shader->SetInt("tex", 0);
		m_Shader->SetFloat3("lightPos", m_LightPosition);
		m_Shader->SetFloat("lightIntens", m_LightIntensity);
		m_Shader->SetFloat3("lightClr", m_LightClr);
		m_Shader->SetMat4("projectedview", m_Camera.GetProjectedView());
		m_Chunks->at(i).Render();
	}
}
