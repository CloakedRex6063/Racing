#pragma once

#include "unordered_map"
#include "unordered_set"
#include "typeindex"
#include "memory"
#include "chrono"
#include "cstdio"
#include "vector"
#include "span"
#include "array"
#include "string"
#include "string_view"
#include "ranges"
#include "algorithm"
#include "filesystem"
#include "expected"
#include "fstream"

namespace fs = std::filesystem;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "swift.hpp"
#include "swift_builders.hpp"
#include "render_graph/swift_render_graph.hpp"
#include "imgui.h"
#include "ImGuizmo.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/GltfMaterial.h"
#include "meshoptimizer.h"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T>
using Scoped = std::unique_ptr<T>;

#define APEX_CONSTRUCT(T) T() = default;
#define APEX_NO_CONSTRUCT(T) T() = delete;
#define APEX_DESTRUCT(T) virtual ~T() = default;

#define APEX_NO_COPY(T)  \
T(const T&) = delete; \
T& operator=(const T&) = delete;

#define APEX_NO_MOVE(T) \
T(T&&) = delete;     \
T& operator=(T&&) = delete;

#define SWIFT_COPY(T)  \
T(const T&) = default; \
T& operator=(const T&) = default;

#define SWIFT_MOVE(T) \
T(T&&) = default;     \
T& operator=(T&&) = default;
