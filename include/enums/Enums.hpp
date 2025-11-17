#pragma once

#include <GLFW/glfw3.h>

namespace star {
    enum Command_Buffer_Order {
        before_render_pass = 0,
        main_render_pass = 1,   //special only usable by main render pass
        after_render_pass = 2,
        end_of_frame = 3, 
        after_presentation = 4,
        presentation = 5        //special only usable by presentation
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
        resolution_y
    };

    enum KEY {
        A = GLFW_KEY_A,
        B = GLFW_KEY_B,
        C = GLFW_KEY_C,
        D = GLFW_KEY_D,
        E = GLFW_KEY_E,
        F = GLFW_KEY_F,
        G = GLFW_KEY_G,
        H = GLFW_KEY_H,
        I = GLFW_KEY_I,
        J = GLFW_KEY_J,
        K = GLFW_KEY_K,
        L = GLFW_KEY_L,
        M = GLFW_KEY_M,
        N = GLFW_KEY_N,
        O = GLFW_KEY_O,
        P = GLFW_KEY_P,
        Q = GLFW_KEY_Q,
        R = GLFW_KEY_R,
        S = GLFW_KEY_S,
        T = GLFW_KEY_T,
        U = GLFW_KEY_U,
        V = GLFW_KEY_V,
        W = GLFW_KEY_W,
        X = GLFW_KEY_X,
        Y = GLFW_KEY_Y,
        Z = GLFW_KEY_Z,
        F12 = GLFW_KEY_F12,
        RIGHT = GLFW_KEY_RIGHT,
        LEFT = GLFW_KEY_LEFT,
        UP = GLFW_KEY_UP,
        DOWN = GLFW_KEY_DOWN,
        SPACE = GLFW_KEY_SPACE
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