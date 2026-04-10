#include "managers/resource_manager.hpp"
#include "core/engine.hpp"
#include "core/renderer.hpp"
#include "dds.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr Swift::Format FromDXGIFormat(const dds::DXGI_FORMAT format) noexcept
{
    switch (format)
    {
    case dds::DXGI_FORMAT_R8G8B8A8_UNORM:
        return Swift::Format::eRGBA8_UNORM;
    case dds::DXGI_FORMAT_R16G16B16A16_FLOAT:
        return Swift::Format::eRGBA16F;
    case dds::DXGI_FORMAT_R32G32B32A32_FLOAT:
        return Swift::Format::eRGBA32F;
    case dds::DXGI_FORMAT_D32_FLOAT:
    case dds::DXGI_FORMAT_R32_FLOAT:
    case dds::DXGI_FORMAT_R32_TYPELESS:
        return Swift::Format::eD32F;
    case dds::DXGI_FORMAT_BC1_UNORM:
        return Swift::Format::eBC1_UNORM;
    case dds::DXGI_FORMAT_BC1_UNORM_SRGB:
        return Swift::Format::eBC1_UNORM_SRGB;
    case dds::DXGI_FORMAT_BC2_UNORM:
        return Swift::Format::eBC2_UNORM;
    case dds::DXGI_FORMAT_BC2_UNORM_SRGB:
        return Swift::Format::eBC2_UNORM_SRGB;
    case dds::DXGI_FORMAT_BC3_UNORM:
        return Swift::Format::eBC3_UNORM;
    case dds::DXGI_FORMAT_BC3_UNORM_SRGB:
        return Swift::Format::eBC3_UNORM_SRGB;
    case dds::DXGI_FORMAT_BC4_UNORM:
        return Swift::Format::eBC4_UNORM;
    case dds::DXGI_FORMAT_BC4_SNORM:
        return Swift::Format::eBC4_SNORM;
    case dds::DXGI_FORMAT_BC5_UNORM:
        return Swift::Format::eBC5_UNORM;
    case dds::DXGI_FORMAT_BC5_SNORM:
        return Swift::Format::eBC5_SNORM;
    case dds::DXGI_FORMAT_BC6H_UF16:
        return Swift::Format::eBC6H_UF16;
    case dds::DXGI_FORMAT_BC6H_SF16:
        return Swift::Format::eBC6H_SF16;
    case dds::DXGI_FORMAT_BC7_UNORM:
        return Swift::Format::eBC7_UNORM;
    case dds::DXGI_FORMAT_BC7_UNORM_SRGB:
        return Swift::Format::eBC7_UNORM_SRGB;
    default:
        return Swift::Format::eRGBA8_UNORM;
    }
}

Apex::Texture::Texture(const std::string& name, const TextureCreateInfo& create_info)
{
    m_name = name;
    m_create_info = create_info;
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    m_texture = Swift::TextureBuilder(context, create_info.width, create_info.height).
                SetArraySize(create_info.array_count).SetMipmapLevels(create_info.mip_count).SetFormat(
                    create_info.format).
                SetData(
                    create_info.pixels.data())
                .SetName(name).Build();
    m_texture_view = context->CreateTextureView(m_texture, {
                                                    .type = Swift::TextureViewType::eShaderResource,
                                                    .mip_count = create_info.mip_count,
                                                    .layer_count = create_info.array_count,
                                                });
}

void Apex::ResourceManager::Init()
{
    auto* context = g_engine.GetSystem<Renderer>()->GetContext();
    constexpr auto white = 0xFFFFFFFF;
    m_dummy_white_texture = Swift::TextureBuilder(context, 1, 1)
                            .SetFlags(EnumFlags(Swift::TextureFlags::eShaderResource))
                            .SetFormat(Swift::Format::eRGBA8_UNORM)
                            .SetData(&white)
                            .Build();
    m_dummy_white_texture_view = context->CreateTextureView(m_dummy_white_texture, {
                                                                .type = Swift::TextureViewType::eShaderResource,
                                                            });

    constexpr auto black = 0xFF000000;
    m_dummy_black_texture = Swift::TextureBuilder(context, 1, 1)
                            .SetFlags(EnumFlags(Swift::TextureFlags::eShaderResource))
                            .SetFormat(Swift::Format::eRGBA8_UNORM)
                            .SetData(&black)
                            .Build();
    m_dummy_black_texture_view = context->CreateTextureView(m_dummy_black_texture, {
                                                                .type = Swift::TextureViewType::eShaderResource,
                                                            });

    constexpr auto normal = 0xFFFF8080;
    m_dummy_normal_texture = Swift::TextureBuilder(context, 1, 1)
                             .SetFlags(EnumFlags(Swift::TextureFlags::eShaderResource))
                             .SetFormat(Swift::Format::eRGBA8_UNORM)
                             .SetData(&normal)
                             .Build();
    m_dummy_normal_texture_view = context->CreateTextureView(m_dummy_normal_texture, {
                                                                 .type = Swift::TextureViewType::eShaderResource,
                                                             });
}

