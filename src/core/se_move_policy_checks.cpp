#include "engine/app/app.hpp"
#include "engine/editor/imgui_manager.hpp"
#include "engine/platform/se_input_system.hpp"
#include "engine/platform/se_window.hpp"
#include "engine/renderer/se_cubemap.hpp"
#include "engine/renderer/se_cubemap_brdf.h"
#include "engine/renderer/se_cubemap_diffuse.h"
#include "engine/renderer/se_cubemap_specular.h"
#include "engine/renderer/se_hdr_to_cubemap.h"
#include "engine/renderer/se_material_system.hpp"
#include "engine/renderer/se_mesh.hpp"
#include "engine/renderer/se_mesh_system.hpp"
#include "engine/renderer/se_offscreen_renderer.hpp"
#include "engine/renderer/se_particle_system.hpp"
#include "engine/renderer/se_pbr.hpp"
#include "engine/renderer/se_pbr_material.hpp"
#include "engine/renderer/se_pipeline.hpp"
#include "engine/renderer/se_postprocess.hpp"
#include "engine/renderer/se_renderer.hpp"
#include "engine/renderer/se_submesh.hpp"
#include "engine/renderer/se_swap_chain.hpp"
#include "engine/renderer/se_texture.hpp"
#include "engine/renderer/se_texture_system.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/scene/se_gameobject.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/scene/se_scene_manager.hpp"
#include "engine/scripting/se_script_manager.hpp"
#include "engine/vulkan/se_device.hpp"

#include <type_traits>

#define SE_ASSERT_IMMOVABLE(Type) \
    static_assert(!std::is_copy_constructible_v<Type>); \
    static_assert(!std::is_copy_assignable_v<Type>); \
    static_assert(!std::is_move_constructible_v<Type>); \
    static_assert(!std::is_move_assignable_v<Type>)

#define SE_ASSERT_MOVE_CONSTRUCTIBLE_ONLY(Type) \
    static_assert(!std::is_copy_constructible_v<Type>); \
    static_assert(!std::is_copy_assignable_v<Type>); \
    static_assert(std::is_nothrow_move_constructible_v<Type>); \
    static_assert(!std::is_move_assignable_v<Type>)

namespace
{
    // App-wired services and pointer-stable scene/resource objects.
    SE_ASSERT_IMMOVABLE(App);
    SE_ASSERT_IMMOVABLE(se::SEWindow);
    SE_ASSERT_IMMOVABLE(se::SEInputSystem);
    SE_ASSERT_IMMOVABLE(se::SEDevice);
    SE_ASSERT_IMMOVABLE(se::SERenderer);
    SE_ASSERT_IMMOVABLE(se::SEOffscreenRenderer);
    SE_ASSERT_IMMOVABLE(se::SECubemap);
    SE_ASSERT_IMMOVABLE(se::PBR);
    SE_ASSERT_IMMOVABLE(se::ParticleSystem);
    SE_ASSERT_IMMOVABLE(se::SEPostProcess);
    SE_ASSERT_IMMOVABLE(se::MaterialSystem);
    SE_ASSERT_IMMOVABLE(se::TextureSystem);
    SE_ASSERT_IMMOVABLE(se::MeshSystem);
    SE_ASSERT_IMMOVABLE(se::ResourceManager);
    SE_ASSERT_IMMOVABLE(se::SceneManager);
    SE_ASSERT_IMMOVABLE(se::ImGuiManager);
    SE_ASSERT_IMMOVABLE(se::ScriptManager);
    SE_ASSERT_IMMOVABLE(se::Scene);
    SE_ASSERT_IMMOVABLE(se::SEGameObject);
    SE_ASSERT_IMMOVABLE(se::SEMesh);
    SE_ASSERT_IMMOVABLE(se::SESubMesh);
    SE_ASSERT_IMMOVABLE(se::SETexture);
    SE_ASSERT_IMMOVABLE(se::SEMaterial);

    // Single-owner immovable exceptions. SESwapChain is held by SERenderer;
    // the previous chain is shared only transiently while a replacement is built.
    // The four cubemap helpers are private SECubemap-owned subobjects with no
    // escaping object pointers.
    SE_ASSERT_IMMOVABLE(se::SESwapChain);
    SE_ASSERT_IMMOVABLE(se::SEHdrToCubemap);
    SE_ASSERT_IMMOVABLE(se::SECubemapDiffuse);
    SE_ASSERT_IMMOVABLE(se::SECubemapSpecular);
    SE_ASSERT_IMMOVABLE(se::SECubemapBRDF);

    // Leaf handle owners transfer their handles on nothrow move construction.
    SE_ASSERT_MOVE_CONSTRUCTIBLE_ONLY(se::SEPipeline);
    static_assert(!std::is_copy_constructible_v<se::Buffer>);
    static_assert(!std::is_copy_assignable_v<se::Buffer>);
    static_assert(std::is_nothrow_move_constructible_v<se::Buffer>);
    static_assert(std::is_nothrow_move_assignable_v<se::Buffer>);
    static_assert(!std::is_copy_constructible_v<se::ParticleBuffer>);
    static_assert(!std::is_copy_assignable_v<se::ParticleBuffer>);
    static_assert(std::is_nothrow_move_constructible_v<se::ParticleBuffer>);
    static_assert(std::is_nothrow_move_assignable_v<se::ParticleBuffer>);
}

#undef SE_ASSERT_MOVE_CONSTRUCTIBLE_ONLY
#undef SE_ASSERT_IMMOVABLE
