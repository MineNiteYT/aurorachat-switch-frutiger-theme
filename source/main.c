#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ft2build.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <png.h>
#include <setjmp.h>
#include FT_FREETYPE_H

#define RGBA(r,g,b,a) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))
#define COL_BG      RGBA(0xA2, 0x46, 0xF3, 0xFF)
#define COL_PANEL   RGBA(0x16, 0x21, 0x3E, 0xFF)
#define COL_HOVER   RGBA(0x2A, 0x3A, 0x5E, 0xFF)
#define COL_HEADER  RGBA(0x0F, 0x34, 0x60, 0xFF)
#define COL_WHITE   RGBA(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_RED     RGBA(0xFF, 0x00, 0x00, 0xFF)
#define COL_BLACK   RGBA(0x00, 0x00, 0x00, 0xFF)

static u32* framebuf;
static u32 framebuf_width;
static FT_Library ft;
static FT_Face face;

struct MemoryStruct {
    char *memory;
    size_t size;
};

void drawPixel(int x, int y, u32 color) {
    if (x >= 0 && x < 1280 && y >= 0 && y < 720)
        framebuf[y * framebuf_width + x] = color;
}

void drawRect(int x, int y, int w, int h, u32 color) {
    for (int row = y; row < y + h && row < 720; row++)
        for (int col = x; col < x + w && col < 1280; col++)
            if (row >= 0 && col >= 0)
                framebuf[row * framebuf_width + col] = color;
}

void clearScreen(u32 color) {
    for (int y = 0; y < 720; y++)
        for (int x = 0; x < 1280; x++)
            framebuf[y * framebuf_width + x] = color;
}

void drawGlyph(int x, int y, FT_Bitmap* bmp, u32 color) {
    u8 cr = (color >>  0) & 0xFF;
    u8 cg = (color >>  8) & 0xFF;
    u8 cb = (color >> 16) & 0xFF;

    for (int row = 0; row < (int)bmp->rows; row++) {
        for (int col = 0; col < (int)bmp->width; col++) {
            int px = x + col;
            int py = y + row;
            if (px < 0 || px >= 1280 || py < 0 || py >= 720) continue;

            u8 alpha = bmp->buffer[row * bmp->pitch + col];
            if (alpha == 0) continue;

            u32 existing = framebuf[py * framebuf_width + px];
            u8 er = (existing >>  0) & 0xFF;
            u8 eg = (existing >>  8) & 0xFF;
            u8 eb = (existing >> 16) & 0xFF;

            u8 a = alpha;
            u8 r = (cr * a + er * (255 - a)) / 255;
            u8 g = (cg * a + eg * (255 - a)) / 255;
            u8 b = (cb * a + eb * (255 - a)) / 255;

            framebuf[py * framebuf_width + px] = RGBA(r, g, b, 0xFF);
        }
    }
}

void drawImage(const char* path, int x, int y) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return;
    }
    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);

    png_read_update_info(png, info);
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int i = 0; i < height; i++) row_pointers[i] = (png_byte*)malloc(png_get_rowbytes(png,info));
    png_read_image(png, row_pointers);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int px = x + col;
            int py = y + row;
            if (px < 0 || px >= 1280 || py < 0 || py >= 720) continue;
            png_byte* pixel = &(row_pointers[row][col * 4]);
            u8 r = pixel[0];
            u8 g = pixel[1];
            u8 b = pixel[2];
            u8 a = pixel[3];
            if (a == 0) continue;
            u32 existing = framebuf[py * framebuf_width + px];
            u8 er = (existing >>  0) & 0xFF;
            u8 eg = (existing >>  8) & 0xFF;
            u8 eb = (existing >> 16) & 0xFF;
            u8 dr = (r * a + er * (255 - a)) / 255;
            u8 dg = (g * a + eg * (255 - a)) / 255;
            u8 db = (b * a + eb * (255 - a)) / 255;
            framebuf[py * framebuf_width + px] = RGBA(dr, dg, db, 0xFF);
        }
    }
    for (int i = 0; i < height; i++) free(row_pointers[i]);
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
}

bool isPointInRect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

void drawText(int x, int y, const char* text, u32 color, int size) {
    FT_Set_Pixel_Sizes(face, 0, size);

    int cx = x;
    int cy = y;

    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            cx = x;
            cy += size + 4;
            continue;
        }

        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;

        FT_GlyphSlot g = face->glyph;
        int bx = cx + g->bitmap_left;
        int by = cy - g->bitmap_top;

        drawGlyph(bx, by, &g->bitmap, color);

        cx += g->advance.x >> 6;
    }
}