void Apex::ResourceManager::EndFrame()
{
}

std::expected<Ref<Apex::Model>, Apex::ResourceManager::LoadError> Apex::ResourceManager::LoadModel(const fs::path& path)
{
    auto renderer = g_engine.GetSystem<Renderer>();
    constexpr u32 import_flags =
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals;

    const aiScene* scene = importer.ReadFile(path.string(), import_flags);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        printf("Assimp error: %s\n", importer.GetErrorString());
        return std::unexpected(LoadError::eInvalidFile);
    }

    const std::string base_path = path.parent_path().string();
    auto model = std::make_shared<Model>();

    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        auto* mesh = scene->mMeshes[i];
        model->m_meshes.emplace_back(LoadMesh(mesh));
    }

    std::unordered_map<std::string, int> texture_index_map;
    for (int i = 0; i < scene->mNumTextures; i++)
    {
        const auto* texture = scene->mTextures[i];
        model->m_textures.emplace_back(LoadTexture(texture));
        texture_index_map["*" + std::to_string(i)] = model->m_textures.back().m_texture_view->GetDescriptorIndex();
    }

    for (int i = 0; i < scene->mNumMaterials; i++)
    {
        const auto* material = scene->mMaterials[i];
        model->m_materials.emplace_back(LoadMaterial(texture_index_map, material));
    }
    renderer->GetMaterialBuffer()->Write(model->m_materials.data(), m_material_count * sizeof(Material),
                                         model->m_materials.size() * sizeof(Material));
    m_material_count += model->m_materials.size();

    int rootIndex = LoadNodeTree(scene->mRootNode, model->m_nodes);
    model->m_root_nodes.push_back(rootIndex);

    return model;
}

std::expected<Entity, Apex::ResourceManager::LoadError> Apex::ResourceManager::InstantiateModel(const fs::path& path)
{
    auto exp_model = LoadModel(path);
    if (!exp_model.has_value()) return std::unexpected(exp_model.error());

    const auto model = std::move(exp_model.value());

    auto entity = ECS::CreateEntity(path.string());

    for (int i = 0; i < model->m_root_nodes.size(); ++i)
    {
        auto node = model->m_nodes[model->m_root_nodes[i]];
        LoadNode(model, node, entity);
    }

    const auto& renderer = g_engine.GetSystem<Renderer>();
    renderer->MarkDrawBufferDirty();
    return entity;
}

std::expected<Apex::Texture, Apex::ResourceManager::LoadError> Apex::ResourceManager::LoadTexture(const fs::path& path)
{
    std::ifstream stream;
    stream.open(path, std::ios::binary);
    std::vector<char> header_data;
    header_data.resize(sizeof(dds::Header));
    stream.read(header_data.data(), sizeof(dds::Header));
    const auto header = dds::read_header(header_data.data(), sizeof(dds::Header));
    stream.seekg(header.data_offset(), std::ios::beg);
    std::vector<uint8_t> data;
    data.resize(header.data_size());
    stream.read(reinterpret_cast<char*>(data.data()), header.data_size());
    return Texture(path.stem().string(), TextureCreateInfo{
                       .pixels = data,
                       .width = header.width(),
                       .height = header.height(),
                       .array_count = header.array_size(),
                       .mip_count = header.mip_levels(),
                       .format = FromDXGIFormat(header.format()),
                   });
}

void Apex::ResourceManager::LoadNode(const Ref<Model>& model, const Node& node, const Entity parent_entity)
{
    const auto& renderer = g_engine.GetSystem<Renderer>();
    const auto entity = ECS::CreateEntity(node.m_name);
    ECS::AddChild(parent_entity, entity);
    glm::vec3 pos, rot, scale;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node.m_transform),
                                          glm::value_ptr(pos),
                                          glm::value_ptr(rot),
                                          glm::value_ptr(scale));
    auto& transform = ECS::GetComponent<Component::Transform>(entity);
    transform.SetLocalTransform(pos, rot, scale);

    if (node.m_mesh_index >= 0)
    {
        ECS::AddComponent<Component::Renderable>(entity, model, node.m_mesh_index);
        if (model->m_materials[model->m_meshes[node.m_mesh_index].material_index].alpha_mode == AlphaMode::eBlend)
        {
            ECS::AddComponent<Component::Translucent>(entity);
        }
        transform.m_transform_index = renderer->AddTransform(transform.m_world_matrix);
    }

    for (int childIndex : node.m_children)
    {
        const auto& child = model->m_nodes[childIndex];
        LoadNode(model, child, entity);
    }
}

