/*
PREP TODOs:
- pull in useful snippets from lance?
- window display + position settings?
- super basic audio already-in-place since that tends to get neglected
- actually test new binary serialization code?
- some generic editor stuff if I have time?
- screenshot saving???
*/

#include "level.hpp"
#include "settings.hpp"
#include "gui.hpp"
#include "common.hpp"
#include "glutil.hpp"
#include "platform.hpp"
#include "trace.hpp"
#include "imm.hpp"
#include "input.hpp"
#include "msf_resample.h"
#include "msf_gif.h"
#include "pixel.hpp"
#include "graphics.hpp"

#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AUDIO                                                                                                            ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static SoLoud::Soloud loud;
static SoLoud::WavStream music_test;
static SoLoud::Wav sfx_test;

static const int NUM_STEP_SOUNDS = 7;
static SoLoud::Wav stepSounds[NUM_STEP_SOUNDS];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MAIN FUNCTION                                                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#undef main
#endif

int main(int argc, char ** argv) {
    init_profiling_trace();

    #ifdef _WIN32
        //SDL2 doesn't call SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
        //for us even though we pass the SDL_WINDOW_ALLOW_HIGHDPI flag, so we have to do it ourselves
        handle_dpi_awareness();
    #endif
        //set the current working directory to the folder containing the game's executable
        char * basePath = SDL_GetBasePath();
        if (basePath) {
            set_current_working_directory(basePath);
            SDL_free(basePath);
        }

        //ensure we are compiling and linking against the same SDL version
        //TODO: this may break due to SDL dynlib funkiness
        // { SDL_version version; SDL_GetVersion(&version); assert(version.patch == SDL_PATCHLEVEL); }

        //initialize timer and startup SDL
        TimeLine("SDL init")
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO)) {
            printf("SDL FAILED TO INIT: %s\n", SDL_GetError());
            exit(1);
        }
    print_log("[] SDL init: %f seconds\n", get_time());
        const int canvasWidth = 420;
        const int canvasHeight = 240;
        const int pixelScale = 3;
        const int windowWidth = canvasWidth * pixelScale;
        const int windowHeight = canvasHeight * pixelScale;
        const int windowDisplay = 0;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

        //we use 4 multisamples because the opengl spec guarantees it as the minimum that applications must support
        //and we have found in practice that many devices do run into problems at higher sample counts
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

        SDL_Window * window;
        TimeLine("CreateWindow") window = SDL_CreateWindow("GameJam+",
            SDL_WINDOWPOS_CENTERED_DISPLAY(0),
            SDL_WINDOWPOS_CENTERED_DISPLAY(0), windowWidth, windowHeight,
            SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (window == nullptr) {
            printf("SDL FAILED TO CREATE WINDOW: %s\n", SDL_GetError());
            return 1;
        }

        TimeLine("CreateContext") assert(SDL_GL_CreateContext(window));
        TimeLine("glad") if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
            printf("failed to load GLAD\n");
            exit(1);
        }

        SDL_GL_SetSwapInterval(1);
    print_log("[] SDL create window: %f seconds\n", get_time());
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
    print_log("[] dear imgui init: %f seconds\n", get_time());
        Imm imm = {};
        TimeLine("Imm.init") imm.init(500);
        TimeLine("load_font") imm.font = load_font("res/nova.fnt");

        uint blitShader = create_program_from_files("res/blit.vert", "res/blit.frag");
        Canvas canvas = make_canvas(canvasWidth, canvasHeight, 16);
        Graphics graphics = load_graphics();
        MonoFont font = load_mono_font("res/font-16-white.png", 8, 16);
    print_log("[] graphics init: %f seconds\n", get_time());
        settings.load();
        Level level = {};
    print_log("[] level init: %f seconds\n", get_time());
        TimeLine("SoLoud init") if (int err = loud.init(); err) printf("soloud init error: %d\n", err);
    print_log("[] soloud init: %f seconds\n", get_time());
        //TODO: audio
        // TimeLine("music_test") music_test.load("res/bg1.ogg");
        // TimeLine("sfx_test") sfx_test.load("res/sfx1.wav");
        // music_test.setLooping(true);
        // int musicHandle = loud.play(music_test, settings.musicVolume);
        loud.setGlobalVolume(settings.sfxVolume);
    print_log("[] audio init: %f seconds\n", get_time());
        float frameTimes[100] = {};
        float accumulator = 0;
        double lastTime = 0;

        InputState input = {};
        // bool leftTriggerHeld = false, rightTriggerHeld = false;
        #define HELD(X) (input.keyHeld[SDL_SCANCODE_ ## X])
        #define FRAME_DOWN(X) (input.frame.keyDown[SDL_SCANCODE_ ## X])
        #define FRAME_REPEAT(X) (input.frame.keyRepeat[SDL_SCANCODE_ ## X])
        #define FRAME_UP(X) (input.frame.keyUp[SDL_SCANCODE_ ## X])

        //undo system data
        // List<Level> undoStack = {};
        // List<Level> redoStack = {};

        const int gifCentiseconds = 4;
        bool giffing = false;
        MsfGifState gifState = {};
        float gifTimer = 0;

        gl_error("program init");
    print_log("[] done initializing: %f seconds\n", get_time());

    const float tickLength = 1.0f/240;
    float gameTime = 0;
    float gameSpeed = 1.0f;
    float tickSpeed = 1.0f;

    bool shouldExit = false;
    bool fullscreen = false;
    int frameCount = 0;
    while (!shouldExit) { TimeScope("frame loop")
        double preWholeFrameTime = get_time();

        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        int gameDisplay = SDL_GetWindowDisplayIndex(window);

        shouldExit |= process_events(input, window);

        //update timestep
        double thisTime = get_time();
        float dt = thisTime - lastTime;
        assert(dt > 0);
        lastTime = thisTime;
        accumulator = fminf(0.5f, accumulator + dt);
        gifTimer += dt;

        //update
        float gspeed = gameSpeed, tspeed = tickSpeed;
        // if (mode == MODE_EDIT) gspeed = tspeed = 1;
        if (HELD(LGUI) || HELD(LCTRL) || HELD(RSHIFT)) gspeed *= 0.1f;
        if (HELD(LSHIFT)) gspeed *= 5.0f;
        //NOTE: we limit the maximum number of ticks per frame to avoid spinlocking
        for (int _tcount = 0; accumulator > tickLength / tspeed && _tcount < 50; ++_tcount) { TimeScope("tick loop")
        #define TICK_DOWN(X) (input.tick.keyDown[SDL_SCANCODE_ ## X])
        #define TICK_REPEAT(X) (input.tick.keyRepeat[SDL_SCANCODE_ ## X])
        #define TICK_UP(X) (input.tick.keyUp[SDL_SCANCODE_ ## X])
            float tick = tickLength * gspeed;
            accumulator -= tickLength / tspeed;

            //handle fullscreen toggle
            if (TICK_DOWN(RETURN) && (HELD(LALT) || HELD(RALT))) {
                fullscreen = !fullscreen;
                if (fullscreen) {
                    SDL_DisplayMode desk, mode;
                    assert(!SDL_GetDesktopDisplayMode(gameDisplay, &desk));
                    assert(SDL_GetClosestDisplayMode(gameDisplay, &desk, &mode));
                    SDL_SetWindowDisplayMode(window, &mode);
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                } else {
                    SDL_SetWindowFullscreen(window, 0);
                }
            }

            //toggle gif recording
            if (TICK_DOWN(G) && (HELD(LGUI) || HELD(RGUI) || HELD(LCTRL) || HELD(RCTRL))) {
                giffing = !giffing;
                if (giffing) {
                    //determine actual canvas size and scale factor
                    int bufferWidth, bufferHeight;
                    SDL_GL_GetDrawableSize(window, &bufferWidth, &bufferHeight);
                    float maxWidth = 480, maxHeight = 320;
                    float scale = fminf(1, fminf(maxWidth / bufferWidth, maxHeight / bufferHeight));
                    msf_gif_begin(&gifState, bufferWidth * scale, bufferHeight * scale);
                    gifTimer = 0;
                } else {
                    MsfGifResult result = msf_gif_end(&gifState);
                    FILE * fp = fopen("out.gif", "wb");
                    fwrite(result.data, result.dataSize, 1, fp);
                    fclose(fp);
                    msf_gif_free(result);
                }
            }



            //game update logic goes here



            reset_tick_transient_state(input);
        #undef TICK_DOWN
        #undef TICK_REPEAT
        #undef TICK_UP
        }

        //update sliding window filter for framerate
        float timeSum = 0;
        for (int i = 1; i < ARR_SIZE(frameTimes); ++i) {
            frameTimes[i - 1] = frameTimes[i];
            timeSum += frameTimes[i - 1];
        }
        frameTimes[ARR_SIZE(frameTimes) - 1] = dt;
        timeSum += dt;

        //NOTE: We need to do this because glViewport() isn't called for us
        //      when the window is resized on Windows, even though it is on macOS
        int bufferWidth, bufferHeight;
        SDL_GL_GetDrawableSize(window, &bufferWidth, &bufferHeight);
        glViewport(0, 0, bufferWidth, bufferHeight);

        //set gl state
        glEnable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        //clear screen
        glClearColor(0.01, 0.01, 0.01, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //clear canvas
        for (int y = 0; y < canvas.height; ++y) {
            for (int x = 0; x < canvas.width; ++x) {
                canvas[y][x] = { (u8) (y + frameCount), (u8) (x + frameCount), 255, 255 };
            }
        }

        //pixel art rendering and such goes here
        //draw player
        draw_sprite(canvas, graphics.player, 0, 0);

        draw_canvas(blitShader, canvas, bufferWidth, bufferHeight);



        //TODO: sprite rendering and such goes here



        glClear(GL_DEPTH_BUFFER_BIT);
        imm.begin(windowWidth, windowHeight);

        //TODO: text + shape related rendering (via `imm`) goes here

        imm.end();



        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //TODO: imgui debug/editor stuff goes here

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());



        //gif rendering
        //TODO: pull straight from the canvas
        if (giffing && gifTimer > gifCentiseconds / 100.0f) {
            //NOTE: Because underlying canvas size can apparently change between the previous call to
            //      SDL_GL_GetDrawableSize() and now if the window is resized, we need to call it again.
            int bufferWidth, bufferHeight;
            SDL_GL_GetDrawableSize(window, &bufferWidth, &bufferHeight);
            u8 * src = (u8 *) malloc(bufferWidth * bufferHeight * sizeof(Pixel));
            TimeLine("glReadPixels") glReadPixels(0, 0, bufferWidth, bufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, src);
            u8 * dst = (u8 *) malloc(gifState.width * gifState.height * sizeof(Pixel));
            TimeLine("msf_resize") msf_resample(src, bufferWidth, bufferHeight, dst, gifState.width, gifState.height);
            free(src);
            TimeLine("msf_gif_frame") msf_gif_frame(&gifState, dst, gifCentiseconds, 15, -gifState.width * 4);
            free(dst);

            gifTimer -= gifCentiseconds / 100.0f;
        }

        reset_frame_transient_state(input);

        gl_error("after everything");
        if (get_time() - preWholeFrameTime > 0.004f) trace_instant_event("frame >4ms");
        TimeLine("buffer swap") SDL_GL_SwapWindow(window);
        fflush(stdout);
        fflush(stderr);
        frameCount += 1;

        //limit framerate when window is not focused
        u32 flags = SDL_GetWindowFlags(window);
        if (!(flags & SDL_WINDOW_INPUT_FOCUS)) {
            SDL_Delay(30);
        }

        //uncomment this to make the game exit immediately (good for testing compile+load times)
        // if (frameCount > 5) shouldExit = true;
    }

    printf("[] exiting game normally at %f seconds\n", get_time()); fflush(stdout);
    return 0;
}
