/** File: connectionFlow.cpp
 *
 * Author: Al Chaussee
 * Created: 02/28/2018
 *
 * Description: Functions for updating what to display on the face
 *              during various parts of the connection flow
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "cozmoAnim/connectionFlow.h"

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animComms.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/robotDataLoader.h"

#include "coretech/common/shared/array2d.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/data/dataScope.h"
#include "coretech/vision/engine/image.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_sendAnimToRobot_helper.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include "osState/osState.h"

#include <chrono>
#include <cmath>

namespace Anki {
namespace Vector {

namespace {
u32 _pin = 123456;

const f32 kRobotNameScale = 0.7f;
const std::string kURLDef = "vectortech.dev/v";
const std::string kURLWP = "wp.pvic.xyz";
const ColorRGBA   kColor(1.f, 1.f, 1.f, 1.f);
// The spinning V: the app's "Tip Over Teal" eye color (hue 0.42, saturation 1)
const ColorRGBA   kTeal(0.f, 1.f, 0.52f, 1.f);

const char* kShowPinScreenSpriteName = "pairing_icon_key";

bool usingWP = Util::FileUtils::FileExists("/data/data/server_config.json");

bool s_enteredAnyScreen = false;

// The start pairing screen is animated (a V spinning on a tablet), so it's redrawn by TickConnectionFlow
bool s_onStartPairingScreen = false;
const ColorRGBA kTealDim(0.f, 0.55f, 0.52f * 0.55f, 1.f);
const f32 kSpinPeriod_sec = 1.6f;
const f32 kFrameInterval_sec = 0.05f;
f32 s_lastFrame_sec = 0.f;

f32 Now_sec()
{
  return std::chrono::duration<f32>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Portrait tablet outline with rounded corners and a home button
void DrawTablet(Vision::ImageRGBA* img, f32 x0, f32 y0, f32 x1, f32 y1)
{
  const f32 r = 4.f;
  img->DrawLine({x0 + r, y0}, {x1 - r, y0}, kColor, 2);
  img->DrawLine({x0 + r, y1}, {x1 - r, y1}, kColor, 2);
  img->DrawLine({x0, y0 + r}, {x0, y1 - r}, kColor, 2);
  img->DrawLine({x1, y0 + r}, {x1, y1 - r}, kColor, 2);
  img->DrawLine({x0, y0 + r}, {x0 + r, y0}, kColor, 2);
  img->DrawLine({x1 - r, y0}, {x1, y0 + r}, kColor, 2);
  img->DrawLine({x0, y1 - r}, {x0 + r, y1}, kColor, 2);
  img->DrawLine({x1 - r, y1}, {x1, y1 - r}, kColor, 2);
  img->DrawCircle({(x0 + x1) / 2.f, y1 - 5.f}, kColor, 2, 1);
}

// The V, turned by angle (radians) about its vertical axis: narrower side-on, dimmer from behind
void DrawSpinningV(Vision::ImageRGBA* img, f32 cx, f32 top, f32 bottom, f32 angle)
{
  const f32 k = std::cos(angle);
  const f32 halfWidth = 11.f * std::abs(k);
  const ColorRGBA color = k >= 0 ? kTeal : kTealDim;
  const s32 thickness = std::max(2, (s32)std::round(4.f * std::abs(k)));
  img->DrawLine({cx - halfWidth, top}, {cx, bottom}, color, thickness);
  img->DrawLine({cx + halfWidth, top}, {cx, bottom}, color, thickness);
}
}

// Draws BLE name (top), a V spinning on a tablet, and the url (bottom)
bool DrawStartPairingScreen(Anim::AnimationStreamer* animStreamer)
{
  // Robot name will be empty until switchboard has set the property
  std::string robotName = OSState::getInstance()->GetRobotName();
  if(robotName == "")
  {
    return false;
  }

  s_enteredAnyScreen = true;
  s_onStartPairingScreen = true;
  s_lastFrame_sec = Now_sec();

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  img->DrawTextCenteredHorizontally(robotName, cv::QT_FONT_NORMAL, kRobotNameScale, 1, kColor, 13, false);

  const f32 cx = FACE_DISPLAY_WIDTH / 2.f;
  DrawTablet(img, cx - 17.f, 19.f, cx + 17.f, 77.f);
  const f32 angle = 2.f * M_PI * std::fmod(s_lastFrame_sec, kSpinPeriod_sec) / kSpinPeriod_sec;
  DrawSpinningV(img, cx, 30.f, 62.f, angle);

  std::string kURL = usingWP ? kURLWP : kURLDef;
  img->DrawTextCenteredHorizontally(kURL, cv::QT_FONT_NORMAL, 0.55f, 1, kColor, FACE_DISPLAY_HEIGHT - 3, false);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool overrideAllSpritesToEyeHue = false;
  animStreamer->SetFaceImage(handle, overrideAllSpritesToEyeHue, 0);

  return true;
}

void TickConnectionFlow(Anim::AnimationStreamer* animStreamer)
{
  if(s_onStartPairingScreen && Now_sec() - s_lastFrame_sec >= kFrameInterval_sec)
  {
    DrawStartPairingScreen(animStreamer);
  }
}

// Draws BLE name, key icon, and BLE pin to screen
void DrawShowPinScreen(Anim::AnimationStreamer* animStreamer, const Anim::AnimContext* context, const std::string& pin)
{
  s_enteredAnyScreen = true;
  
  Vision::ImageRGB key;
  key.Load(context->GetDataLoader()->GetSpritePaths()->GetAssetPath(kShowPinScreenSpriteName));
  if(IsXray()) {
    key.Resize(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  }

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  Point2f p((FACE_DISPLAY_WIDTH - key.GetNumCols())/2,
            (FACE_DISPLAY_HEIGHT - key.GetNumRows())/2);
  img->DrawSubImage(key, p);

  img->DrawTextCenteredHorizontally(OSState::getInstance()->GetRobotName(), cv::QT_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

  img->DrawTextCenteredHorizontally(pin, cv::QT_FONT_NORMAL, 0.8f, 1, kColor, FACE_DISPLAY_HEIGHT-5, false);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool overrideAllSpritesToEyeHue = false;
  animStreamer->SetFaceImage(handle, overrideAllSpritesToEyeHue, 0);
}

// Uses a png sequence animation to draw wifi icon to screen
void DrawWifiScreen(Anim::AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_wifi", 0, 0, 0, shouldInterrupt);
}

// Uses a png sequence animation to draw os updating icon to screen
void DrawUpdatingOSScreen(Anim::AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update", 0, 0, 0, shouldInterrupt);
}

// Uses a png sequence animation to draw os updating error icon to screen
void DrawUpdatingOSErrorScreen(Anim::AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update_error", 0, 0, 0, shouldInterrupt);
}

// Uses a png sequence animation to draw waiting for app icon to screen
void DrawWaitingForAppScreen(Anim::AnimationStreamer* animStreamer)
{
  s_enteredAnyScreen = true;
  
  const bool shouldInterrupt = true;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_awaitingapp", 0, 0, 0, shouldInterrupt);
}

void SetBLEPin(uint32_t pin)
{
  _pin = pin;
}

bool InitConnectionFlow(Anim::AnimationStreamer* animStreamer)
{
  if(FACTORY_TEST)
  {
    // Don't start connection flow if not packed out
    if(!Factory::GetEMR()->fields.PACKED_OUT_FLAG)
    {
      return true;
    }

    return DrawStartPairingScreen(animStreamer);
  }
  return true;
}

void UpdateConnectionFlow(const SwitchboardInterface::SetConnectionStatus& msg,
                          Anim::AnimationStreamer* animStreamer,
                          const Anim::AnimContext* context)
{
  using namespace SwitchboardInterface;

  PRINT_NAMED_INFO("ConnectionFlow.UpdateConnectionFlow.NewStatus", "%s", EnumToString(msg.status));
  
  // isPairing is a proxy for "switchboard is doing something and needs to display something on face"
  const bool isPairing = msg.status != ConnectionStatus::NONE &&
                         msg.status != ConnectionStatus::COUNT &&
                         msg.status != ConnectionStatus::SHOW_URL_FACE &&
                         msg.status != ConnectionStatus::END_PAIRING;

  const bool shouldControlFace = isPairing || 
    (msg.status == ConnectionStatus::SHOW_URL_FACE);

  // Enable pairing screen if status is anything besides NONE, COUNT, and END_PAIRING
  // Should do nothing if called multiple times with same argument such as when transitioning from
  // START_PAIRING to SHOW_PRE_PIN
  FaceInfoScreenManager::getInstance()->EnablePairingScreen(isPairing);

  // Disable face keepalive, but don't re-enable it when ending pairing. The engine will send a message
  // when it's ready to re-enable it, since it needs time to send its first animation upon resuming
  if (shouldControlFace) {
    animStreamer->Abort();
    animStreamer->EnableKeepFaceAlive(false, 0);

    // Always look up since we're displaying something that user will want to see
    RobotInterface::SetHeadAngle msg;
    msg.angle_rad             = MAX_HEAD_ANGLE;
    msg.max_speed_rad_per_sec = DEG_TO_RAD(60);
    msg.accel_rad_per_sec2    = DEG_TO_RAD(360);
    msg.duration_sec          = 0;
    msg.actionID              = 0;
    SendAnimToRobot(std::move(msg));
  }


  s_onStartPairingScreen = false;

  switch(msg.status)
  {
    case ConnectionStatus::NONE:
    {

    }
    break;
    case ConnectionStatus::SHOW_URL_FACE:
    case ConnectionStatus::START_PAIRING:
    {
      DrawStartPairingScreen(animStreamer);
    }
    break;
    case ConnectionStatus::SHOW_PRE_PIN:
    {
      DrawShowPinScreen(animStreamer, context, "######");
    }
    break;
    case ConnectionStatus::SHOW_PIN:
    {
      DrawShowPinScreen(animStreamer, context, std::to_string(_pin));
    }
    break;
    case ConnectionStatus::SETTING_WIFI:
    {
      DrawWifiScreen(animStreamer);
     }
    break;
    case ConnectionStatus::UPDATING_OS:
    {
      DrawUpdatingOSScreen(animStreamer);
    }
    break;
    case ConnectionStatus::UPDATING_OS_ERROR:
    {
      DrawUpdatingOSErrorScreen(animStreamer);
    }
    break;
    case ConnectionStatus::WAITING_FOR_APP:
    {
      DrawWaitingForAppScreen(animStreamer);
    }
    break;
    case ConnectionStatus::END_PAIRING:
    {
      if(s_enteredAnyScreen)
      {
        animStreamer->Abort();
      }
      s_enteredAnyScreen = false;
      

      // Probably will never get here because we will restart
      // while updating os
      if(FACTORY_TEST)
      {
        DrawStartPairingScreen(animStreamer);
      }
    }
    break;
    case ConnectionStatus::COUNT:
    {

    }
    break;
  }
}

}
}