Apex::Mesh Apex::ResourceManager::LoadMesh(const aiMesh* mesh)
{
    std::vector<glm::vec3> positions;
    std::vector<Vertex> vertices;
    positions.reserve(mesh->mNumVertices);
    vertices.reserve(mesh->mNumVertices);

    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        auto vertex = mesh->mVertices[i];
        positions.emplace_back(vertex.x, vertex.y, vertex.z);

        auto normal = QuantizeNormal(mesh->mNormals[i]);
        auto tangent = QuantizeNormal(mesh->mTangents[i]);
        auto bitangent = glm::dot(glm::cross(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
                                             glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                                                       mesh->mTangents[i].z)),
                                  glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                            mesh->mBitangents[i].z)) >= 0.0f
                             ? 1.0f
                             : -1.0f;
        auto uv = QuantizeUV(mesh->mTextureCoords[0][i]);
        vertices.emplace_back(normal, tangent, bitangent, uv);
    }

    std::vector<u32> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        const auto face = mesh->mFaces[i];
        indices.emplace_back(face.mIndices[0]);
        indices.emplace_back(face.mIndices[1]);
        indices.emplace_back(face.mIndices[2]);
    }

    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), positions.size());

    auto max_meshlets = meshopt_buildMeshletsBound(indices.size(), k_max_vertices, k_max_triangles);

    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<u32> meshlet_vertices(indices.size());
    std::vector<u8> meshlet_triangles(indices.size());
    auto meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(),
                                               indices.data(), indices.size(),
                                               reinterpret_cast<float*>(positions.data()), positions.size(),
                                               sizeof(glm::vec3), k_max_vertices, k_max_triangles, k_cone_weight);
    const meshopt_Meshlet& last = meshlets[meshlet_count - 1];

    meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
    meshlet_triangles.resize(last.triangle_offset + last.triangle_count * 3);
    meshlets.resize(meshlet_count);

    for (const auto& meshlet : meshlets)
    {
        meshopt_optimizeMeshlet(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset],
                                meshlet.triangle_count, meshlet.vertex_count);
    }

    std::vector<u32> packed_triangles;
    for (auto& m : meshlets)
    {
        const auto triangle_offset = static_cast<u32>(packed_triangles.size());

        for (u32 i = 0; i < m.triangle_count; ++i)
        {
            const auto idx0 = meshlet_triangles[m.triangle_offset + i * 3 + 0];
            const auto idx1 = meshlet_triangles[m.triangle_offset + i * 3 + 1];
            const auto idx2 = meshlet_triangles[m.triangle_offset + i * 3 + 2];
            auto packed = (static_cast<u32>(idx0) & 0xFF) << 0 | (static_cast<u32>(idx1) & 0xFF) << 8 |
                (static_cast<u32>(idx2) & 0xFF) << 16;
            packed_triangles.push_back(packed);
        }

        m.triangle_offset = triangle_offset;
    }

    std::vector<glm::uvec2> packed_positions;
    std::vector<Vertex> packed_vertices;
    u32 vertex_base = m_vertex_count;
    u32 triangle_base = m_triangle_count;

    for (auto& meshlet : meshlets)
    {
        const u32* meshlet_verts = &meshlet_vertices[meshlet.vertex_offset];

        for (u32 i = 0; i < meshlet.vertex_count; ++i)
        {
            const u32 vertex_index = meshlet_verts[i];
            packed_positions.push_back(QuantizePosition(positions[vertex_index]));
            packed_vertices.push_back(vertices[vertex_index]);
        }

        meshlet.vertex_offset = vertex_base;
        meshlet.triangle_offset = triangle_base;

        vertex_base += meshlet.vertex_count;
        triangle_base += meshlet.triangle_count;
    }

    Mesh new_mesh
    {
        .name = mesh->mName.C_Str(),
        .vertex_offset = m_vertex_count,
        .vertex_count = static_cast<u32>(packed_positions.size()),
        .meshlet_offset = m_meshlet_count,
        .meshlet_count = static_cast<u32>(meshlet_count),
        .triangle_offset = m_triangle_count,
        .triangle_count = static_cast<u32>(packed_triangles.size()),
        .material_index = m_material_count + mesh->mMaterialIndex,
    };

    const auto renderer = g_engine.GetSystem<Renderer>();
    renderer->GetPositionBuffer()->Write(packed_positions.data(), m_vertex_count * sizeof(glm::uvec2),
                                         packed_positions.size() * sizeof(glm::uvec2));
    renderer->GetVertexBuffer()->Write(packed_vertices.data(), m_vertex_count * sizeof(Vertex),
                                       packed_vertices.size() * sizeof(Vertex));
    renderer->GetTriangleBuffer()->Write(packed_triangles.data(), m_triangle_count * sizeof(u32),
                                         packed_triangles.size() * sizeof(u32));
    renderer->GetMeshletBuffer()->Write(meshlets.data(), m_meshlet_count * sizeof(Meshlet),
                                        meshlets.size() * sizeof(Meshlet));

    m_vertex_count += new_mesh.vertex_count;
    m_triangle_count += new_mesh.triangle_count;
    m_meshlet_count += new_mesh.meshlet_count;

    return new_mesh;
}

