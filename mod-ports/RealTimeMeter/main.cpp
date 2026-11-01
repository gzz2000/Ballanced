#include "main.h"

static RealTimeMeter *active_meter = nullptr;
MOD_EXPORT IMod* BMLEntry(IBML* bml) { return active_meter = new RealTimeMeter(bml); }
MOD_EXPORT void BMLExit(IMod* mod) { delete mod; active_meter = nullptr; }
MOD_EXPORT const char *BML_WebRealTimeMeterStatus() {
  return active_meter ? active_meter->Inspect() : "{}";
}
MOD_EXPORT const char *BML_WebRealTimeMeterText() { return active_meter ? active_meter->LabelText() : ""; }
const char *RealTimeMeter::Inspect() const {
  static char text[320];
  std::snprintf(text,sizeof(text),"{\"enabled\":%s,\"counting\":%s,\"label\":%s,\"ingameMs\":%.3f,\"worldMs\":%.3f,\"levelExited\":%s,\"streak\":%d}",
    enabled?"true":"false",counting?"true":"false",status?"true":"false",bml_time,
    sys_time ? (get_system_timestamp()-sys_time)/1000000.0 : 0.0,level_exited?"true":"false",streak_level);
  return text;
}
void RealTimeMeter::OnUnload() {
  status.reset();counting=false;GetLogger()->Info("RealTimeMeter unloaded");
}

void RealTimeMeter::OnLoad() {
  GetLogger()->Info("RealTimeMeter WASM loaded; upstream 0.1.2");
  time_manager = m_BML->GetTimeManager();
  GetConfig()->SetCategoryComment("Main", "Settings for Real Time Meter.");
  prop_enable = GetConfig()->GetProperty("Main", "Enabled");
  prop_enable->SetComment("Whether to show the timer when ingame.");
  prop_enable->SetDefaultBoolean(true);
  enabled = prop_enable->GetBoolean();
  prop_streak = GetConfig()->GetProperty("Main", "StreakMode");
  prop_streak->SetComment("Only reset the timer after the level set here is finished. 0 to disable. Example: 13 = Timer will only stop after finishing level 13.");
  prop_streak->SetDefaultInteger(0);
  streak_level = prop_streak->GetInteger();
}

void RealTimeMeter::OnModifyConfig(const char * category, const char * key, IProperty* prop) {
  if (prop == prop_enable) {
    enabled = prop_enable->GetBoolean();
    if (enabled) {
      if (m_BML->IsIngame() && !status)
        init_status_label();
    }
    else
      status.reset();
  }
  if (prop == prop_streak) {
    streak_level = prop_streak->GetInteger();
  }
}

void RealTimeMeter::OnLoadObject(const char * filename, CKBOOL isMap, const char * masterName,
																 CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
																 CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) {
	if (!enabled || !isMap)
    return;
  if (!status)
    init_status_label();
  counting = false;
  update_status(0.0f, 0.0f);
}

void RealTimeMeter::OnPostResetLevel() {
  if (!enabled)
    return;
  if (!status)
    init_status_label();
  counting = false;
  update_status(0.0f, 0.0f);
  streak_completed = false;
}

void RealTimeMeter::OnPreLoadLevel() {
  if (!enabled)
    return;
  // update current level
  m_BML->GetArrayByName("CurrentLevel")->GetElementValue(0, 0, &current_level);
  if (streak_level > 0 && level_exited) {
    if (current_level == 1 ||
        get_system_timestamp() - last_exit_sys_time > 8000000000) {
      bml_time = 0;
      sys_time = get_system_timestamp() - static_cast<double>(time_manager->GetLastDeltaTime()) * 1e6;
    }
    level_exited = false;
  }
}

void RealTimeMeter::OnPreExitLevel() {
  if (!enabled)
    return;
  status.reset();
  level_exited = true;
  last_exit_sys_time = get_system_timestamp();
  level_finished = false;
  counting = false;
  streak_completed = false;
}

void RealTimeMeter::OnStartLevel() {
  if (!enabled)
    return;
  counting = false;
  level_finished = false;
}

void RealTimeMeter::OnLevelFinish() {
  if (!enabled)
    return;
  level_finished = true;
  if (current_level == streak_level)
    streak_completed = true;
}

void RealTimeMeter::OnCounterInactive() {
  if (!enabled || !level_finished)
    return;
  counting = false;
  last_finish_sys_time = get_system_timestamp();
}

void RealTimeMeter::OnCounterActive() {
  if (!enabled || counting)
    return;
  if (streak_level <= 0 || level_exited) {
    bml_time = 0;
    // SR Timer is in fact counting 1 more frame than the acutal SR time
    sys_time = get_system_timestamp() - static_cast<double>(time_manager->GetLastDeltaTime()) * 1e6;
    level_exited = false;
  }
  status->SetVisible(true);
  counting = true;
}

void RealTimeMeter::OnProcess() {
  if (!enabled)
    return;
  auto current_time = get_system_timestamp();
  if (counting) {
    bml_time += time_manager->GetLastDeltaTime();
    update_status(bml_time, static_cast<float>((current_time - sys_time) / 1000000));
  }
  else if (!level_exited && streak_level > 0 && !streak_completed
           && current_time - last_finish_sys_time > 1610612735) {
    update_status(bml_time, static_cast<float>((current_time - sys_time) / 1000000));
  }
  if (status)
    status->Process();
}

void RealTimeMeter::init_status_label() {
  if (status)
    status.reset();
  status = std::make_unique<BGui::Label>("RealTimeMeter");
  status->SetAlignment(ALIGN_BOTTOM);
  status->SetPosition({ 0.03f, 0.732f });
  status->SetSize({ 0.20f, 0.03f });
  status->SetText("");
  status->SetFont(ExecuteBB::GAMEFONT_03);
  status->SetAlignment(ALIGN_LEFT);
  // status->SetZOrder(20);
  status->SetVisible(true);
}

inline void RealTimeMeter::format_time(float duration, char* buf) {
  int counter = int(duration);
  int ms = counter % 1000;
  counter /= 1000;
  int s = counter % 60;
  counter /= 60;
  int m = counter % 60;
  counter /= 60;
  int h = counter % 100;
  snprintf(buf, 16, "%02d:%02d:%02d.%03d", h, m, s, ms);
}

int64_t RealTimeMeter::get_system_timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void RealTimeMeter::update_status(float bml_time_ms, float sys_time_ms) {
  if (!status)
    return;
  char text[64], ingame_time_text[16], world_time_text[16];
  format_time(bml_time_ms, ingame_time_text);
  format_time(sys_time_ms, world_time_text);
  snprintf(text, 64, "Real Time\n Ingame  %s\n World   %s", ingame_time_text, world_time_text);
  status->SetText(text);
}
