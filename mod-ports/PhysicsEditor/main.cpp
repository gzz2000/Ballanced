#include "main.h"

static PhysicsEditor* active_mod = nullptr;

IMod* BMLEntry(IBML* bml) {
  active_mod = new PhysicsEditor(bml);
  return active_mod;
}

bool PhysicsEditor::set_physics() {
  for (size_t i = 0; i < physics_bb.size(); ++i) {
    auto* bb = physics_bb[i];
    if (!bb || !bb->GetInputParameter(0) || !bb->GetInputParameter(0)->GetRealSource() ||
        (bb->GetInputParameterCount() > 1 && (!bb->GetInputParameter(1) || !bb->GetInputParameter(1)->GetRealSource()))) {
      GetLogger()->Error("Physics graph %zu is unavailable; settings not applied", i);
      return false;
    }
  }
  for (const auto& bb: physics_bb) {
    auto* gravity_param = bb->GetInputParameter(0)->GetRealSource();
    gravity_param->SetValue(&gravity, sizeof(gravity));
    if (bb->GetInputParameterCount() > 1) {
      auto* time_param = bb->GetInputParameter(1)->GetRealSource();
      time_param->SetValue(&time_factor, sizeof(time_factor));
    }
    bb->ActivateInput(0);
    bb->Execute(0);
  }
  return true;
}

void PhysicsEditor::update_config() {
  gravity.x = prop_gravity_x->GetFloat();
  gravity.y = prop_gravity_y->GetFloat();
  gravity.z = prop_gravity_z->GetFloat();
  time_factor = prop_time_factor->GetFloat();
  game_speed = prop_game_speed->GetFloat();

  disabled = !(gravity != default_gravity || time_factor != default_time_factor || game_speed != 1.0f);

  if (disabled) {
    status->SetVisible(false);
  }
  else {
    std::string text = "PhysicsEditor";
    if (gravity != default_gravity) {
      std::string gravity_text(128, 0);
      gravity_text.resize(snprintf(gravity_text.data(), 128, " | Gravity: %.4g, %.4g (%.4g%%), %.4g",
                                  gravity.x, gravity.y, gravity.y / default_gravity.y * 100, gravity.z));
      text += gravity_text;
    }
    if (time_factor != default_time_factor) {
      std::string time_text(64, 0);
      time_text.resize(snprintf(time_text.data(), 64, " | Time factor: %.4gx (%.4g%%)",
                               time_factor, time_factor / default_time_factor * 100));
      text += time_text;
    }
    if (game_speed != 1.0f) {
      std::string text_game_speed(64, 0);
      text_game_speed.resize(snprintf(text_game_speed.data(), 64, " | Game speed: %.4gx",
                                 game_speed));
      text += text_game_speed;
    }
    status->SetText(text.c_str());
    status->SetVisible(true);
  }
  status->Process();
}

void PhysicsEditor::OnLoad() {
  GetConfig()->SetCategoryComment("Gravity", "Gravity of the game world.");
  prop_gravity_x = GetConfig()->GetProperty("Gravity", "X");
  prop_gravity_x->SetComment("Gravity on the X-axis. Default: 0.");
  prop_gravity_x->SetDefaultFloat(0);
  prop_gravity_y = GetConfig()->GetProperty("Gravity", "Y");
  prop_gravity_y->SetComment("Gravity on the Y-axis. Default: -20.");
  prop_gravity_y->SetDefaultFloat(-20);
  prop_gravity_z = GetConfig()->GetProperty("Gravity", "Z");
  prop_gravity_z->SetComment("Gravity on the Z-axis. Default: 0.");
  prop_gravity_z->SetDefaultFloat(0);

  GetConfig()->SetCategoryComment("Time", "Time of the game and its world.");
  prop_time_factor = GetConfig()->GetProperty("Time", "PhysicsTimeFactor");
  prop_time_factor->SetComment("Physics time factor of the game. Default: 2. (not 1 !!!)");
  prop_time_factor->SetDefaultFloat(2);
  prop_game_speed = GetConfig()->GetProperty("Time", "GlobalGameSpeed");
  prop_game_speed->SetComment("Global game speed scale (will affect everything). Default: 1.");
  prop_game_speed->SetDefaultFloat(1);

  status = std::make_unique<BGui::Label>("PhysicsStatus");
  status->SetAlignment(ALIGN_BOTTOM);
  status->SetPosition({ 0.5f, 0.998f });
  status->SetText("");
  status->SetFont(ExecuteBB::GAMEFONT_03);
  status->SetZOrder(20);
  status->SetVisible(false);

  update_config();
}

