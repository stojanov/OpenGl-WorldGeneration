#include "Chunk.h"


#include "glm/ext/matrix_transform.hpp"
#include "prism/Math/Interpolation.h"
#include "prism/Math/Smoothing.h"
#include "prism/System/ScopeTimer.h"

namespace std
{
	// TEMPPPP!!!
	template<> struct hash<Prism::Voxel::Vec2>
	{
		size_t operator()(const Prism::Voxel::Vec2& vec) const noexcept
		{
			hash<float> hasher;
			auto a = hasher(vec.x);
			auto b = hasher(vec.y);
			// (a ^ b) >> 2
			//return b + 0x9e3779b9 + (a << 6) + (a >> 2);
			return b + 0x9e3779b9 + (a << 6) + (a >> 2);
		}
	};
}

namespace Prism::Voxel
{
	Chunk::Chunk(int Size, int blockSize)
		:
		m_BlockSize(blockSize),
		m_XSize(Size),
		m_YSize(Size),
		m_ZSize(Size)
	{
		m_Mesh = MakePtr<Renderer::DynamicMesh>();
		
		m_NormalBuffer = m_Mesh->CreateNewVertexBuffer({
			{ Gl::ShaderDataType::Float3, "normal" }
		});

		m_ColorBuffer = m_Mesh->CreateNewVertexBuffer({
			{ Gl::ShaderDataType::Float3, "color" }
		});

		
		m_MeshReady = MakePtr<std::atomic_bool>();
		PR_INFO("MADE ATOMIC=");

		m_Colors.push_back({ 0.f, 0.f, 0.6f });
		m_Colors.push_back({ 0.9f, 0.9f, 0.52f });
		m_Colors.push_back({ 0.68f, 0.52f, 0.2f });
		m_Colors.push_back({ 0.f, 0.54f, 0.f });
		m_Colors.push_back({ 0.f, 0.75f, 0.f });
		m_Colors.push_back({ 0.f, 0.85f, 0.25f });
		m_Colors.push_back({ 0.f, 0.78f, 0.5f });
		m_Colors.push_back({ 0.f, 0.78f, 0.65f });
	}

	// Allocation is in another function in order to
	// defer it until it can be done in a separate thread
	void Chunk::Allocate()
	{
		if (m_IsAllocated)
		{
			return;
		}

		size_t total = m_XSize* m_ZSize* m_YSize;
		m_Blocks.resize(total);
		m_BlockHeights.resize(m_XSize * m_ZSize, 0);

		/*
		m_Mesh->AllocateBufferData(0, 4 * total);
		m_Mesh->AllocateBufferData(1, 8 * total);
		m_Mesh->AllocateIndexData(8192);
		*/
		m_IsAllocated = true;
		*m_MeshReady = false;
	}

	void Chunk::Populate(std::function<float(int, int)> popFunc)
	{
		System::Time::Scope<System::Time::Miliseconds> RandomTimer("Chunk Population");
		*m_MeshReady = false;
		for (int x = 0; x < m_XSize; x++)
		{
			for (int z = 0; z < m_ZSize; z++)
			{
				int height = ceil(popFunc(x + (m_XSize * m_XOffset), z + (m_ZSize * m_YOffset)) * (m_YSize - 1));
				height = glm::clamp(height, 0, m_YSize);
				for (int i = 0; i < height; i++)
				{
					// Todo: make this a mapping function
					if (i == height - 1)
					{
						m_Blocks[_GetBlockLoc(x, z, i)].Type = BlockType::DIRT;
						break;
					}
					m_Blocks[_GetBlockLoc(x, z, i)].Type = BlockType::BLOCK;
				}
				m_BlockHeights[_GetLoc(x, z)] = height;
			}
		}
	}

	void Chunk::_CreateQuad(
		int v0x, int v0y, int v0z, 
		int v1x, int v1y, int v1z, 
		int v2x, int v2y, int v2z, 
		int v3x, int v3y, int v3z)
	{
		glm::vec3 p0 = { v0x, v0y, v0z };
		glm::vec3 p1 = { v1x, v1y, v1z };
		glm::vec3 p2 = { v2x, v2y, v2z };
		glm::vec3 p3 = { v3x, v3y, v3z };

		auto v0p = m_Mesh->AddVertex(p0);
		auto v1p = m_Mesh->AddVertex(p1);
		auto v2p = m_Mesh->AddVertex(p2);
		auto v3p = m_Mesh->AddVertex(p3);

		 glm::vec3 normal = glm::cross((p2 - p0), (p3 - p1));
		
		m_Mesh->ConnectVertices(v0p, v1p, v3p);
		m_Mesh->ConnectVertices(v3p, v1p, v2p);
		
		_PassVertParam(m_NormalBuffer, normal);
		//_TexCord();
	}
	
