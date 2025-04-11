#include <SDL.h>
#include <SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./constants.h"
#include "SDL_pixels.h"
#include "SDL_scancode.h"

Uint64 start;
Uint64 end;
float fps;

int game_is_running    = FALSE;
int game_is_in_pause   = FALSE;
SDL_Window *window     = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font_regular = NULL;
TTF_Font *font_small   = NULL;

int last_frame_time = 0;

typedef enum {
    VMOV_STOPPED = 0,
    VMOV_UP      = 1,
    VMOV_DOWN    = -1,
} VMOV;

struct Ball {
    float x;
    float y;
    float width;
    float height;
    float dy;
    float dx;
} ball;

typedef struct Pad Pad;
struct Pad {
    float x;
    float y;
    float width;
    float height;
    VMOV dv;
};

typedef struct Player Player;
struct Player {
    Pad paddle;
    int score;
};

Player left_player, right_player;

void draw_score();
void goal();
void kick_off();
void start_new_game();

void
freeze_game(int milliseconds) {
    SDL_Delay(milliseconds);
    last_frame_time = SDL_GetTicks64(); // reset time past since last frame
}

int
initialize_window(void) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error initalizing SDL.\n");
        return FALSE;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Error initalizing TTF.\n");
        return FALSE;
    }

    // update score
    font_regular =
        TTF_OpenFont(FONT_PATH,
                     32); // specify the path to your font file and font size
    font_small =
        TTF_OpenFont(FONT_PATH,
                     12); // specify the path to your font file and font size
    if (!font_regular || !font_small) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return FALSE;
    }

    window =
        SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_BORDERLESS);
    if (!window) {
        fprintf(stderr, "Error creating SDL Window.\n");
        return FALSE;
    }

    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Error creating SDL Renderer.\n");
        return FALSE;
    }

    return TRUE;
}

int
draw_text(char *text, SDL_Color color, TTF_Font *font, int x, int y) {
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);
    if (!text_surface) {
        printf("Failed to create text surface: %s\n", TTF_GetError());
        return FALSE;
    }

    SDL_Texture *text_texture =
        SDL_CreateTextureFromSurface(renderer, text_surface);

    if (!text_texture) {
        printf("Failed to create text texture: %s\n", SDL_GetError());
        return FALSE;
    }

    SDL_Rect text_rect = {x - text_surface->w / 2, y - text_surface->h / 2,
                          text_surface->w, text_surface->h};
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
    return TRUE;
}

void
draw_pause_menu() {
    SDL_Color text_color = {GRAY_COLOR, SDL_ALPHA_OPAQUE}; // black color
    int x                = WINDOW_WIDTH * 0.5;
    int y                = WINDOW_HEIGHT * 0.5;
    draw_text("PAUSE", text_color, font_regular, x, y);
}

void
draw_game_frame() {
    SDL_Rect ball_rect = {(int)ball.x, (int)ball.y, (int)ball.width,
                          (int)ball.height};

    SDL_Rect left_pad_rect = {
        (int)left_player.paddle.x, (int)left_player.paddle.y,
        (int)left_player.paddle.width, (int)left_player.paddle.height};

    SDL_Rect right_pad_rect = {
        (int)right_player.paddle.x, (int)right_player.paddle.y,
        (int)right_player.paddle.width, (int)right_player.paddle.height};

    SDL_SetRenderDrawColor(renderer, GREEN_COLOR, SDL_ALPHA_OPAQUE);

    SDL_RenderFillRect(renderer, &ball_rect);
    SDL_RenderFillRect(renderer, &left_pad_rect);
    SDL_RenderFillRect(renderer, &right_pad_rect);

    draw_score();
}

void
draw_fps(float fps) {
    char fps_str[9];
    sprintf(fps_str, "%2.2f", fps);
    SDL_Color text_color = {GRAY_COLOR, SDL_ALPHA_OPAQUE}; // black color
    int x                = WINDOW_WIDTH * 0.97;
    int y                = WINDOW_HEIGHT * 0.98;
    draw_text(fps_str, text_color, font_small, x, y);
}

void
render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    if (game_is_in_pause) {
        draw_pause_menu();
    } else {
        draw_game_frame();
    }
    draw_fps(fps);
    SDL_RenderPresent(renderer);
}

