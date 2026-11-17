/*
​Slither.io Clone para PlayStation Portable (PSP)
​Desenvolvido em C usando SDL e SDL_gfx (PSPSDK)
​Controles:
​Analógico ou D-Pad: Direcionar a cobra
​Botão X (Cross): Turbo / Boost (consome tamanho e solta massa)
​Botão Start: Reiniciar após Game Over / Pausar
*/
​#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
​#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
​/* Configuração do Módulo PSP */
PSP_MODULE_INFO("SlitherPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
​/* Constantes da Tela do PSP */
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
​/* Constantes do Mundo e Gameplay */
#define WORLD_WIDTH   2400
#define WORLD_HEIGHT  2400
#define MAX_SEGMENTS  400
#define MAX_ORBS      350
#define MAX_BOTS      12
#define BASE_SPEED    2.6f
#define BOOST_SPEED   4.8f
#define PI            3.14159265f
​/* Estrutura para os pontos do corpo da cobra */
typedef struct {
float x;
float y;
} Point;
​/* Estrutura da Cobra (Jogador e Bots) */
typedef struct {
Point body[MAX_SEGMENTS];
int length;
float angle;
float target_angle;
float radius;
int alive;
int is_player;
int boosting;
float boost_timer;
Uint8 r, g, b;       // Cor principal
Uint8 r2, g2, b2;    // Cor secundária (listras)
float ai_timer;
} Snake;
​/* Estrutura das Orbes de Comida */
typedef struct {
float x;
float y;
float radius;
Uint8 r, g, b;
int active;
} Orb;
​/* Variáveis Globais */
SDL_Surface *screen = NULL;
Snake player;
Snake bots[MAX_BOTS];
Orb orbs[MAX_ORBS];
float cam_x = 0.0f;
float cam_y = 0.0f;
int running = 1;
int game_over = 0;
int score = 0;
​/* Callbacks de Saída do PSP (Botão Home) */
int exit_callback(int arg1, int arg2, void *common) {
running = 0;
return 0;
}
​int CallbackThread(SceSize args, void *argp) {
int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
sceKernelRegisterExitCallback(cbid);
sceKernelSleepThreadCB();
return 0;
}
​int SetupCallbacks(void) {
int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
if (thid >= 0) {
sceKernelStartThread(thid, 0, 0);
}
return thid;
}
​/* Funções Utilitárias */
float get_distance(float x1, float y1, float x2, float y2) {
float dx = x2 - x1;
float dy = y2 - y1;
return sqrtf(dx * dx + dy * dy);
}
​/* Inicializa uma orbe aleatória ou na posição da morte */
void spawn_orb(int index, float x, float y, int from_death) {
if (from_death) {
orbs[index].x = x + (rand() % 20 - 10);
orbs[index].y = y + (rand() % 20 - 10);
orbs[index].radius = 5.5f;
} else {
orbs[index].x = (float)(rand() % (WORLD_WIDTH - 40) + 20);
orbs[index].y = (float)(rand() % (WORLD_HEIGHT - 40) + 20);
orbs[index].radius = (float)(rand() % 3 + 3);
}
​/* Cores neon vibrantes */
int color_type = rand() % 6;
switch (color_type) {
case 0: orbs[index].r = 255; orbs[index].g = 60;  orbs[index].b = 100; break;
case 1: orbs[index].r = 60;  orbs[index].g = 255; orbs[index].b = 120; break;
case 2: orbs[index].r = 60;  orbs[index].g = 180; orbs[index].b = 255; break;
case 3: orbs[index].r = 255; orbs[index].g = 220; orbs[index].b = 50;  break;
case 4: orbs[index].r = 210; orbs[index].g = 80;  orbs[index].b = 255; break;
default: orbs[index].r = 255; orbs[index].g = 130; orbs[index].b = 40; break;
}
orbs[index].active = 1;
}
​/* Inicializa ou reinicia uma cobra */
void init_snake(Snake *s, int is_player) {
s->is_player = is_player;
s->alive = 1;
s->length = is_player ? 25 : (15 + rand() % 25);
s->radius = 7.5f;
s->boosting = 0;
s->boost_timer = 0.0f;
s->ai_timer = 0.0f;
s->angle = ((float)(rand() % 360)) * (PI / 180.0f);
s->target_angle = s->angle;
​float start_x = (float)(rand() % (WORLD_WIDTH - 400) + 200);
float start_y = (float)(rand() % (WORLD_HEIGHT - 400) + 200);
​for (int i = 0; i < MAX_SEGMENTS; i++) {
s->body[i].x = start_x - cosf(s->angle) * (i * 4.0f);
s->body[i].y = start_y - sinf(s->angle) * (i * 4.0f);
}
​if (is_player) {
s->r = 0;   s->g = 230; s->b = 160;
s->r2 = 0;  s->g2 = 160; s->b2 = 255;
} else {
s->r = 80 + rand() % 175;
s->g = 80 + rand() % 175;
s->b = 80 + rand() % 175;
s->r2 = (s->r > 128) ? s->r - 60 : s->r + 60;
s->g2 = (s->g > 128) ? s->g - 60 : s->g + 60;
s->b2 = (s->b > 128) ? s->b - 60 : s->b + 60;
}
}
​/* Transforma o corpo de uma cobra morta em orbes brilhantes */
void kill_snake(Snake *s) {
s->alive = 0;
int orb_idx = 0;
for (int i = 0; i < s->length; i += 2) {
while (orb_idx < MAX_ORBS && orbs[orb_idx].active) {
orb_idx++;
}
if (orb_idx < MAX_ORBS) {
spawn_orb(orb_idx, s->body[i].x, s->body[i].y, 1);
}
}
if (s->is_player) {
game_over = 1;
}
}
​/* Reinicia toda a partida */
void reset_game() {
game_over = 0;
score = 0;
init_snake(&player, 1);
for (int i = 0; i < MAX_BOTS; i++) {
init_snake(&bots[i], 0);
}
for (int i = 0; i < MAX_ORBS; i++) {
spawn_orb(i, 0, 0, 0);
}
}
​/* Lógica de Inteligência Artificial simples para os Bots */
void update_bot_ai(Snake *bot) {
bot->ai_timer -= 1.0f;
​/* Evitar as bordas da arena */
float margin = 150.0f;
if (bot->body[0].x < margin || bot->body[0].x > WORLD_WIDTH - margin ||
bot->body[0].y < margin || bot->body[0].y > WORLD_HEIGHT - margin) {
float center_x = WORLD_WIDTH / 2.0f;
float center_y = WORLD_HEIGHT / 2.0f;
bot->target_angle = atan2f(center_y - bot->body[0].y, center_x - bot->body[0].x);
bot->ai_timer = 20.0f;
return;
}
​/* Buscar comida próxima periodicamente */
if (bot->ai_timer <= 0) {
bot->ai_timer = 15.0f + (rand() % 20);
float best_dist = 250.0f;
int target_orb = -1;
​for (int i = 0; i < MAX_ORBS; i++) {
if (orbs[i].active) {
float d = get_distance(bot->body[0].x, bot->body[0].y, orbs[i].x, orbs[i].y);
if (d < best_dist) {
best_dist = d;
target_orb = i;
}
}
}
​if (target_orb != -1) {
bot->target_angle = atan2f(orbs[target_orb].y - bot->body[0].y,
orbs[target_orb].x - bot->body[0].x);
} else {
bot->target_angle += ((float)(rand() % 100 - 50)) * 0.02f;
}
}
}
​/* Atualiza física, movimento e colisões de uma cobra */
void update_snake(Snake *s) {
if (!s->alive) {
if (!s->is_player && (rand() % 120 == 0)) {
init_snake(s, 0); // Respawn automático para bots
}
return;
}
​if (!s->is_player) {
update_bot_ai(s);
}
​/* Suavização da rotação (turn rate limitado como no Slither.io) */
float diff = s->target_angle - s->angle;
while (diff < -PI) diff += 2 * PI;
while (diff > PI)  diff -= 2 * PI;
​float turn_speed = 0.09f;
if (fabsf(diff) < turn_speed) {
s->angle = s->target_angle;
} else {
s->angle += (diff > 0) ? turn_speed : -turn_speed;
}
​/* Velocidade e mecânica de Boost */
float current_speed = BASE_SPEED;
if (s->boosting && s->length > 12) {
current_speed = BOOST_SPEED;
s->boost_timer += 1.0f;
if (s->boost_timer >= 12.0f) {
s->boost_timer = 0.0f;
s->length--;
if (s->is_player && score > 0) score -= 5;
​/* Solta uma orbe na cauda ao usar boost */
for (int i = 0; i < MAX_ORBS; i++) {
if (!orbs[i].active) {
spawn_orb(i, s->body[s->length].x, s->body[s->length].y, 1);
break;
}
}
}
}
​/* Move a cabeça */
float new_x = s->body[0].x + cosf(s->angle) * current_speed;
float new_y = s->body[0].y + sinf(s->angle) * current_speed;
​/* Colisão com as paredes da arena */
if (new_x < s->radius || new_x > WORLD_WIDTH - s->radius ||
new_y < s->radius || new_y > WORLD_HEIGHT - s->radius) {
kill_snake(s);
return;
}
​/* Atualiza os segmentos do corpo mantendo espaçamento constante */
float segment_dist = 4.2f;
Point prev = { new_x, new_y };
s->body[0] = prev;
​for (int i = 1; i < s->length; i++) {
float dx = s->body[i].x - prev.x;
float dy = s->body[i].y - prev.y;
float dist = sqrtf(dx * dx + dy * dy);
if (dist > 0.001f) {
s->body[i].x = prev.x + (dx / dist) * segment_dist;
s->body[i].y = prev.y + (dy / dist) * segment_dist;
}
prev = s->body[i];
}
​/* Ajusta espessura proporcional ao tamanho */
s->radius = 7.5f + (s->length * 0.022f);
if (s->radius > 18.0f) s->radius = 18.0f;
​/* Comer Orbes /
for (int i = 0; i < MAX_ORBS; i++) {
if (orbs[i].active) {
float d = get_distance(s->body[0].x, s->body[0].y, orbs[i].x, orbs[i].y);
if (d < s->radius + orbs[i].radius + 3.0f) {
orbs[i].active = 0;
if (s->length < MAX_SEGMENTS - 1) {
s->body[s->length] = s->body[s->length - 1];
s->length++;
}
if (s->is_player) {
score += 10;
}
/ Repõe orbe no mapa */
spawn_orb(i, 0, 0, 0);
}
}
}
}
​/* Verifica colisões entre cabeças e corpos de cobras diferentes */
void check_snake_collisions() {
Snake *all_snakes[MAX_BOTS + 1];
all_snakes[0] = &player;
for (int i = 0; i < MAX_BOTS; i++) {
all_snakes[i + 1] = &bots[i];
}
​for (int i = 0; i <= MAX_BOTS; i++) {
if (!all_snakes[i]->alive) continue;
float head_x = all_snakes[i]->body[0].x;
float head_y = all_snakes[i]->body[0].y;
float head_r = all_snakes[i]->radius * 0.8f;
​for (int j = 0; j <= MAX_BOTS; j++) {
if (i == j || !all_snakes[j]->alive) continue;
​for (int seg = 1; seg < all_snakes[j]->length; seg += 2) {
float dist = get_distance(head_x, head_y,
all_snakes[j]->body[seg].x,
all_snakes[j]->body[seg].y);
if (dist < head_r + all_snakes[j]->radius * 0.85f) {
kill_snake(all_snakes[i]);
break;
}
}
if (!all_snakes[i]->alive) break;
}
}
}
​/* Renderiza uma cobra na tela */
void draw_snake(Snake *s) {
if (!s->alive) return;
​/* Desenha da cauda para a cabeça para sobreposição correta */
for (int i = s->length - 1; i >= 0; i--) {
int sx = (int)(s->body[i].x - cam_x);
int sy = (int)(s->body[i].y - cam_y);
​/* Frustum Culling: ignora segmentos fora da tela do PSP */
if (sx < -30 || sx > SCREEN_WIDTH + 30 || sy < -30 || sy > SCREEN_HEIGHT + 30) {
continue;
}
​int r_seg = (int)s->radius;
if (i > s->length - 5) {
r_seg = (int)(s->radius * 0.75f); // Cauda afinada
}
​/* Efeito de brilho quando usa Boost */
if (s->boosting && (i % 2 == 0)) {
filledCircleRGBA(screen, sx, sy, r_seg + 2, 255, 255, 255, 90);
}
​/* Alterna cores para criar padrão listrado */
if ((i / 3) % 2 == 0) {
filledCircleRGBA(screen, sx, sy, r_seg, s->r, s->g, s->b, 255);
} else {
filledCircleRGBA(screen, sx, sy, r_seg, s->r2, s->g2, s->b2, 255);
}
}
​/* Desenha os olhos na cabeça */
int hx = (int)(s->body[0].x - cam_x);
int hy = (int)(s->body[0].y - cam_y);
if (hx >= -30 && hx <= SCREEN_WIDTH + 30 && hy >= -30 && hy <= SCREEN_HEIGHT + 30) {
float eye_offset = s->radius * 0.55f;
float eye_angle_1 = s->angle - 0.55f;
float eye_angle_2 = s->angle + 0.55f;
​int ex1 = hx + (int)(cosf(eye_angle_1) * eye_offset);
int ey1 = hy + (int)(sinf(eye_angle_1) * eye_offset);
int ex2 = hx + (int)(cosf(eye_angle_2) * eye_offset);
int ey2 = hy + (int)(sinf(eye_angle_2) * eye_offset);
​filledCircleRGBA(screen, ex1, ey1, 3, 255, 255, 255, 255);
filledCircleRGBA(screen, ex2, ey2, 3, 255, 255, 255, 255);
filledCircleRGBA(screen, ex1 + (int)(cosf(s->angle)), ey1 + (int)(sinf(s->angle)), 1, 0, 0, 0, 255);
filledCircleRGBA(screen, ex2 + (int)(cosf(s->angle)), ey2 + (int)(sinf(s->angle)), 1, 0, 0, 0, 255);
}
}
​/* Renderiza o cenário, grid hexagonal/quadriculado, orbes, cobras e HUD /
void render_scene() {
/ Fundo escuro estilo Slither.io */
SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 22, 24, 30));
​/* Grid de fundo com paralaxe */
int grid_size = 40;
int offset_x = -((int)cam_x % grid_size);
int offset_y = -((int)cam_y % grid_size);
​for (int x = offset_x; x < SCREEN_WIDTH; x += grid_size) {
vlineRGBA(screen, x, 0, SCREEN_HEIGHT, 34, 38, 46, 255);
}
for (int y = offset_y; y < SCREEN_HEIGHT; y += grid_size) {
hlineRGBA(screen, 0, SCREEN_WIDTH, y, 34, 38, 46, 255);
}
​/* Bordas vermelhas da arena */
int bx1 = (int)(0 - cam_x);
int by1 = (int)(0 - cam_y);
int bx2 = (int)(WORLD_WIDTH - cam_x);
int by2 = (int)(WORLD_HEIGHT - cam_y);
rectangleRGBA(screen, bx1, by1, bx2, by2, 255, 50, 50, 255);
​/* Desenha Orbes */
for (int i = 0; i < MAX_ORBS; i++) {
if (orbs[i].active) {
int ox = (int)(orbs[i].x - cam_x);
int oy = (int)(orbs[i].y - cam_y);
if (ox >= -10 && ox <= SCREEN_WIDTH + 10 && oy >= -10 && oy <= SCREEN_HEIGHT + 10) {
filledCircleRGBA(screen, ox, oy, (int)orbs[i].radius + 2, orbs[i].r, orbs[i].g, orbs[i].b, 70);
filledCircleRGBA(screen, ox, oy, (int)orbs[i].radius, orbs[i].r, orbs[i].g, orbs[i].b, 255);
}
}
}
​/* Desenha Bots e Jogador */
for (int i = 0; i < MAX_BOTS; i++) {
draw_snake(&bots[i]);
}
draw_snake(&player);
​/* HUD: Pontuação e Tamanho */
char hud_text[64];
sprintf(hud_text, "Massa: %d  |  Pontos: %d", player.length * 10, score);
stringRGBA(screen, 10, 10, hud_text, 255, 255, 255, 255);
​/* Minimapa no canto inferior direito */
int map_size = 54;
int map_x = SCREEN_WIDTH - map_size - 10;
int map_y = SCREEN_HEIGHT - map_size - 10;
boxRGBA(screen, map_x, map_y, map_x + map_size, map_y + map_size, 10, 12, 16, 180);
rectangleRGBA(screen, map_x, map_y, map_x + map_size, map_y + map_size, 100, 110, 130, 255);
​for (int i = 0; i < MAX_BOTS; i++) {
if (bots[i].alive) {
int px = map_x + (int)((bots[i].body[0].x / WORLD_WIDTH) * map_size);
int py = map_y + (int)((bots[i].body[0].y / WORLD_HEIGHT) * map_size);
pixelRGBA(screen, px, py, 255, 80, 80, 255);
}
}
if (player.alive) {
int px = map_x + (int)((player.body[0].x / WORLD_WIDTH) * map_size);
int py = map_y + (int)((player.body[0].y / WORLD_HEIGHT) * map_size);
filledCircleRGBA(screen, px, py, 2, 0, 255, 160, 255);
}
​/* Tela de Game Over */
if (game_over) {
boxRGBA(screen, 110, 95, 370, 175, 15, 15, 20, 220);
rectangleRGBA(screen, 110, 95, 370, 175, 255, 70, 70, 255);
stringRGBA(screen, 205, 112, "GAME OVER!", 255, 80, 80, 255);
sprintf(hud_text, "Pontuacao Final: %d", score);
stringRGBA(screen, 165, 132, hud_text, 255, 255, 255, 255);
stringRGBA(screen, 135, 154, "Pressione START para jogar novamente", 180, 220, 255, 255);
}
​SDL_Flip(screen);
}
​/* Lê controles analógicos e digitais do PSP */
void handle_input(SceCtrlData *pad, SceCtrlData *old_pad) {
sceCtrlReadBufferPositive(pad, 1);
​if (game_over) {
if ((pad->Buttons & PSP_CTRL_START) && !(old_pad->Buttons & PSP_CTRL_START)) {
reset_game();
}
return;
}
​/* Leitura do Analógico (valores de 0 a 255, centro ~128) */
float ax = ((float)pad->Lx - 128.0f) / 128.0f;
float ay = ((float)pad->Ly - 128.0f) / 128.0f;
​/* Zona morta (Deadzone) do analógico do PSP /
if (fabsf(ax) > 0.22f || fabsf(ay) > 0.22f) {
player.target_angle = atan2f(ay, ax);
} else {
/ Suporte alternativo ao D-Pad */
float dx = 0.0f, dy = 0.0f;
if (pad->Buttons & PSP_CTRL_UP)    dy -= 1.0f;
if (pad->Buttons & PSP_CTRL_DOWN)  dy += 1.0f;
if (pad->Buttons & PSP_CTRL_LEFT)  dx -= 1.0f;
if (pad->Buttons & PSP_CTRL_RIGHT) dx += 1.0f;
if (dx != 0.0f || dy != 0.0f) {
player.target_angle = atan2f(dy, dx);
}
}
​/* Botão X (Cross) ativa o Boost */
player.boosting = (pad->Buttons & PSP_CTRL_CROSS) ? 1 : 0;
}
​int main(int argc, char *argv[]) {
SetupCallbacks();
srand((unsigned int)time(NULL));
​/* Inicializa leitura do analógico do PSP */
sceCtrlSetSamplingCycle(0);
sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
​/* Inicializa SDL Video */
if (SDL_Init(SDL_INIT_VIDEO) < 0) {
return 1;
}
​screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
if (!screen) {
SDL_Quit();
return 1;
}
SDL_ShowCursor(SDL_DISABLE);
​reset_game();
​SceCtrlData pad, old_pad;
sceCtrlReadBufferPositive(&old_pad, 1);
​/* Loop Principal a ~60 FPS */
while (running) {
Uint32 frame_start = SDL_GetTicks();
​handle_input(&pad, &old_pad);
old_pad = pad;
​if (!game_over) {
update_snake(&player);
for (int i = 0; i < MAX_BOTS; i++) {
update_snake(&bots[i]);
}
check_snake_collisions();
​/* Centraliza câmera suavemente na cabeça do jogador */
if (player.alive) {
cam_x = player.body[0].x - (SCREEN_WIDTH / 2.0f);
cam_y = player.body[0].y - (SCREEN_HEIGHT / 2.0f);
}
}
​render_scene();
​/* Limita a 60 FPS */
Uint32 frame_time = SDL_GetTicks() - frame_start;
if (frame_time < 16) {
SDL_Delay(16 - frame_time);
}
}
​SDL_Quit();
sceKernelExitGame();
return 0;
}