char* openKeyboard(int maxlen, const char* guideText) {
    Result rc = 0;
    SwkbdConfig kbd;
    char* result = malloc(256);
    
    if (!result) return NULL;
    
    rc = swkbdCreate(&kbd, 0);
    if (R_SUCCEEDED(rc)) {
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetInitialText(&kbd, "");
        swkbdConfigSetGuideText(&kbd, guideText);
        swkbdConfigSetStringLenMax(&kbd, maxlen);
        rc = swkbdShow(&kbd, result, 256);
        swkbdClose(&kbd);
        
        if (R_SUCCEEDED(rc)) {
            return result;
        }
    }
    
    free(result);
    return NULL;
}

const char* errmsg = "";
const char* errcode = "";

void drawError(const char* message, const char* error_code) {
    drawText(10, 48, "oops, something went wrong :/", COL_RED, 50);
    drawText(10, 114, message, COL_WHITE, 35);
    drawText(10, 710, error_code, COL_WHITE, 22);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        drawError("Not enough memory", "REALLOC_NULL");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

void network_request(const char* url, char** result, const char* method, const char* body, const char* content_type, const char* authorization) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    struct curl_slist *headers = NULL;  // moved here
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "aurorachat-switch/6.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        if (body != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
        }
        if (content_type != NULL) {
            char content_header[128];
            snprintf(content_header, sizeof(content_header), "Content-Type: %s", content_type);
            headers = curl_slist_append(headers, content_header);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        if (authorization != NULL) {
            char auth_header[512];
            snprintf(auth_header, sizeof(auth_header), "auth: %s", authorization);
            headers = curl_slist_append(headers, auth_header);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        consoleUpdate(NULL);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            drawError("curl_easy_perform() failed", curl_easy_strerror(res));
            free(chunk.memory);
            *result = NULL;
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            *result = chunk.memory;
        }
        if (headers) curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

Mix_Chunk* sfx_cache[16];
int sfx_count = 0;

Mix_Chunk* loadSFX(const char* path) {
    Mix_Chunk* sfx = Mix_LoadWAV(path);
    if (sfx && sfx_count < 16) {
        sfx_cache[sfx_count++] = sfx;
    }
    return sfx;
}

void freeSFX() {
    for (int i = 0; i < sfx_count; i++) {
        Mix_FreeChunk(sfx_cache[i]);
        sfx_cache[i] = NULL;
    }
    sfx_count = 0;
}

typedef struct {
    Mix_Chunk* sfx;
    int fade_ms;
} SFXThreadData;

int sfxThreadFunc(void* data) {
    SFXThreadData* d = (SFXThreadData*)data;
    Mix_Chunk* sfx   = d->sfx;
    int fade_ms      = d->fade_ms;
    free(d);

    int full_vol  = MIX_MAX_VOLUME;
    int half_vol  = MIX_MAX_VOLUME / 2;
    int step_time = 10;
    int steps     = fade_ms / step_time;

    for (int i = 0; i <= steps; i++) {
        int vol = full_vol - (int)((float)i / steps * (full_vol - half_vol));
        Mix_VolumeMusic(vol);
        SDL_Delay(step_time);
    }

    Mix_VolumeChunk(sfx, MIX_MAX_VOLUME);
    int channel = Mix_PlayChannel(-1, sfx, 0);
    if (channel != -1) {
        while (Mix_Playing(channel)) {
            SDL_Delay(10);
        }
    }

    for (int i = 0; i <= steps; i++) {
        int vol = half_vol + (int)((float)i / steps * (full_vol - half_vol));
        Mix_VolumeMusic(vol);
        SDL_Delay(step_time);
    }

    Mix_VolumeMusic(full_vol);
    return 0;
}

void playSFX(Mix_Chunk* sfx, int fade_ms) {
    if (!sfx) return;

    SFXThreadData* d = malloc(sizeof(SFXThreadData));
    if (!d) return;
    d->sfx     = sfx;
    d->fade_ms = fade_ms;

    SDL_Thread* thread = SDL_CreateThread(sfxThreadFunc, "sfx_fade", d);
    if (thread) {
        SDL_DetachThread(thread);
    } else {
        free(d);
    }
}

int screen = 0; // 0 = main menu, 1 = error screen, 2 = rules screen
const char* username = "";
const char* password = "";
char token[512];
int sock;
struct sockaddr_in server;

void drawMainMenu(u64 kDown) {
    AppletOperationMode mode = appletGetOperationMode();
    HidTouchScreenState touchState;
    if (hidGetTouchScreenStates(&touchState, 1) > 0 && touchState.count > 0) {
        u32 tx = touchState.touches[0].x;
        u32 ty = touchState.touches[0].y;
        if (isPointInRect(tx, ty, 470, 447, 341, 83)) {
            screen = 2;
            return;
        }
    }
    if (kDown & HidNpadButton_A) {
        screen = 2;
        return;
    }

    if (mode == AppletOperationMode_Console) drawText(0, 24, "AuroraChat works better in handheld mode!", COL_WHITE, 24);
    drawText(1200, 715, "v26.6.5", COL_WHITE, 24);
    drawImage("romfs:/images/aurorachat.png", 383, 190);
    drawImage("romfs:/images/buttons/enter.png", 470, 447);
}

int ruleslinescroll = 0;
char *rules = NULL;

void loadRules() {
    FILE *file = fopen("romfs:/rules.txt", "r");
    rules = malloc(1168);
    fread(rules, 1, 1167, file);
    rules[1167] = '\0';
    fclose(file);
}

void drawRules(u64 kDown) {
    if ((kDown & HidNpadButton_Up) && ruleslinescroll != 0) ruleslinescroll--;
    else if ((kDown & HidNpadButton_Down) && ruleslinescroll != 30) ruleslinescroll++;
    drawText(0, 24, "(use the D-Pad to scroll)", COL_WHITE, 24);
    drawText(375, 100, "Code of Conduct:", COL_WHITE, 64);
    drawImage("romfs:/images/boxes/rules.png", 439, 203);

    char *copy = strdup(rules);
    int linenum = 0;
    char *line = strtok(copy, "\n");
    while (line != NULL) {
        int y = (235 - (ruleslinescroll * 20)) + (linenum * 20);
        if (y >= 235 && y <= 545) {
            drawText(447, y, line, COL_BLACK, 20);
        }
        linenum++;
        line = strtok(NULL, "\n");
    }
    free(copy);

    drawImage("romfs:/images/buttons/done.png", 524, 598);
}

int loginselection = 1;
bool loginAttempted = false;
char* roomresult = NULL;
int roomselection = 1;
char** rooms = NULL;
int roomcount = 0;
char* selectedRoom = "";
void drawLogIn(u64 kDown) {
    if (kDown & HidNpadButton_Down) {
        loginselection++;
    }
    if (kDown & HidNpadButton_Up) {
        loginselection--;
    }
    if (kDown & HidNpadButton_A) {
        if (loginselection == 1) {
            char* result = openKeyboard(255, "Enter your username");
            if (result) {
                username = result;
            }
        } else if (loginselection == 2) {
            char* result = openKeyboard(255, "Enter your password");
            if (result) {
                password = result;
            }
        } else if (loginselection == 3) {
            if (strlen(username) == 0 || strlen(password) == 0) {
                errmsg = "Invalid username or password";
                errcode = "INV_AUTH";
                screen = 3;
                return;
            }

            if (loginAttempted) return;
            loginAttempted = true;
            char sender[512];
            snprintf(sender, sizeof(sender), "%s|%s|", username, password);
            char* loginreqresult = NULL;
            for (int attempt = 0; attempt < 3 && loginreqresult == NULL; attempt++) {
                network_request("http://104.236.25.60:6767/api/login", &loginreqresult, "POST", sender, "text/plain", NULL);
            }
            if (loginreqresult == NULL) {
                errmsg = "The server never responded.";
                errcode = "SRV_UNREACH";
                loginAttempted = false;
                screen = 3;
                return;
            }

            if (strstr(loginreqresult, "ERR_WRONG_PASS") != NULL) {
                errmsg = "You entered the wrong password. Try again.";
                errcode = "WRONG_PASS";
                free(loginreqresult);
                loginAttempted = false;
                screen = 3;
                return;
            }

            char loginbuf[1024];
            strncpy(loginbuf, loginreqresult, sizeof(loginbuf) - 1);
            loginbuf[sizeof(loginbuf) - 1] = '\0';
            free(loginreqresult);

            char* parsed_token = strtok(loginbuf, "|");
            if (parsed_token == NULL) {
                errmsg = "Invalid response from server.";
                errcode = "BAD_TOKEN";
                loginAttempted = false;
                screen = 3;
                return;
            }
            strncpy(token, parsed_token, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';

            // Fetch rooms
            char* roomreqresult = NULL;
            network_request("http://104.236.25.60:6767/api/rooms", &roomreqresult, "POST", NULL, NULL, NULL);
            roomresult = roomreqresult;

            // Play SFX
            Mix_Chunk* signedup_sfx = loadSFX("romfs:/sfx/signedup.mp3");
            playSFX(signedup_sfx, 150);

            screen = 4;
        }
    }
    if (kDown & HidNpadButton_B) {
        screen = 0;
    }
    if (loginselection == 4) {
        loginselection = 1;
    } else if (loginselection == 0) {
        loginselection = 3;
    }

    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Log In", COL_WHITE, 22);

    drawRect(0, 40, 1280, 40, loginselection == 1 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+40, "Username", COL_WHITE, 22);

    drawRect(0, 82, 1280, 40, loginselection == 2 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+82, "Password", COL_WHITE, 22);

    drawRect(0, 124, 1280, 40, loginselection == 3 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+124, "Log In", COL_WHITE, 22);
}

void drawCreateAccount(u64 kDown) { // create account is just login but with signup instead
    if (kDown & HidNpadButton_Down) {
        loginselection++;
    }
    if (kDown & HidNpadButton_Up) {
        loginselection--;
    }
    if (kDown & HidNpadButton_A) {
        if (loginselection == 1) {
            char* result = openKeyboard(255, "Enter your username");
            if (result) {
                username = result;
            }
        } else if (loginselection == 2) {
            char* result = openKeyboard(255, "Enter your password");
            if (result) {
                password = result;
            }
        } else if (loginselection == 3) {
            if (strlen(username) == 0 || strlen(password) == 0) {
                errmsg = "Invalid username or password";
                errcode = "INV_AUTH";
                screen = 3;
                return;
            }

            if (loginAttempted) return;
            loginAttempted = true;
            char sender[512];
            snprintf(sender, sizeof(sender), "%s|%s|", username, password);
            char* loginreqresult = NULL;
            for (int attempt = 0; attempt < 3 && loginreqresult == NULL; attempt++) {
                network_request("http://104.236.25.60:6767/api/signup", &loginreqresult, "POST", sender, "text/plain", NULL);
            }
            if (loginreqresult == NULL) {
                errmsg = "The server never responded.";
                errcode = "SRV_UNREACH";
                loginAttempted = false;
                screen = 3;
                return;
            }

            if (strstr(loginreqresult, "ERR_USER_USED") != NULL) {
                errmsg = "This user is already used.";
                errcode = "USER_USED";
                free(loginreqresult);
                loginAttempted = false;
                screen = 3;
                return;
            }

            char loginbuf[1024];
            strncpy(loginbuf, loginreqresult, sizeof(loginbuf) - 1);
            loginbuf[sizeof(loginbuf) - 1] = '\0';
            free(loginreqresult);

            char* parsed_token = strtok(loginbuf, "|");
            if (parsed_token == NULL) {
                errmsg = "Invalid response from server.";
                errcode = "BAD_TOKEN";
                loginAttempted = false;
                screen = 3;
                return;
            }
            strncpy(token, parsed_token, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';

            // Fetch rooms
            char* roomreqresult = NULL;
            network_request("http://104.236.25.60:6767/api/rooms", &roomreqresult, "POST", NULL, NULL, NULL);
            roomresult = roomreqresult;

            // Play SFX
            Mix_Chunk* signedup_sfx = loadSFX("romfs:/sfx/signedup.mp3");
            playSFX(signedup_sfx, 150);

            screen = 4;
        }
    }
    if (kDown & HidNpadButton_B) {
        screen = 0;
    }
    if (loginselection == 4) {
        loginselection = 1;
    } else if (loginselection == 0) {
        loginselection = 3;
    }

    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Create Account", COL_WHITE, 22);

    drawRect(0, 40, 1280, 40, loginselection == 1 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+40, "Username", COL_WHITE, 22);

    drawRect(0, 82, 1280, 40, loginselection == 2 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+82, "Password", COL_WHITE, 22);

    drawRect(0, 124, 1280, 40, loginselection == 3 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+124, "Create Account", COL_WHITE, 22);
}

void parseRooms(const char* roomdata) {
    if (!roomdata) return;
    
    if (rooms) {
        for (int i = 0; i < roomcount; i++) {
            free(rooms[i]);
        }
        free(rooms);
        rooms = NULL;
        roomcount = 0;
    }
    char* data = strdup(roomdata);
    char* token = strtok(data, "|");
    
    if (token) {
        roomcount = atoi(token);
        if (roomcount > 0) {
            rooms = malloc(roomcount * sizeof(char*));
            
            for (int i = 0; i < roomcount; i++) {
                token = strtok(NULL, "|");
                if (token) {
                    rooms[i] = strdup(token);
                } else {
                    rooms[i] = strdup("Unknown Room");
                }
            }
        }
    }
    
    free(data);
    roomselection = 1;
}

void drawRoomSelection(u64 kDown) {
    if (roomresult && !rooms) {
        parseRooms(roomresult);
    }
    if (kDown & HidNpadButton_Down) {
        roomselection++;
        if (roomselection > roomcount) roomselection = 1;
    }
    if (kDown & HidNpadButton_Up) {
        roomselection--;
        if (roomselection < 1) roomselection = roomcount;
    }
    if (kDown & HidNpadButton_A) {
        selectedRoom = rooms[roomselection-1];
        screen = 5;
    }
    
    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Room Selection", COL_WHITE, 22);

    if (rooms && roomcount > 0) {
        for (int i = 0; i < roomcount; i++) {
            int y_pos = 40 + (i * 42);
            drawRect(0, y_pos, 1280, 40, (i + 1 == roomselection) ? COL_HOVER : COL_PANEL);
            drawText(10, y_pos + 28, rooms[i], COL_WHITE, 22);
        }
    } else {
        drawError("Failed to load rooms", "ROOM_FETCH_FAIL");
    }
}

#define MAX_MESSAGES 26
#define MAX_MSG_LEN 350
char messages[MAX_MESSAGES][MAX_MSG_LEN];
int messageCount = 0;
void drawChatScreen(u64 kDown) {
    if (kDown & HidNpadButton_B) {
        memset(messages, 0, sizeof(messages));
        messageCount = 0;
        screen = 4;
    }
    if (kDown & HidNpadButton_Y) {
        char* result = openKeyboard(300, "Enter your message");
        if (result) {
            char* msg = result;
            char sender[512];
            snprintf(sender, sizeof(sender), "%s|%s|", msg, selectedRoom);
            char* networkresult = NULL;
            network_request("http://104.236.25.60:6767/api/chat", &networkresult, "POST", sender, "text/plain", token);
            free(networkresult);
        }
    }
    char title[128];
    snprintf(title, sizeof(title), "Chat Screen | #%s", selectedRoom);
    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, title, COL_WHITE, 22);
    for (int i = 0; i < messageCount; i++) {
        drawText(10, 80 + (i * 24), messages[i], COL_WHITE, 22);
    }

    drawText(1140, 707, "Press Y to type a message", COL_WHITE, 11);
}


void append_message(char* msg_username, char* msg, char* msg_room) {
    if (strcmp(msg_room, selectedRoom) != 0) return;
    if (messageCount >= MAX_MESSAGES) {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            memcpy(messages[i], messages[i+1], MAX_MSG_LEN);
        }
        messageCount = MAX_MESSAGES - 1;
    }
    snprintf(messages[messageCount], MAX_MSG_LEN, "<%s>: %s", msg_username, msg);
    messageCount++;
}

int main(int argc, char* argv[]) {
    romfsInit();

    FT_Init_FreeType(&ft);
    FT_New_Face(ft, "romfs:/fonts/OpenSans-Regular.ttf", 0, &face);

    NWindow* win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(3033);
    server.sin_addr.s_addr = inet_addr("104.236.25.60");
    connect(sock, (struct sockaddr*)&server, sizeof(server));
    int nonblock = 1;
    ioctl(sock, FIONBIO, &nonblock);

    loadRules();
    SDL_Init(SDL_INIT_AUDIO);
    Mix_Init(MIX_INIT_MP3);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
    Mix_Music *audio = Mix_LoadMUS("romfs:/music/loop.mp3");
    if (!audio) {
        errmsg = Mix_GetError();
        errcode = "MIX_LOAD_FAIL";
        screen = 3;
    } else if (Mix_PlayMusic(audio, -1) < 0) {
        errmsg = Mix_GetError();
        errcode = "MIX_PLAY_FAIL";
        screen = 3;
    }
    Mix_PlayMusic(audio, -1);
    
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        u32 stride;
        framebuf = (u32*)framebufferBegin(&fb, &stride);
        framebuf_width = stride / sizeof(u32);
        clearScreen(COL_BG);

        char buffer[1024] = {0};
        ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            char* username = strtok(buffer, "|");
            char* message  = strtok(NULL, "|");
            char* room     = strtok(NULL, "|");
            if (username && message && room) {
                append_message(username, message, room);
            }
        } else if (len == 0) {}
        
        if (screen == 0) {
            drawMainMenu(kDown);
        } else if (screen == 1) {
            drawError(errmsg, errcode);
        } else if (screen == 2) {
            drawRules(kDown);
        } else {
            drawError("Invalid screen value", "SCR_VAL_INV");
        }

        framebufferEnd(&fb);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    Mix_FreeMusic(audio);
    SDL_Quit();
    framebufferClose(&fb);
    socketExit();
    romfsExit();
    return 0;
}