Apex::Material Apex::ResourceManager::LoadMaterial(std::unordered_map<std::string, int> texture_index_map,
                                                   const aiMaterial* material) const
{
    Material mat{};

    aiColor4D albedo(1.f, 1.f, 1.f, 1.f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == AI_SUCCESS)
    {
        int r = meshopt_quantizeSnorm(albedo.r, 10);
        int g = meshopt_quantizeSnorm(albedo.g, 10);
        int b = meshopt_quantizeSnorm(albedo.b, 10);

        mat.albedo = (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20);
    }
    else
    {
        int r = meshopt_quantizeSnorm(1.f, 10);
        int g = meshopt_quantizeSnorm(1.f, 10);
        int b = meshopt_quantizeSnorm(1.f, 10);

        mat.albedo = (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20);
    }

    aiColor3D emissive(0.f, 0.f, 0.f);
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
    {
        int r = meshopt_quantizeSnorm(emissive.r, 10);
        int g = meshopt_quantizeSnorm(emissive.g, 10);
        int b = meshopt_quantizeSnorm(emissive.b, 10);

        mat.emissive = (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20);
    }
    else
    {
        int r = meshopt_quantizeSnorm(0.f, 10);
        int g = meshopt_quantizeSnorm(0.f, 10);
        int b = meshopt_quantizeSnorm(0.f, 10);

        mat.emissive = (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20);
    }

    float metallic = 1.0f;
    float roughness = 1.0f;
    material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

    u16 m = meshopt_quantizeHalf(metallic);
    u16 r = meshopt_quantizeHalf(roughness);

    mat.metal_rough = u32(m) | (u32(r) << 16);

    float ior = 1.5f;
    material->Get(AI_MATKEY_REFRACTI, ior);

    // Quantize to 16-bit half
    u16 o = meshopt_quantizeHalf(albedo.a);
    u16 i = meshopt_quantizeHalf(ior);

    mat.ior_opacity = u32(i) | (u32(o) << 16);

    int blend_func = 0;
    material->Get(AI_MATKEY_BLEND_FUNC, blend_func);

    u32 clearcoat = 0.0;
    material->Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoat);
    u32 clearcoat_roughness = 0.0;
    material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoat_roughness);
    mat.clear_coat = meshopt_quantizeHalf(clearcoat) | (meshopt_quantizeHalf(clearcoat_roughness) << 16);

    aiString alpha_mode_str;
    material->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode_str);

    const std::string mode = alpha_mode_str.C_Str();

    if (mode == "OPAQUE")
        mat.alpha_mode = AlphaMode::eOpaque;

    if (mode == "BLEND")
        mat.alpha_mode = AlphaMode::eBlend;

    if (mode == "MASK")
        mat.alpha_mode = AlphaMode::eMask;

    if (material->GetTextureCount(aiTextureType_CLEARCOAT) > 2)
        mat.flags |= FLAG_USE_CLEARCOAT_NORMAL_MAP;

    auto resolve_texture_or_dummy = [&](const aiTextureType type, Swift::ITextureView* dummy) -> int
    {
        aiString path;
        if (material->GetTexture(type, 0, &path) == AI_SUCCESS)
        {
            if (const auto it = texture_index_map.find(path.C_Str()); it != texture_index_map.end())
                return it->second;
        }

        return dummy ? dummy->GetDescriptorIndex() : -1;
    };
    auto resolve_indexed_texture_or_dummy =
        [&](aiTextureType type, unsigned int index, Swift::ITextureView* dummy) -> int
    {
        aiString path;
        if (material->GetTexture(type, index, &path) == AI_SUCCESS)
        {
            if (const auto it = texture_index_map.find(path.C_Str()); it != texture_index_map.end())
                return it->second;
        }

        return dummy ? dummy->GetDescriptorIndex() : -1;
    };

    mat.albedo_index = resolve_texture_or_dummy(aiTextureType_BASE_COLOR, m_dummy_white_texture_view);
    mat.emissive_index = resolve_texture_or_dummy(aiTextureType_EMISSIVE, m_dummy_black_texture_view);
    mat.metal_rough_index = resolve_texture_or_dummy(aiTextureType_METALNESS, m_dummy_white_texture_view);
    mat.normal_index = resolve_texture_or_dummy(aiTextureType_NORMALS, m_dummy_normal_texture_view);
    mat.occlusion_index = resolve_texture_or_dummy(aiTextureType_LIGHTMAP, m_dummy_white_texture_view);
    mat.clearcoat_index = resolve_texture_or_dummy(aiTextureType_CLEARCOAT, m_dummy_white_texture_view);
    mat.clearcoat_rough_index = resolve_indexed_texture_or_dummy(
        AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE,
        m_dummy_white_texture_view);
    mat.clearcoat_normal_index = resolve_indexed_texture_or_dummy(
        AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE,
        m_dummy_normal_texture_view);

    return mat;
}

