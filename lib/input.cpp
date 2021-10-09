#include <SDL.h>
#include <stdio.h>

#include "input.hpp"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "trace.hpp"
#include "types.hpp"

//NOTE: We use SDL_GetGlobalMouseState() because SDL_GetMouseState() updates at roughly half of refresh rate on windows.
//      The same bug exists on macOS, but for both versions of the function. So mouse is laggy on mac no matter what :(
 Coord2 SDL_GetMousePosition(SDL_Window * window) {
    int mx, my;
    SDL_GetGlobalMouseState(&mx, &my);
    int wx, wy;
    SDL_GetWindowPosition(window, &wx, &wy);
    return coord2(mx - wx, my - wy);
}

static inline Vec2 deadzone(Vec2 v, float d) {
    return noz(v) * clamp(0, (len(v) - d) / (1 - d), 1);
}

bool process_events(InputState & in, SDL_Window * window) { TimeFunc
    bool shouldExit = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) { TimeScope("event iter")
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            shouldExit = true;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            SDL_Scancode scancode = event.key.keysym.scancode;
            if (ImGui::GetIO().WantCaptureKeyboard) {
                in.keyHeld    [scancode] &= event.type == SDL_KEYDOWN;
                in.tick .keyUp[scancode] |= event.type == SDL_KEYUP;
                in.frame.keyUp[scancode] |= event.type == SDL_KEYUP;
            } else {
                in.keyHeld        [scancode]  = event.type == SDL_KEYDOWN;
                in.tick .keyDown  [scancode] |= event.type == SDL_KEYDOWN && !event.key.repeat;
                in.tick .keyRepeat[scancode] |= event.type == SDL_KEYDOWN;
                in.tick .keyUp    [scancode] |= event.type == SDL_KEYUP;
                in.frame.keyDown  [scancode] |= event.type == SDL_KEYDOWN && !event.key.repeat;
                in.frame.keyRepeat[scancode] |= event.type == SDL_KEYDOWN;
                in.frame.keyUp    [scancode] |= event.type == SDL_KEYUP;
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN && !ImGui::GetIO().WantCaptureMouse) {
            switch (event.button.button) {
                case SDL_BUTTON_LEFT  : in.  leftMouseHeld = in.tick.  leftMouseDown = in.frame  .leftMouseDown = true; break;
                case SDL_BUTTON_RIGHT : in. rightMouseHeld = in.tick. rightMouseDown = in.frame .rightMouseDown = true; break;
                case SDL_BUTTON_MIDDLE: in.middleMouseHeld = in.tick.middleMouseDown = in.frame.middleMouseDown = true; break;
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            switch (event.button.button) {
                case SDL_BUTTON_LEFT  : in.  leftMouseHeld = !(in.tick.  leftMouseUp = in.frame.  leftMouseUp = true); break;
                case SDL_BUTTON_RIGHT : in. rightMouseHeld = !(in.tick. rightMouseUp = in.frame. rightMouseUp = true); break;
                case SDL_BUTTON_MIDDLE: in.middleMouseHeld = !(in.tick.middleMouseUp = in.frame.middleMouseUp = true); break;
            }
        } else if (event.type == SDL_MOUSEWHEEL && !ImGui::GetIO().WantCaptureMouse) {
            in.tick .scrollUnits += event.wheel.y;
            in.frame.scrollUnits += event.wheel.y;
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            Gamepad pad = {};
            pad.joy = SDL_JoystickOpen(event.cdevice.which);
            pad.con = SDL_GameControllerOpen(event.cdevice.which);
            pad.id = SDL_JoystickInstanceID(pad.joy);
            in.gamepads.add(pad);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            //find gamepad with this ID
            for (int i = 0; i < in.gamepads.len; ++i) {
                if (in.gamepads[i].id == event.cdevice.which) {
                    SDL_GameControllerClose(in.gamepads[i].con);
                    SDL_JoystickClose(in.gamepads[i].joy);

                    //remove controller from list
                    in.gamepads.remove(i);
                    break;
                }
            }
        } else if (event.type == SDL_CONTROLLERAXISMOTION) {
            for (int i = 0; i < in.gamepads.len; ++i) {
                if (in.gamepads[i].id == event.caxis.which) {
                    in.gamepads[i].axis[event.caxis.axis] = fmax(event.caxis.value / 32767.0f, -1);
                    const char * name = SDL_GameControllerName(in.gamepads[i].con);
                    //Xbox 360 controllers: bias away from the sticky axes
                    //NOTE: The Baker reports that his XBox 360 analog sticks feel biased towards the axes.
                    //      This was described as a really bad bias that made the analog lance feel really bad.
                    //      So this is some code to bias *away* from the axes if you are using analog sticks.
                    bool isXbox360 = (strstr(name, "x360") ||
                                      strstr(name, "X360") ||
                                      strstr(name, "x 360") ||
                                      strstr(name, "X 360"));
                    if (isXbox360 && (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX  ||
                                      event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY  ||
                                      event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX ||
                                      event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY)) {
                        float & v = in.gamepads[i].axis[event.caxis.axis];
                        v *= 2 - fabsf(v);
                    }
                    break;
                }
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            for (int i = 0; i < in.gamepads.len; ++i) {
                if (in.gamepads[i].id == event.cbutton.which) {
                    in.gamepads[i].buttonHeld[event.cbutton.button] = event.cbutton.state;
                    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                        in.gamepads[i].tick .buttonDown[event.cbutton.button] = true;
                        in.gamepads[i].frame.buttonDown[event.cbutton.button] = true;
                    } else {
                        in.gamepads[i].tick .buttonUp[event.cbutton.button] = true;
                        in.gamepads[i].frame.buttonUp[event.cbutton.button] = true;
                    }
                    break;
                }
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            //NOTE: We need to use mouse motion events for this in order to correctly handle relative mouse modes.
            //      It seems to be just as responsive as SDL_GetMousePosition() on windows in relative mouse mode.
            //      I don't remember what the problems were before. Maybe they only applied to absolute mode?
            in.tick .mouseMotion.x += event.motion.xrel;
            in.tick .mouseMotion.y += event.motion.yrel;
            in.frame.mouseMotion.x += event.motion.xrel;
            in.frame.mouseMotion.y += event.motion.yrel;
        }
    }

    //handle mouse positioning using SDL_GetMousePosition() because it's more responsive than events on Windows
    in.mousePosition = SDL_GetMousePosition(window);

    //deadzone and clamp gamepad analog controls
    for (Gamepad & pad : in.gamepads) {
        const static float D = 0.3f;
        pad.leftAnalog = deadzone(vec2(pad.axis[SDL_CONTROLLER_AXIS_LEFTX], pad.axis[SDL_CONTROLLER_AXIS_LEFTY]), D);
        pad.rightAnalog = deadzone(vec2(pad.axis[SDL_CONTROLLER_AXIS_RIGHTX], pad.axis[SDL_CONTROLLER_AXIS_RIGHTY]), D);
    }

    //update main gamepad
    //TODO: allow selecting a preferred gamepad (by ID?)
    //TODO: correctly handle sending button up/down events when disconnecting/changing active gamepad
    in.gamepad = in.gamepads.len? in.gamepads[0] : Gamepad {};

    return shouldExit;
}

void reset_tick_transient_state(InputState & input) {
    input.tick = {};
    input.gamepad.tick = {};
    for (Gamepad & pad : input.gamepads) {
        pad.tick = {};
    }
}

void reset_frame_transient_state(InputState & input) {
    input.frame = {};
    input.gamepad.frame = {};
    for (Gamepad & pad : input.gamepads) {
        pad.frame = {};
    }
}
