#pragma once

namespace star {
    enum Command_Buffer_Order {
        before_render_pass = 0,
        main_render_pass = 1,   //special only usable by main render pass
        after_render_pass = 2,
        end_of_frame = 3, 
        presentation = 4        //special only usable by presentation
    };

    enum Command_Buffer_Order_Index {
        dont_care = 0, 
        first = 1, 
        second = 2, 
		third = 3,
        fourth = 4, 
		fifth = 5
    };

    enum Rendering_Features {
		shader_float64
    };

    enum class Rendering_Device_Features{
        timeline_semaphores
    };

    enum Buffer_Type {
        update_once_per_frame, 
        once, 
        geometry_vertex, 
        geometry_index
    };

    enum Queue_Type {
        Tgraphics = 0, 
        Ttransfer = 1, 
        Tcompute = 2,
        Tpresent = 3
    };

    enum Shader_File_Type {
        spirv,
        glsl
    };

    enum class Shader_Stage {
        none,
        vertex,
        fragment,
        compute, 
        geometry
    };

    enum Config_Settings {
        app_name,
        mediadirectory,
        texture_filtering,
        texture_anisotropy,
        frames_in_flight, 
		required_device_feature_shader_float64,
        resolution_x, 
        resolution_y,
        tmp_directory
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