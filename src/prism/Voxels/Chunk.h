#pragma once
#include <functional>
#include <vector>

#include "prism/Renderer/DynamicMesh.h"
#include "prism/Core/SharedContext.h"

namespace Prism::Voxel
{
	struct Vec2
	{
		int x;
		int y;

		bool operator==(const Prism::Voxel::Vec2& other) const noexcept
		{
			return (x == other.x) && (y == other.y);
		}
	};
	
	class Chunk
	{
	public:
		enum class BlockType
		{
			NONE = 0,
			BLOCK,// TEMP
			DIRT
		};

		struct BlockData
		{
			BlockType Type{ BlockType::NONE };
			Vec2 Pos;
		};

		enum class ChunkBlockPosition
		{
			NONEXIST = 0,
			TOP,
			EDGE,
			BODY,
			
			COUNT
		};

		Chunk(int Size, int blockSize);
		
		void Allocate();
		void Populate(std::function<float(int, int)> popFunc);
		void GenerateMesh();
		void SendToGpu();
		void SetOffset(int x, int y);
		const Vec2& GetOffset()
		{
			return Vec2{ m_XOffset, m_YOffset };
		}
		void RebuildMesh();
		void UpdateGpu(); // Will update only if rebuild has been called

		glm::mat4& GetTransform()
		{
			return m_Transform;
		}

		glm::vec3& GetPosition()
		{
			return m_Position;
		}
		
		const glm::vec3& Size()
		{
			return glm::vec3{
				m_XSize * m_BlockSize,
				m_YSize * m_BlockSize,
				m_ZSize * m_BlockSize,
			};
		}

		bool MeshReady()
		{
			return *m_MeshReady;
		}

		// Will prepare for destruction
		void Clear();
		void PrepareForClearing();
		void Render();
	private:
		void _CreateQuad(
			int v0x, int v0y, int v0z,
			int v1x, int v1y, int v1z,
			int v2x, int v2y, int v2z,
			int v4x, int v3y, int v3z
		);
		void _PassBlockParam(const glm::vec3& param);
		void _PassVertParam(uint32_t buffer, const glm::vec3& param);

		int _GetLoc(int x, int y) const
		{
			return m_XSize * y + x;
		}
		int _GetBlockLoc(int x, int y, int z) const
		{
			return m_XSize * (y + m_ZSize * z) + x;
		}
		int _GetMeshOffset(int x, int y, int z) const
		{
			if ((x * y * z * 2) > m_Mesh->GetIndexData().size())
			{
				return -1;
			}
			x <<= 1;
			y <<= 1;
			z <<= 1;
			return m_XSize * (y + m_ZSize * z) + x;
		}
		ChunkBlockPosition _GetBlockState(int x, int y, int z)
		{
			if (x == 0 || x == m_XSize - 1 || z == 0 || z == m_ZSize - 1)
			{
				return ChunkBlockPosition::EDGE;
			}
			if (
					(x < 0 || x >= m_XSize || z < 0 || z >= m_ZSize || y < 0 || y > m_YSize)
				)
			{
				return ChunkBlockPosition::NONEXIST;
			}

			static ChunkBlockPosition ChunkBlockSelection[3] = {
				ChunkBlockPosition::NONEXIST,
				ChunkBlockPosition::BODY,
				ChunkBlockPosition::TOP
			};
			
			return ChunkBlockSelection[(int)m_Blocks[_GetBlockLoc(x, y, z)].Type];
		}

		bool _BlockExists(ChunkBlockPosition b)
		{
			return b == ChunkBlockPosition::BODY || b == ChunkBlockPosition::TOP;
		}
	
		Ptr<Renderer::DynamicMesh> m_Mesh;
		std::vector<BlockData> m_Blocks; // Vector of vectors to secure infinite height on terrain
		 // Will be used once the mesh is created to create a more
		//  optimized mesh for adding and removing blocks
		std::vector<int> m_BlockHeights;
		Ptr<std::atomic_bool> m_MeshReady;
		std::vector<glm::vec3> m_Colors;
		glm::vec3 m_Position;
		glm::mat4 m_Transform{ 1.f };
		int m_CreatedFaces{ 0 };
		bool m_IsAllocated{ false };
		bool m_DataSentToGpu{ false };
		uint32_t m_NormalBuffer;
		uint32_t m_ColorBuffer;
		int m_BlockSize;
		int m_XSize;
		int m_YSize;
		int m_ZSize;
		int m_XOffset{ 0 };
		int m_YOffset{ 0 }; 
	};
	
}
