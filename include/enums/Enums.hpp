#pragma once

namespace star {
    enum Rendering_Features {
        geometry_shader
    };

    enum Buffer_Type {
        update_once_per_frame, 
        once, 
        geometry_vertex, 
        geometry_index
    };

    enum Command_Buffer_Type {
        Tgraphics, 
        Ttransfer, 
        Tcompute
    };

    enum Command_Execution_Order {
        before_main_graphics, 
        after_main_graphics,
        dont_care
    };

    enum Shader_File_Type {
        spirv,
        glsl
    };

    enum Shader_Stage {
        vertex,
        fragment,
        compute, 
        geometry
    };

    enum Config_Settings {
        mediadirectory
    };

    enum Handle_Type {
        null,
        defaultHandle,
        shader,
        object,
        texture,
        material,
        light,
        map,
        buffer
    };

    enum Buttons {
        STAR_W,
        STAR_A,
        STAR_S,
        STAR_D,
        STAR_CLICK_LEFT,
        STAR_CLICK_RIGHT,
        STAR_UP,
        STAR_DOWN,
        STAR_LEFT,
        STAR_RIGHT
    };

    namespace Type {
        enum Entity {
            model,
            light
        };

        enum Light {
            point,
            directional,
            spot
        };

        enum Axis {
            x,
            y,
            z
        };
    }

    namespace Render_Settings {

        enum Material {
            none,
            ambient,
            diffuse,
            specular,
            bumpMap
        };

        enum Feature {
            bumpMapping
        };
    }
}