void PhysicsEditor::OnPostLoadLevel() {
  if (init)
    return;

  physics_bb.clear();
  CKBehavior* script;

  script = m_BML->GetScriptByName("Gameplay_Ingame");
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(script,
    "Init Ingame"), "Set Physics Globals"));

  script = m_BML->GetScriptByName("Gameplay_Refresh");
  auto* tutorial_freeze = ScriptHelper::FindFirstBB(script, "Tutorial freeze/unfreeze");
  // Stock game assets also contain this graph with a trailing space. The
  // current ScriptHelper does exact matching, unlike legacy assumptions.
  if (!tutorial_freeze)
    tutorial_freeze = ScriptHelper::FindFirstBB(script, "Tutorial freeze/unfreeze ");
  physics_bb.push_back(ScriptHelper::FindFirstBB(tutorial_freeze, "Set Physics Globals"));

  script = m_BML->GetScriptByName("Gameplay_Tutorial");
  auto* kapitel_aktion = ScriptHelper::FindFirstBB(script, "Kapitel Aktion");
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(kapitel_aktion,
    "Tut continue/exit"), "Set Physics Globals"));
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(kapitel_aktion,
    "MoveKeys"), "Set Physics Globals"));

  script = m_BML->GetScriptByName("Event_handler");
  physics_bb.push_back(ScriptHelper::FindFirstBB(
    ScriptHelper::FindFirstBB(script, "Unpause Level"), "Set Physics Globals"));
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(script,
    "reset Level"), "Set Physics Globals"));

  init = set_physics();
}

void PhysicsEditor::OnPostStartMenu() {
  if (init)
    return;
  m_BML->GetTimeManager()->SetTimeScaleFactor(game_speed);
}

void PhysicsEditor::OnStartLevel() {
  if (disabled) return;
  set_physics();
}

void PhysicsEditor::OnCamNavActive() {
  if (disabled) return;
  set_physics();
  m_BML->AddTimer(CKDWORD(1), [this]() { set_physics(); });
}

void PhysicsEditor::OnBallNavActive() {
  ++nav_activations;
  if (disabled) return;
  set_physics();
  m_BML->AddTimer(CKDWORD(1), [this]() { set_physics(); });
}

void PhysicsEditor::OnProcess() {
  if (disabled) return;
  status->Process();
}

void PhysicsEditor::OnModifyConfig(const char* category, const char* key, IProperty* prop) {
  update_config();
  m_BML->GetTimeManager()->SetTimeScaleFactor(game_speed);
}

void PhysicsEditor::OnUnload() {
  status.reset();
  physics_bb.clear();
  init = false;
  GetLogger()->Info("PhysicsEditor unloaded");
}

const char* PhysicsEditor::WebStatus() {
  static std::string result;
  char buf[512];
  auto* tm = m_BML->GetTimeManager();
  VxVector pos{};
  auto* current = m_BML->GetArrayByName("CurrentLevel");
  auto* ball = current ? static_cast<CK3dObject*>(current->GetElementObject(0, 1)) : nullptr;
  if (ball) ball->GetPosition(&pos);
  snprintf(buf, sizeof(buf), "{\"init\":%s,\"disabled\":%s,\"clock\":%.9g,\"delta\":%.9g,\"scale\":%.9g,\"tick\":%u,\"navActivations\":%u,\"ball\":[%.9g,%.9g,%.9g],\"blocks\":[",
           init ? "true" : "false", disabled ? "true" : "false", tm->GetAbsoluteTime(), tm->GetLastDeltaTime(), tm->GetTimeScaleFactor(), tm->GetMainTickCount(), nav_activations, pos.x,pos.y,pos.z);
  result = buf;
  for (auto* bb : physics_bb) {
    if (!bb) { if (result.back() != '[') result += ','; result += "null"; continue; }
    VxVector g{}; float t = 0;
    bb->GetInputParameter(0)->GetValue(&g);
    if (bb->GetInputParameterCount() > 1) bb->GetInputParameter(1)->GetValue(&t);
    if (result.back() != '[') result += ',';
    snprintf(buf,sizeof(buf),"{\"gravity\":[%.9g,%.9g,%.9g],\"timeFactor\":%.9g}",g.x,g.y,g.z,t);
    result += buf;
  }
  result += "]}";
  return result.c_str();
}

MOD_EXPORT const char* BML_WebPhysicsEditorStatus() { return active_mod ? active_mod->WebStatus() : "{}"; }
MOD_EXPORT void BMLExit(IMod* mod) { if (mod == active_mod) active_mod = nullptr; delete mod; }
