#ifndef INPUT_HPP
#define INPUT_HPP

#include <SDL_scancode.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <SDL_video.h>
#include "list.hpp"
#include "math.hpp"

struct TransientGamepadState {
    bool buttonDown[SDL_CONTROLLER_BUTTON_MAX]; //15
    bool buttonUp[SDL_CONTROLLER_BUTTON_MAX]; //15
};

struct Gamepad {
    SDL_Joystick * joy;
    SDL_GameController * con;
    SDL_JoystickID id;
    float axis[SDL_CONTROLLER_AXIS_MAX]; //6
    bool buttonHeld[SDL_CONTROLLER_BUTTON_MAX]; //15
    TransientGamepadState tick, frame;
    Vec2 leftAnalog, rightAnalog; //pre-deadzoned-and-clamped
    //TODO: handle default logic for treating triggers as buttons?
    //TODO: handle analog->discrete conversion for analog sticks (for menus, etc.)
};

//NOTES for correct usage of this data:
//- up and down flags get cleared at the end of every tick, even if events are handled once per frame
//- up and down flags can both be set for the same key/button in a given tick.
//- when in doubt, put code that handles DOWN flags before code that handles UP flags for the same key/button
struct TransientInputState {
    bool keyDown[SDL_NUM_SCANCODES]; //512
    bool keyRepeat[SDL_NUM_SCANCODES]; //512
    bool keyUp[SDL_NUM_SCANCODES]; //512
    bool leftMouseDown, leftMouseUp, rightMouseDown, rightMouseUp, middleMouseDown, middleMouseUp;
    Coord2 mouseMotion;
    int scrollUnits;
};

struct InputState {
    bool keyHeld[SDL_NUM_SCANCODES]; //512
    TransientInputState tick, frame;
    bool leftMouseHeld, rightMouseHeld, middleMouseHeld;
    Coord2 mousePosition;

    Gamepad gamepad; //aggregates inputs from all currently connected gamepads
    List<Gamepad> gamepads;
};

bool process_events(InputState & input, SDL_Window * window);
void reset_tick_transient_state(InputState & input);
void reset_frame_transient_state(InputState & input);

Coord2 SDL_GetMousePosition(SDL_Window * window);

#endif //INPUT_HPP
