#pragma once 
#include "Camera.hpp"
#include "Interactivity.hpp"
#include "KeyStates.hpp"
#include "Time.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>

namespace star {
	/// <summary>
	/// Camera with default controls
	/// </summary>
	class CameraController :
		public Camera,
		public Interactivity
	{
	public:
		CameraController(); 
		~CameraController() = default; 

		/// <summary>
		/// Key callback for camera object. Implements default controls for the camera. 
		/// </summary>
		/// <param name="key"></param>
		/// <param name="scancode"></param>
		/// <param name="action"></param>
		/// <param name="mods"></param>
		virtual void onKeyPress(int key, int scancode, int mods) override;

		virtual void onKeyRelease(int key, int scancode, int mods) override; 

		/// <summary>
		/// Mouse callback for camera objects. Implements default controls for the camera. 
		/// </summary>
		/// <param name="xpos"></param>
		/// <param name="ypos"></param>
		virtual void onMouseMovement(double xpos, double ypos) override;

		/// <summary>
		/// Mouse button callback for camera object. 
		/// </summary>
		/// <param name="button"></param>
		/// <param name="action"></param>
		/// <param name="mods"></param>
		virtual void onMouseButtonAction(int button, int action, int mods) override;

		/// <summary>
		/// Update camera locations as needed 
		/// </summary>
		virtual void onWorldUpdate() override;

		void onScroll(double xoffset, double yoffset) override {};

	protected:

	private:
		Time time = Time();

		const float sensitivity = 0.1f;
		bool init = false;

		//previous mouse coordinates from GLFW
		float prevX, prevY, xMovement, yMovement;

		//control information for camera 
		float pitch = -0.f, yaw = -90.0f;

		bool click = false;
	};
}