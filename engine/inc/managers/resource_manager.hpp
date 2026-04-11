#pragma once

#include "core/ecs.hpp"
#include "core/system.hpp"

namespace Apex
{
    struct Node
    {
        std::string m_name;
        glm::mat4 m_transform;
        int m_mesh_index;
        std::vector<int> m_children;
    };

    struct Vertex
    {
        u32 normal;
        u32 tangent;
        float bitangent;
        u32 uv;
    };

    enum MaterialFlags : u32
    {
        FLAG_USE_CLEARCOAT_NORMAL_MAP = 1 << 1,
    };

    enum class AlphaMode : u32
    {
        eOpaque,
        eBlend,
        eMask,
    };

    struct Material
    {
        u32 albedo;
        u32 emissive;
        u32 metal_rough;
        u32 ior_opacity;
        u32 clear_coat;
        AlphaMode alpha_mode;
        u32 flags;

        int albedo_index;
        int emissive_index;
        int metal_rough_index;
        int normal_index;
        int occlusion_index;
        int clearcoat_index;
        int clearcoat_rough_index;
        int clearcoat_normal_index;

        u32 padding;
    };

    struct TextureCreateInfo
    {
        std::vector<u8> pixels;
        u32 width = 1;
        u32 height = 1;
        u32 array_count = 1;
        u32 mip_count = 1;
        Swift::Format format = Swift::Format::eRGBA8_UNORM;
    };

    struct Texture
    {
        Texture(const std::string& name, const TextureCreateInfo& create_info);

        TextureCreateInfo m_create_info;
        std::string m_name;
        Swift::ITexture* m_texture;
        Swift::ITextureView* m_texture_view;
    };

    struct Mesh
    {
        std::string name;
        u32 vertex_offset;
        u32 vertex_count;
        u32 meshlet_offset;
        u32 meshlet_count;
        u32 triangle_offset;
        u32 triangle_count;
        u32 material_index;
    };

    struct Meshlet
    {
        u32 vertex_offset;
        u32 triangle_offset;
        u32 vertex_count;
        u32 triangle_count;
    };

    struct CullData
    {
        glm::vec3 center{};
        float radius{};
        glm::vec3 cone_apex;
        u32 cone_packed;
    };

    struct Model
    {
        std::string m_name;
        std::vector<int> m_root_nodes;
        std::vector<Node> m_nodes;
        std::vector<Mesh> m_meshes;
        std::vector<Material> m_materials;
        std::vector<Texture> m_textures;
    };

    class ResourceManager final : public System
    {
    public:
        void Init() override;
        void EndFrame() override;

        enum class LoadError
        {
            eInvalidFile,
            eNone,
        };

        std::expected<Ref<Model>, LoadError> LoadModel(const fs::path& path);
        std::expected<Entity, LoadError> InstantiateModel(const fs::path& path);
        static std::expected<Texture, LoadError> LoadTexture(const fs::path& path);

    private:
        static void LoadNode(const Ref<Model>& model, const Node& node, Entity parent_entity);
        Mesh LoadMesh(const aiMesh* mesh);
        Material LoadMaterial(std::unordered_map<std::string, int> texture_index_map, const aiMaterial* material) const;
        static Texture LoadTexture(const aiTexture* texture);
        static int LoadNodeTree(const aiNode* ainode, std::vector<Node>& nodes);
        static glm::uvec2 QuantizePosition(glm::vec3 position);
        static u32 QuantizeNormal(const aiVector3D& normal);
        static u32 QuantizeUV(const aiVector3D& uv);

        Assimp::Importer importer;
        std::vector<std::weak_ptr<Model>> m_loaded_models;
        u32 m_vertex_count = 0;
        u32 m_meshlet_count = 0;
        u32 m_triangle_count = 0;
        u32 m_material_count = 0;

        Swift::ITexture* m_dummy_white_texture = nullptr;
        Swift::ITextureView* m_dummy_white_texture_view = nullptr;
        Swift::ITexture* m_dummy_black_texture = nullptr;
        Swift::ITextureView* m_dummy_black_texture_view = nullptr;
        Swift::ITexture* m_dummy_normal_texture = nullptr;
        Swift::ITextureView* m_dummy_normal_texture_view = nullptr;
    };
} // namespace Apex