double
randf(double low, double high) {
    return (rand() / (double)(RAND_MAX)) * fabs(low - high) + low;
}

void
set_init_position() {
    ball.x      = (WINDOW_WIDTH / 2.0);
    ball.y      = (WINDOW_HEIGHT / 2.0);
    ball.width  = 15;
    ball.height = 15;
    ball.dy     = 0;
    ball.dx     = 0;

    left_player.paddle.x      = 0 + 10;
    left_player.paddle.y      = (WINDOW_HEIGHT / 2.0) - PLAYER_PAD_HEIGHT / 2.0;
    left_player.paddle.width  = PLAYER_PAD_WIDTH;
    left_player.paddle.height = PLAYER_PAD_HEIGHT;
    left_player.paddle.dv     = VMOV_STOPPED;

    right_player.paddle.x     = WINDOW_WIDTH - 10 - 5;
    right_player.paddle.y     = (WINDOW_HEIGHT / 2.0) - PLAYER_PAD_HEIGHT / 2.0;
    right_player.paddle.width = PLAYER_PAD_WIDTH;
    right_player.paddle.height = PLAYER_PAD_HEIGHT;
    right_player.paddle.dv     = VMOV_STOPPED;
}

void
kick_off() {
    // give random movement to the ball
    ball.dy = randf(-0.1, 0.1);
    ball.dx = randf(-1, 1) > 0 ? 0.2 : -0.2;
}

void
start_new_game() {
    left_player.score  = 0;
    right_player.score = 0;
    set_init_position();
    kick_off();
}

void
draw_score() {
    // Create surface with rendered text
    SDL_Color text_color = {155, 155, 155, 255}; // black color

    char right_score[2];
    sprintf(right_score, "%d", right_player.score);
    char left_score[2];
    sprintf(left_score, "%d", left_player.score);

    int left_score_x  = WINDOW_WIDTH * 0.5 - 50;
    int left_score_y  = WINDOW_HEIGHT * 0.1;
    int right_score_x = WINDOW_WIDTH * 0.5 + 50;
    int right_score_y = WINDOW_HEIGHT * 0.1;

    draw_text(right_score, text_color, font_regular, right_score_x,
              right_score_y);
    draw_text(left_score, text_color, font_regular, left_score_x, left_score_y);
}

void
goal() {
    set_init_position();
    draw_score();
    render();
    freeze_game(500);
    kick_off();
}

void
setup() {
    srand(time(NULL));
    start_new_game();
}

void
process_input() {
    SDL_Event event;
    const Uint8 *keyboard_state_array = SDL_GetKeyboardState(NULL);

    // poll until all events are handled!
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            game_is_running = FALSE;
            break;
        case SDL_KEYDOWN:
            if (keyboard_state_array[SDL_SCANCODE_ESCAPE] ||
                keyboard_state_array[SDL_SCANCODE_Q]) {
                game_is_running = FALSE;
            } else if (keyboard_state_array[SDL_SCANCODE_SPACE]) {
                game_is_in_pause = !game_is_in_pause;
                last_frame_time  = SDL_GetTicks64();
            }
            break;
        }
    }

    // handle players movement
    if (keyboard_state_array[SDL_SCANCODE_W] &&
        !keyboard_state_array[SDL_SCANCODE_S]) {
        left_player.paddle.dv = VMOV_UP;
    } else if (!keyboard_state_array[SDL_SCANCODE_W] &&
               keyboard_state_array[SDL_SCANCODE_S]) {
        left_player.paddle.dv = VMOV_DOWN;
    }

    if (keyboard_state_array[SDL_SCANCODE_K] &&
        !keyboard_state_array[SDL_SCANCODE_J]) {
        right_player.paddle.dv = VMOV_UP;
    } else if (!keyboard_state_array[SDL_SCANCODE_K] &&
               keyboard_state_array[SDL_SCANCODE_J]) {
        right_player.paddle.dv = VMOV_DOWN;
    }
}

