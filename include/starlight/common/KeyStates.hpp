#pragma once 

#include "Enums.hpp"

#include <map>

namespace star {
class KeyStates {
public:
	KeyStates() = default; 
	~KeyStates() = default; 

	/// <summary>
	/// Get the current state of a key. 
	/// </summary>
	/// <param name="key">Target key</param>
	static bool state(KEY key) {
		return states[key];
	}
protected:
	/// <summary>
	/// Mark key as pressed
	/// </summary>
	/// <param name="key">Target key</param>
	static void press(KEY key) {
		states[key] = true;
	}

	/// <summary>
	/// Mark key as released.
	/// </summary>
	/// <param name="key">Target key</param>
	static void release(KEY key) {
		states[key] = false;
	}

private:
	static std::map<KEY, bool> states;
	
};
}