#pragma once

#include <GLFW/glfw3.h>

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