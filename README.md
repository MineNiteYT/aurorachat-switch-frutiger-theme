<h1 align="center">Welcome to the aurorachat repository!</h1>
This is the Switch client for Aurorachat.<br>
For more clients and stuff, see the <a href="https://github.com/Unitendo/aurorachat">main repo</a>.
The license, code of conduct, and security/contributing guidelines in the main repo also apply here.

<br>This repository is <b>open</b> for contributions! If you'd like to, you may open a PR or an issue, contributing helps us as we develop aurorachat!

<h1 align="center">How to build aurorachat</h1>

Install devkitpro with the Switch development libraries and make, then execute the following commands based on your OS:

Windows:
```sh
pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

Arch Linux or other distros with pacman:
```sh
yay -S devkitpro-pacman
sudo dkp-pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

Other Linux distros without pacman:
```sh
sudo dkp-pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

(At least that's what I think you gotta do)

## Troubleshooting
*When using AuroraChat on the switch, you may run into some problems, or error screens. These all have different meanings.*
There are three possibilities on why you may get an error:
1. The server is down or unreachable
2. You have a bad internet connection
3. An internal error occurred

<div align="center">
  <details>
  <summary><strong>Errors</strong></summary>
  
  | Errors | Message | Meaning |
  |--------|---------|---------|
  | ROOM_FETCH_FAIL | Failed to load rooms | The server is down or unreachable. |
  | SCR_WIP | Screen Work in progress | The screen is still being worked on. |
  | SCR_VAL_INV | Invalid screen value | The screen value is set to an invalid screen ID. Usually means the screen is Work in progress. |
  | REALLOC_NULL | Not enough memory | Happens when AuroraChat fails to allocate enough memory. |
  | (unsure) | curl_easy_perform() failed | Happens when curl somehow fails to make a request. |
  | INV_AUTH | Invalid username or password | Happens when the username or the password is invalid (none is set) |
  | SRV_UNREACH | The server never responded. | Happens when the server is unreachable or down. |
  | WRONG_PASS | You entered the wrong password. Try again. | You entered the wrong password. |
  | BAD_TOKEN | Invalid response from server. | Happens when the server sent an invalid token or response. |
  | MIX_LOAD_FAIL | (unsure) | Happens when SDL2 fails to load the music. |
  | MIX_PLAY_FAIL | (unsure) | Happens when SDL2 fails to play the music. |
  | USER_USED | This user is already used. | The username you chose has already been taken. Choose another username. |
  </details>
</div>

## Current development status
- [x] Main menu
- [x] Create Account screen
- [x] Login screen
- [x] Room selection screen
- [x] Chat screen