	void Chunk::_PassVertParam(uint32_t buffer, const glm::vec3& param)
	{
		m_Mesh->AddVertex(buffer, param);
		m_Mesh->AddVertex(buffer, param);
		m_Mesh->AddVertex(buffer, param);
		m_Mesh->AddVertex(buffer, param);
	}

	void Chunk::_PassBlockParam(const glm::vec3& param)
	{
		for (int i = 0; i < m_CreatedFaces; i++)
		{
		}
	}

	void Chunk::SetOffset(int x, int y)
	{
		m_XOffset = x;
		m_YOffset = y;
		m_Position = glm::vec3{
			x * (m_BlockSize * m_XSize- 2 * m_BlockSize),
			0.f,
			y * m_BlockSize * m_ZSize
		};

		m_Transform = glm::translate(glm::mat4(1.f), m_Position);
	}
	
	void Chunk::SendToGpu()
	{
		if (m_DataSentToGpu)
		{
			return;
		}
		m_Mesh->Flush();

		m_DataSentToGpu = true;
	}

	void Chunk::Clear()
	{
		m_Blocks.clear();
		m_BlockHeights.clear();
	}

	void Chunk::PrepareForClearing()
	{
		*m_MeshReady = false;
	}

	void Chunk::Render()
	{
		m_Mesh->DrawIndexed();
	}
	