Apex::Texture Apex::ResourceManager::LoadTexture(const aiTexture* texture)
{
    TextureCreateInfo create_info{};
    const std::string name = texture->mFilename.C_Str();

    if (texture->mHeight == 0)
    {
        int width, height, channels;
        stbi_uc* data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(texture->pcData),
            static_cast<int>(texture->mWidth),
            &width, &height, &channels, STBI_rgb_alpha);

        assert(data && "stb_image failed to decode embedded texture");

        create_info.width = static_cast<u32>(width);
        create_info.height = static_cast<u32>(height);
        create_info.format = Swift::Format::eRGBA8_UNORM;
        create_info.pixels.assign(data, data + width * height * 4);

        stbi_image_free(data);
    }
    else
    {
        create_info.width = texture->mWidth;
        create_info.height = texture->mHeight;
        create_info.format = Swift::Format::eRGBA8_UNORM;
        create_info.pixels.resize(texture->mWidth * texture->mHeight * 4);

        for (u32 i = 0; i < texture->mWidth * texture->mHeight; i++)
        {
            const aiTexel& texel = texture->pcData[i];
            create_info.pixels[i * 4 + 0] = texel.r;
            create_info.pixels[i * 4 + 1] = texel.g;
            create_info.pixels[i * 4 + 2] = texel.b;
            create_info.pixels[i * 4 + 3] = texel.a;
        }
    }
    return Texture(name, create_info);
}

int Apex::ResourceManager::LoadNodeTree(const aiNode* ainode, std::vector<Node>& nodes)
{
    Node node;
    node.m_name = ainode->mName.C_Str();
    node.m_transform = glm::transpose(glm::make_mat4(&ainode->mTransformation.a1));
    node.m_mesh_index = ainode->mNumMeshes > 0 ? static_cast<int>(ainode->mMeshes[0]) : -1;

    const int index = static_cast<int>(nodes.size());
    nodes.push_back(node);

    for (u32 i = 0; i < ainode->mNumChildren; i++)
    {
        int child_index = LoadNodeTree(ainode->mChildren[i], nodes);
        nodes[index].m_children.push_back(child_index);
    }

    return index;
}

glm::uvec2 Apex::ResourceManager::QuantizePosition(const glm::vec3 position)
{
    return glm::uvec2(meshopt_quantizeHalf(position.x) | (meshopt_quantizeHalf(position.y) << 16),
                      meshopt_quantizeHalf(position.z));
}

u32 Apex::ResourceManager::QuantizeNormal(const aiVector3D& normal)
{
    return ((meshopt_quantizeSnorm(normal.x, 10) & 1023) << 20) |
        ((meshopt_quantizeSnorm(normal.y, 10) & 1023) << 10) |
        (meshopt_quantizeSnorm(normal.z, 10) & 1023);
}

u32 Apex::ResourceManager::QuantizeUV(const aiVector3D& uv)
{
    return (meshopt_quantizeHalf(uv.x) | (meshopt_quantizeHalf(uv.y) << 16));
}