void
update() {
    // ensure fixed timestep
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks64() - last_frame_time);

    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        SDL_Delay(time_to_wait);
    }

    // get delta time factor in seconds
    float delta_time = (SDL_GetTicks64() - last_frame_time) / 1000.0f;
    last_frame_time = SDL_GetTicks64();

    if (game_is_in_pause) {
        return;
    }


    // move the ball X pixels per second
    float ball_delta   = BALL_MOV_PIXELS_PER_SECOND * delta_time;
    float ball_delta_x = ball.dx * ball_delta;
    float ball_delta_y = ball.dy * ball_delta;

    if (ball.y + ball_delta_y > WINDOW_HEIGHT || ball.y + ball_delta_y < 0) {
        ball.dy = -ball.dy;
    }
    if (ball.dx > 0 && // if ball is moving right
        (ball.x + ball_delta_x >
         WINDOW_WIDTH - PLAYER_PAD_WIDTH - 10)) { // if on the right edge
        if (ball.y + ball_delta_y + ball.height >=
                right_player.paddle.y - ball.height &&
            ball.y + ball_delta_y <=
                right_player.paddle.y +
                    PLAYER_PAD_HEIGHT) { // if touching the paddle
            float relative_intersect_y =
                right_player.paddle.y + (PLAYER_PAD_HEIGHT / 2.0) - ball.y;
            float normalized_relative_intersection_y =
                (relative_intersect_y / (PLAYER_PAD_HEIGHT / 2.0));
            float bounce_angle =
                normalized_relative_intersection_y * MAX_BOUNCE_ANGLE;
            ball.dx = -cos(bounce_angle);
            ball.dy = -sin(bounce_angle);
        } else {
            // if goal on the right
            left_player.score++;
            goal();
            if (left_player.score == 3) {
                start_new_game();
                freeze_game(2000);
            }
            return;
        }
        // if ball on the left goal
    } else if (ball.dx < 0 && // bal is moving left
               (ball.x + ball_delta_x < 0 + PLAYER_PAD_WIDTH + 10)) {
        if (ball.y + ball_delta_y + ball.height >=
                left_player.paddle.y - ball.height &&
            ball.y <= left_player.paddle.y + PLAYER_PAD_HEIGHT) {
            // if ball on the paddle
            float relative_intersect_y =
                left_player.paddle.y + (PLAYER_PAD_HEIGHT / 2.0) - ball.y;
            float normalized_relative_intersection_y =
                (relative_intersect_y / (PLAYER_PAD_HEIGHT / 2.0));
            float bounce_angle =
                normalized_relative_intersection_y * MAX_BOUNCE_ANGLE;
            ball.dx = cos(bounce_angle);
            ball.dy = -sin(bounce_angle);
        } else {
            // if goal on the left
            right_player.score++;
            goal();
            if (right_player.score == 3) {
                start_new_game();
                freeze_game(1500);
            }
            return;
        }
    }

    if (ball.dx == 0)
        ball.dx = ball_delta_x > 0 ? 0.5 : -0.5;
    ball.x += ball.dx * ball_delta;
    ball.y += ball.dy * ball_delta;

    // move the players pads
    float pad_delta = PLAYER_MOV_PIXELS_PER_SECOND * delta_time;
    if (left_player.paddle.dv == VMOV_UP) {
        if (left_player.paddle.y - pad_delta > 0) {
            left_player.paddle.y -= pad_delta;
        }
    } else if (left_player.paddle.dv == VMOV_DOWN) {
        if (left_player.paddle.y + pad_delta <
            (WINDOW_HEIGHT - PLAYER_PAD_HEIGHT)) {
            left_player.paddle.y += pad_delta;
        }
    }
    left_player.paddle.dv = VMOV_STOPPED;
    if (right_player.paddle.dv == VMOV_UP) {
        if (right_player.paddle.y - pad_delta > 0) {
            right_player.paddle.y -= pad_delta;
        }
    } else if (right_player.paddle.dv == VMOV_DOWN) {
        if (right_player.paddle.y + pad_delta <
            (WINDOW_HEIGHT - PLAYER_PAD_HEIGHT)) {
            right_player.paddle.y += pad_delta;
        }
    }
    right_player.paddle.dv = VMOV_STOPPED;
}

void
destroy_window() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int
main() {

    game_is_running = initialize_window();

    setup();

    while (game_is_running) {
        start = SDL_GetPerformanceCounter();

        process_input();
        update();
        render();

        end           = SDL_GetPerformanceCounter();
        float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
        fps           = 1.0f / elapsed;
    }

    destroy_window();
    return 0;
}