	void Chunk::GenerateMesh()
	{
		System::Time::Scope<System::Time::Miliseconds> RandomTimer("Chunk Mesh Generation");
		bool Sides[4] =
		{
			1,  // LEFT
			1,	// RIGHT
			1,	// FRONT
			1	// BACK
		};
		
		int yStart;
		int yEnd;
		int xStart;
		int xEnd;
		int zStart;
		int zEnd;

		int height;
		int l;

		// TEMP will optimize
		int ValidSides;

		// Will use
		int* VertexOffsets[48] = {
			// Left
			&xEnd, &yEnd, &zStart,
			&xEnd, &yStart, &zStart,
			&xEnd, &yStart, &zEnd,
			&xEnd, &yEnd, &zEnd,
			// Right
			&xStart, &yEnd, &zStart,
			&xStart, &yStart, &zStart,
			&xStart, &yStart, &zEnd,
			&xStart, &yEnd, &zEnd,
			// Front
			&xStart, &yEnd, &zEnd,
			&xStart, &yStart, &zEnd,
			&xEnd, &yStart, &zEnd,
			&xEnd, &yEnd, &zEnd,
			// Back
			&xStart, &yEnd, &zStart,
			&xStart, &yStart, &zStart,
			&xEnd, &yStart, &zStart,
			&xEnd, &yEnd, &zStart
		};

		int colorOffset = (m_YSize - 1) / (m_Colors.size() - 1);
		for (int x = 0; x < m_XSize; x++)
		{
			for (int z = 0; z < m_ZSize; z++)
			{
				height = m_BlockHeights[_GetLoc(x, z)];
				l = height;
				ValidSides = 4;

				xStart = x * m_BlockSize;
				xEnd = xStart + m_BlockSize;
				zStart = z * m_BlockSize;
				zEnd = zStart + m_BlockSize;
				
				Sides[0] = 1;
				Sides[1] = 1;
				Sides[2] = 1;
				Sides[3] = 1;
				
				while (l-- >= 0)
				{
					m_CreatedFaces = 0;
					ChunkBlockPosition current = _GetBlockState(x, z, l);
					if (current == ChunkBlockPosition::EDGE)
					{
						break;
					}

					yStart = l * m_BlockSize;
					yEnd = yStart + m_BlockSize;

					// Will change dramatically
					// Just a simple way to color the data
					float r;
					float g;
					float b;
					int activeClrIdx = l / colorOffset;
					int offset = l % colorOffset;

					int nextColorIdx = activeClrIdx == m_Colors.size() ? activeClrIdx : activeClrIdx + 1;
					int prevColorIdx = activeClrIdx == 0 ? activeClrIdx : activeClrIdx - 1;

					if (offset < 5)
					{
						r = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
								0, 5,
								m_Colors[prevColorIdx].r,
								m_Colors[activeClrIdx].r,
								offset
							);
						g = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
								0, 5,
								m_Colors[prevColorIdx].g,
								m_Colors[activeClrIdx].g,
								offset
							);
						b = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
								0, 5,
								m_Colors[prevColorIdx].b,
								m_Colors[activeClrIdx].b,
								offset
							);
					}
					else
					{
						r = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
							0, 5,
							m_Colors[activeClrIdx].r,
							m_Colors[nextColorIdx].r,
							offset
							);
						g = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
							0, 5,
							m_Colors[activeClrIdx].g,
							m_Colors[nextColorIdx].g,
							offset
							);
						b = Math::SmoothLinearTranslate<Math::fastACosSmooth>(
							0, 5,
							m_Colors[activeClrIdx].b,
							m_Colors[nextColorIdx].b,
							offset
							);
					}
					
					if (current == ChunkBlockPosition::TOP)
					{
						m_CreatedFaces++;
						_CreateQuad(
							 xStart,  yEnd,  zEnd,
							 xStart,  yEnd,  zStart,
							 xEnd,  yEnd,  zStart,
							 xEnd,  yEnd,  zEnd
						);
						_PassVertParam(m_ColorBuffer, { r, g, b });
					}

					
					 // WIP
					/*
					int positions[12] = {
						x + 1, z, l, // Left
						x - 1, z, l, // Right
						x, z + 1, l, // Front
						x, z - 1, l, // Back
					};
					for (int i = 0; i < 4; i++)
					{
						int* posPtr = &positions[i * 3];
						int** Vertex = &VertexOffsets[i * 12];
						if (_BlockExists(_GetBlockState(posPtr[0], posPtr[1], posPtr[2])))
						{
							_CreateQuad(
									*Vertex[0], *Vertex[1], *Vertex[2],
									*Vertex[3], *Vertex[4], *Vertex[5],
									*Vertex[6], *Vertex[7], *Vertex[8],
									*Vertex[9], *Vertex[10], *Vertex[11]
							);
						}
					}
					*/
					
					ChunkBlockPosition left = _GetBlockState(x + 1, z, l);
					ChunkBlockPosition right = _GetBlockState(x - 1, z, l);
					ChunkBlockPosition front = _GetBlockState(x, z + 1, l);
					ChunkBlockPosition back = _GetBlockState(x, z - 1, l);
					
					if (!_BlockExists(left))
					{
						m_CreatedFaces++;
						_CreateQuad(
							 xEnd,  yEnd,  zStart,
							 xEnd,  yStart,  zStart,
							 xEnd,  yStart,  zEnd,
							 xEnd,  yEnd,  zEnd
						);
						_PassVertParam(m_ColorBuffer, { r, g, b });
					}

					if (!_BlockExists(right))
					{
						m_CreatedFaces++;
						_CreateQuad(
							 xStart,  yEnd,  zStart,
							 xStart,  yStart,  zStart,
							 xStart,  yStart,  zEnd,
							 xStart,  yEnd,  zEnd
						);
						_PassVertParam(m_ColorBuffer, { r, g, b });
					}

					if (!_BlockExists(front))
					{
						m_CreatedFaces++;
						_CreateQuad(
							 xStart,  yEnd,  zEnd,
							 xStart,  yStart,  zEnd,
							 xEnd,  yStart,  zEnd,
							 xEnd,  yEnd,  zEnd
						);
						_PassVertParam(m_ColorBuffer, { r, g, b });
					}
					if (!_BlockExists(back))
					{
						m_CreatedFaces++;
						_CreateQuad(
							 xStart,  yEnd,  zStart,
							 xStart,  yStart,  zStart,
							 xEnd,  yStart,  zStart,
							 xEnd,  yEnd,  zStart
						);
						_PassVertParam(m_ColorBuffer, { r, g, b });
					}
				}
			}
		}

		*m_MeshReady = true;
	}
}
