#include "game.h"
#include "UI.h"
#include "Logo.h"
#include "Title.h"
#include "Scene.h"
#include "FadeManager.h"
#include "data_ptrs.h"
#include "ScriptRunner.h"

UBYTE joy;
UBYTE prev_joy;
UBYTE time;

POS camera_dest;
UBYTE camera_settings = CAMERA_LOCK_FLAG;
UBYTE wait_time = 0;
UBYTE shake_time = 0;
UBYTE actor_move_settings;
POS actor_move_dest;
STAGE_TYPE stage_type;
STAGE_TYPE stage_next_type = MAP;
typedef void (*STAGE_UPDATE_FN)();
STAGE_UPDATE_FN UpdateFn;

SCRIPT_CMD_FN last_fn;
UBYTE script_continue;
UBYTE script_action_complete = TRUE;
UBYTE script_actor;

int main()
{
  // Init LCD
  LCDC_REG = 0x67;
  set_interrupts(VBL_IFLAG | LCD_IFLAG);
  STAT_REG = 0x45;

  // Set palettes
  BGP_REG = 0xE4U;
  OBP0_REG = 0xD2U;

  // Position Window Layer
  WY_REG = MAXWNDPOSY - 7;
  WY_REG = MAXWNDPOSY + 1;

  actors[0].sprite = 0;
  actors[0].redraw = TRUE;
  map_next_pos.x = actors[0].pos.x = (START_SCENE_X << 3) + 8;
  map_next_pos.y = actors[0].pos.y = (START_SCENE_Y << 3) + 8;
  map_next_dir.x = actors[0].dir.x =
      START_SCENE_DIR == 2 ? -1 : START_SCENE_DIR == 4 ? 1 : 0;
  map_next_dir.y = actors[0].dir.y =
      START_SCENE_DIR == 8 ? -1 : START_SCENE_DIR == 1 ? 1 : 0;
  actors[0].movement_type = PLAYER_INPUT;
  actors[0].enabled = TRUE;

  scene_index = START_SCENE_INDEX;
  scene_next_index = START_SCENE_INDEX;

  UIInit();

  UpdateFn = SceneUpdate;

  DISPLAY_ON;
  SHOW_SPRITES;

  FadeInit();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(game_loop, 60, 1);
#else
  while (1)
  {
    game_loop();
  }
#endif
}

void game_loop()
{

#ifdef __EMSCRIPTEN__
  emscripten_update_registers(SCX_REG, SCY_REG,
                              WX_REG, WY_REG,
                              LYC_REG, LCDC_REG, BGP_REG, OBP0_REG, OBP1_REG);
#endif

  wait_vbl_done();
  LYC_REG = 0x0;

  joy = joypad();

  // Handle stage switch
  if (stage_type != stage_next_type && !IsFading())
  {

    if (stage_type == TITLE)
    {
      TitleCleanup();
    }

    stage_type = stage_next_type;
    scene_index = scene_next_index;

    map_next_pos.x = actors[0].pos.x;
    map_next_pos.y = actors[0].pos.y;
    map_next_dir.x = actors[0].dir.x;
    map_next_dir.y = actors[0].dir.y;

    if (stage_type == MAP)
    {
      SceneInit();
      UpdateFn = SceneUpdate;
    }
    else if (stage_type == LOGO)
    {
      LogoInit();
      UpdateFn = LogoUpdate;
    }
    else if (stage_type == TITLE)
    {
      TitleInit();
      UpdateFn = TitleUpdate;
    }
  }

  UpdateFn();

  // Handle Fade
  FadeUpdate();

  prev_joy = joy;
  time++;
}

void script_cmd_line()
{
  LOG("- SCRIPT: EVENT_LINE %u\n", (script_cmd_args[0] * 256) + script_cmd_args[1]);
  script_action_complete = FALSE;
  script_ptr += 3;
  set_text_line((script_cmd_args[0] * 256) + script_cmd_args[1]);
}

void script_cmd_set_emotion()
{
  script_ptr += 3;
  // MapSetEmotion(script_arg1, script_arg2);
  script_action_complete = FALSE;
  script_continue = FALSE;
}
