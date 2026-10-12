#ifndef __ZELDA_RENDER_H__
#define __ZELDA_RENDER_H__

#include "common/rt64_user_configuration.h"
#include "ultramodern/renderer_context.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace RT64 {
    struct Application;
}

namespace zelda64 {
    namespace renderer {
        class RT64Context : public ultramodern::renderer::RendererContext {
        public:
            ~RT64Context() override;
            RT64Context(uint8_t *rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

            bool valid() override { return static_cast<bool>(app); }

            bool update_config(const ultramodern::renderer::GraphicsConfig &old_config, const ultramodern::renderer::GraphicsConfig &new_config) override;

            void enable_instant_present() override;
            void send_dl(const OSTask *task) override;
            void update_screen(uint32_t vi_origin) override;
            void shutdown() override;
            uint32_t get_display_framerate() const override;
            float get_resolution_scale() const override;

        protected:
            std::unique_ptr<RT64::Application> app;
        };

        std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(uint8_t *rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

        struct ModPackInfo {
            std::string mod_id;
            std::string display_name;
            std::string short_description;
            std::string description;
            std::string version;
            std::string authors;           // "Authors: a, b"
            std::string thumb;             // absolute path to a cached .tga, or empty
            bool enabled;
        };
        // Texture packs (.rtz) managed by librecomp::mods. Defined in main.cpp.
        std::vector<ModPackInfo> get_texture_packs();
        void set_texture_pack_enabled(const std::string &mod_id, bool enabled);
        void rescan_texture_packs();
        std::filesystem::path get_texture_pack_dir();
        std::vector<std::filesystem::path> get_enabled_texture_pack_paths();
        // Defined in rt64_render_context.cpp: flags that the enabled set changed; RT64 applies it on its own thread.
        void mark_texture_packs_dirty();
        void init_texture_packs();

        RT64::UserConfiguration::Antialiasing RT64MaxMSAA();
        bool RT64SamplePositionsSupported();
        bool RT64HighPrecisionFBEnabled();
    }
}

#endif
