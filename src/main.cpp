/*
PREP TODOs:
- pull in useful snippets from lance?
- window display + position settings?
- super basic audio already-in-place since that tends to get neglected
- actually test new binary serialization code?
- some generic editor stuff if I have time?
- screenshot saving???

GAMEPLAY TODOS:
- fix up sections
- player + enemy tuning
- audio
- tutorial text
- collision detect against enemies?
- enemy multiplication?
- enemy physics???
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

struct DebugToggleData { bool * b; const char * s1; const char * s2; };
static List<DebugToggleData> debugToggles = {};

inline static bool debug_toggle_impl(bool & b, bool c, const char * s1, const char * s2) {
    if (c && !ImGui::GetIO().WantCaptureKeyboard) {
#if !CONFIG_RELEASE
        b = !b;
        print_log("%s = %s\n", s1, b? "true" : "false");
#endif
    }

    bool alreadyAdded = false;
    for (DebugToggleData & toggle : debugToggles) {
        if (toggle.b == &b) { //should this comparison include the strings (instead)? idk
            alreadyAdded = true;
            break;
        }
    }
    if (!alreadyAdded) {
        debugToggles.add({ &b, s1, s2 });
    }

    return c;
};
#define DEBUG_TOGGLE(name, cond) debug_toggle_impl(name, cond, #name, #cond)

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

#include <time.h>

int main(int argc, char ** argv) {
    init_profiling_trace();
    global_pcg_state = time(NULL);

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

        //initialize timer and startup SDL
        TimeLine("SDL init")
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO)) {
            printf("SDL FAILED TO INIT: %s\n", SDL_GetError());
            exit(1);
        }
    print_log("[] SDL init: %f seconds\n", get_time());
        const int canvasWidth = 640;
        const int canvasHeight = 360;
        const int pixelScale = 2;
        const int windowWidth = canvasWidth * pixelScale;
        const int windowHeight = canvasHeight * pixelScale;
        // const int windowDisplay = 0;

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
        Level level = init_level();
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
        double lastTime = get_time();

        InputState input = {};
        // bool leftTriggerHeld = false, rightTriggerHeld = false;
        #define HELD(X) (input.keyHeld[SDL_SCANCODE_ ## X])
        #define FRAME_DOWN(X) (input.frame.keyDown[SDL_SCANCODE_ ## X])
        #define FRAME_REPEAT(X) (input.frame.keyRepeat[SDL_SCANCODE_ ## X])
        #define FRAME_UP(X) (input.frame.keyUp[SDL_SCANCODE_ ## X])

        const int gifCentiseconds = 4;
        bool giffing = false;
        MsfGifState gifState = {};
        float gifTimer = 0;

        gl_error("program init");
    print_log("[] done initializing: %f seconds\n", get_time());

    const float tickLength = 1.0f/250;
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
        SDL_SetRelativeMouseMode(SDL_TRUE); //because we want a virtual mouse cursor

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
                    msf_gif_begin(&gifState, canvas.width, canvas.height);
                    gifTimer = 0;
                } else {
                    MsfGifResult result = msf_gif_end(&gifState);
                    FILE * fp = fopen("out.gif", "wb");
                    fwrite(result.data, result.dataSize, 1, fp);
                    fclose(fp);
                    msf_gif_free(result);
                }
            }

            //print profiling trace
            if (TICK_DOWN(BACKSLASH) && (HELD(LCTRL) || HELD(RCTRL))) {
                print_profiling_trace();
                print_log("printed profiling trace\n");
            }



            //game update logic goes here
            //update virtual cursor
            level.player.cursor += vec2(input.tick.mouseMotion) * 0.04f;
            float cursorRadiusInner = 1.0f, cursorRadiusOuter = 5.0f;
            if (len(level.player.cursor) < 0.001f) level.player.cursor = vec2(0, cursorRadiusInner);
            if (len(level.player.cursor) < cursorRadiusInner) level.player.cursor = setlen(level.player.cursor, cursorRadiusInner);
            if (len(level.player.cursor) > cursorRadiusOuter) level.player.cursor = setlen(level.player.cursor, cursorRadiusOuter);

            //tick player
            if (!level.player.dead) {
                //apply gravity to player
                Vec2 gravity = vec2(0, 8.0f);
                level.player.vel += gravity * tick;
                level.player.pos += level.player.vel * tick;

                //kill player if they touch any solid geometry
                if (collide_with_tiles(level.tiles, player_hitbox(level.player.pos))) {
                    level.player.dead = true;
                    settings.bestDistance = fmaxf(settings.bestDistance, level.player.pos.x - level.playerStartPos.x);
                    settings.save();
                }
            }

            //tick enemies and spawn bullets
            for (Enemy & enemy : level.enemies) {
                if (len(enemy.pos - level.player.pos) > 100) continue;

                //proximity will make enemies shoot slightly faster as you get closer, to keep the game balanced
                //NOTE: it's important to apply the proximity effect to how fast the timer counts down
                //      instead of to its starting value, since the latter will inherently create some lag
                //      in the responsiveness of the effect
                float proximity = 0.2f * powf(len(enemy.pos - level.player.pos), 1.0f / 2);
                enemy.timer -= tick / fmaxf(0.2f, proximity);
                if (enemy.timer < 0) {
                    float random = rand_float(BULLET_INTERVAL_VARIANCE, 1 / BULLET_INTERVAL_VARIANCE);
                    enemy.timer = BULLET_INTERVAL * random;

                    //TODO: make enemies partly lead their shots
                    Vec2 dir = noz(level.player.pos - enemy.pos);
                    level.bullets.add({ .pos = enemy.pos + dir * 0.5f, .vel = dir * BULLET_VEL_MAX });
                }
            }

            //tick bullets
            for (int i = 0; i < level.bullets.len; ++i) {
                Bullet & bullet = level.bullets[i];

                //bullet velocity exponentially decays from the starting velocity down to BULLET_VEL_MIN
                bullet.vel = noz(bullet.vel) * (BULLET_VEL_MIN + (len(bullet.vel) - BULLET_VEL_MIN) * powf(0.4f, tick));
                bullet.pos += bullet.vel * tick;

                //despawn if very far from the camera
                if (len(bullet.pos - level.camCenter) > canvas.width / PIXELS_PER_UNIT * 2) {
                    level.bullets.remove(i);
                    i -= 1;
                    continue;
                }

                //collide with level
                if (collide_with_tiles(level.tiles, bullet_hitbox(bullet.pos))) {
                    level.bullets.remove(i);
                    i -= 1;
                    continue;
                }

                //collide with player
                if (intersects(bullet_hitbox(bullet.pos), player_hitbox(level.player.pos))) {
                    level.player.vel += (bullet.vel - level.player.vel) * (BULLET_MASS / PLAYER_MASS) * 0.5f;
                    Vec2 normal = noz(level.player.pos - bullet.pos);
                    level.player.vel += normal * fmaxf(0, dot(bullet.vel, normal)) * (BULLET_MASS / PLAYER_MASS) * 0.5f;
                    level.bullets.remove(i);
                    i -= 1;
                    continue;
                }

                //collide with shield
                if (intersects(reverse_winding(shield_hitbox(level.player)),
                               reverse_winding(obb(bullet_hitbox(bullet.pos)))))
                {
                    //this check ensures bullets only bounce off the shield's front side, not its back side
                    if (dot(bullet.vel - level.player.vel, level.player.cursor) < 0) {
                        //in order to do this properly we have to do proper rigidbody collision response
                        //so the math gets kind of hairy. equations basically copied from chris hecker's
                        //collision response articles http://www.chrishecker.com/images/e/e7/Gdmphys3.pdf
                        //NOTE: because |n| and e are both 1, some equations from the article become simplified
                        Vec2 normal = noz(level.player.cursor);
                        Vec2 relVel = bullet.vel - level.player.vel;
                        float impulse = -2 * dot(relVel, normal) / (1 / BULLET_MASS + 1 / PLAYER_MASS);
                        bullet.vel += normal * (impulse / BULLET_MASS);
                        level.player.vel -= normal * (impulse / PLAYER_MASS);
                    }
                }
            }

            //tick walkers
            for (Walker & walker : level.walkers) {
                //attack
                if (walker.attackTimer == 0 && len(level.player.pos - walker.pos) < WALKER_ATTACK_RANGE) {
                    level.player.vel.y = 0;
                    level.player.vel -= noz(level.player.cursor) * 20;
                    walker.attackTimer = WALKER_ATTACK_TIME;
                    walker.walkTimer = 0;
                }
                walker.attackTimer = fmaxf(0, walker.attackTimer - tick);

                //walk
                if (walker.attackTimer == 0 && len(level.player.pos - walker.pos) < WALKER_AGRO_RANGE) {
                    if (walker.pos.x > walker.home.x - WALKER_HOME_RADIUS && level.player.pos.x < walker.pos.x - 0.1f) {
                        walker.pos.x -= WALKER_WALK_SPEED * tick;
                        walker.walkTimer += tick;
                        walker.facingRight = false;
                    } else if (walker.pos.x < walker.home.x + WALKER_HOME_RADIUS && level.player.pos.x > walker.pos.x + 0.1f) {
                        walker.pos.x += WALKER_WALK_SPEED * tick;
                        walker.walkTimer += tick;
                        walker.facingRight = true;
                    } else {
                        walker.walkTimer = 0;
                    }
                }
            }

            //update camera
            static bool debugCam = false;
            DEBUG_TOGGLE(debugCam, TICK_DOWN(F));
            if (debugCam) {
                level.camCenter += vec2(HELD(D) - HELD(A), HELD(S) - HELD(W)) * 50 * tick;
            } else {
                level.camCenter += (level.player.pos - level.camCenter) * 0.01f; //TODO: tick rate dependent
                level.camCenter.x = fmaxf(level.camCenter.x, canvas.width / 2.0f / PIXELS_PER_UNIT);
                level.camCenter.y = fmaxf(level.camCenter.y, canvas.height / 2.0f / PIXELS_PER_UNIT);
                level.camCenter.y = fminf(level.camCenter.y, level.tiles.height * UNITS_PER_TILE - canvas.height / 2 / PIXELS_PER_UNIT);
            }

            //DEBUG exit
            if (TICK_DOWN(ESCAPE)) shouldExit = true;



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

        //print framerate every so often
        float framerate = ARR_SIZE(frameTimes) / timeSum;
        if (frameCount % 100 == 99) {
            print_log("frame %5d     fps %3d\n", frameCount, (int)(framerate + 0.5f));
        }

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

        //calculate camera offset
        int offx = level.camCenter.x * PIXELS_PER_UNIT - canvas.width  * 0.5f;
        int offy = level.camCenter.y * PIXELS_PER_UNIT - canvas.height * 0.5f;

        //clear canvas
        for (int y = 0; y < canvas.height; ++y) {
            for (int x = 0; x < canvas.width; ++x) {
                canvas[y][x] = { (u8) (y + offy), (u8) (x + offx), 255, 255 };
            }
        }

        //pixel art rendering and such goes here
        auto draw_sprite_centered = [&canvas, &offx, &offy] (Image sprite, Vec2 pos) {
            draw_sprite(canvas, sprite, pos.x * PIXELS_PER_UNIT - offx - sprite.width  * 0.5f,
                                        pos.y * PIXELS_PER_UNIT - offy - sprite.height * 0.5f);
        };

        auto draw_anim_centered = [&canvas, &offx, &offy] (Tileset anim, Vec2 pos, int frame, bool flip) {
            draw_tile_flip(canvas, anim, frame % anim.width, 0, pos.x * PIXELS_PER_UNIT - offx - anim.tileWidth  * 0.5f,
                                                                pos.y * PIXELS_PER_UNIT - offy - anim.tileHeight * 0.5f, flip);
        };

        auto draw_hitbox = [&canvas, &offx, &offy] (Rect hb) {
            draw_rect(canvas, lroundf(hb.x * PIXELS_PER_UNIT) - offx, lroundf(hb.y * PIXELS_PER_UNIT) - offy,
                              lroundf(hb.w * PIXELS_PER_UNIT), lroundf(hb.h * PIXELS_PER_UNIT), { 255, 100, 255, 200 });
        };

        //draw level
        draw_tile_grid(canvas, level.tiles, graphics.tileset, offx, offy);

        if (!level.player.dead) {
            //draw player and cursor
            draw_sprite_centered(graphics.player, level.player.pos);
            draw_sprite_centered(graphics.cursor, level.player.pos + level.player.cursor);

            //draw shield
            auto draw_obb = [&canvas, &offx, &offy] (OBB o, Color c) {
                Coord2 p[4]; for (int i = 0; i < 4; ++i) p[i] = coord2(o.p[i] * PIXELS_PER_UNIT) - coord2(offx, offy);
                draw_triangle(canvas, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y, c);
                draw_triangle(canvas, p[0].x, p[0].y, p[2].x, p[2].y, p[3].x, p[3].y, c);
            };
            draw_obb(shield_hitbox(level.player), { 128, 255, 128, 255 });
        }

        //draw enemies
        for (Enemy & enemy : level.enemies) {
            draw_sprite_centered(graphics.ghost, enemy.pos);
        }

        for (Walker & walker : level.walkers) {
            if (walker.attackTimer > 0) {
                draw_anim_centered(graphics.walkerAttack, walker.pos,
                                   (1 - (walker.attackTimer / WALKER_ATTACK_TIME)) * graphics.walkerAttack.width, !walker.facingRight);
            } else {
                draw_anim_centered(graphics.walkerWalk, walker.pos, walker.walkTimer * WALKER_WALK_SPEED, !walker.facingRight);
            }
        }

        //draw bullets
        for (Bullet & bullet : level.bullets) {
            float r1 = BULLET_RADIUS * PIXELS_PER_UNIT * 1.1f; //hitbox radiuse
            float r2 = BULLET_RADIUS * PIXELS_PER_UNIT * 1.5f; //visual radius
            draw_oval_f(canvas, bullet.pos.x * PIXELS_PER_UNIT - offx,
                                bullet.pos.y * PIXELS_PER_UNIT - offy, r2, r2, { 255, 0, 0, 255 });
            draw_oval_f(canvas, bullet.pos.x * PIXELS_PER_UNIT - offx,
                                bullet.pos.y * PIXELS_PER_UNIT - offy, r1, r1, { 127, 0, 0, 255 });
        }

        //DEBUG draw hitboxes
        static bool debugDraw = false;
        DEBUG_TOGGLE(debugDraw, FRAME_DOWN(D));
        if (debugDraw) {
            draw_hitbox(player_hitbox(level.player.pos));
            for (Bullet & bullet : level.bullets) draw_hitbox(bullet_hitbox(bullet.pos));
        }

        //distance counter
        {
            char buf[50];
            snprintf(buf, sizeof(buf), "distance: %.0f meters", level.player.pos.x - level.playerStartPos.x);
            float r = 4;
            float w = font.glyphWidth * strlen(buf) + r * 2, h = font.glyphHeight + r * 2;
            draw_rect(canvas, canvas.width / 2 - w / 2, font.glyphHeight - r, w, h, { 0, 0, 0, 100 });
            draw_text_center(canvas, font, canvas.width / 2 + 1, font.glyphHeight + 1, { 0, 0, 0, 255 }, buf);
            draw_text_center(canvas, font, canvas.width / 2 + 0, font.glyphHeight + 0, { 255, 255, 255, 255 }, buf);
        }

        //draw game over screen
        if (level.player.dead) {
            draw_rect(canvas, 0, 0, canvas.width, canvas.height, { 0, 0, 0, 160 });
            Color white = { 255, 255, 255, 255 };
            draw_text_center(canvas, font, canvas.width / 2, canvas.height / 2 - 6 * font.glyphHeight, white, "GAME OVER");
            char buf[100] = {};
            snprintf(buf, sizeof(buf), "You made it %.0f meters toward freedom...", level.player.pos.x - level.playerStartPos.x);
            draw_text_center(canvas, font, canvas.width / 2, canvas.height / 2 - 2 * font.glyphHeight, white, buf);
            snprintf(buf, sizeof(buf), "Your personal best is %0.f meters.", settings.bestDistance);
            draw_text_center(canvas, font, canvas.width / 2, canvas.height / 2 + 2 * font.glyphHeight, white, buf);
            draw_text_center(canvas, font, canvas.width / 2, canvas.height / 2 + 6 * font.glyphHeight, white, "Press R to try again.");
        }

        //level restart
        if (FRAME_DOWN(R)) {
            level = init_level();
        }

        //framerate display
        {
            char buf[20] = {};
            snprintf(buf, sizeof(buf), "%dfps", (int) lroundf(framerate));
            draw_text_right(canvas, font, canvas.width - font.glyphWidth, font.glyphHeight, { 255, 255, 255, 255 }, buf);
            if (giffing) draw_text(canvas, font, font.glyphWidth, font.glyphHeight, { 255, 255, 255, 255 }, "GIF");
        }

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
            msf_gif_frame(&gifState, (uint8_t *) canvas.pixels, gifCentiseconds, 15, canvas.pitch * 4);
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
