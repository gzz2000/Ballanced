#pragma once

#include <BML/BMLAll.h>
#include <memory>

extern "C" {
  MOD_EXPORT IMod* BMLEntry(IBML* bml);
  MOD_EXPORT void BMLExit(IMod* mod);
}

class PhysicsEditor : public IMod {
public:
  PhysicsEditor(IBML* bml) : IMod(bml) {}

  virtual const char* GetID() override { return "PhysicsEditor"; }
  virtual const char* GetVersion() override { return "0.0.4"; }
  virtual const char* GetName() override { return "Physics Editor"; }
  virtual const char* GetAuthor() override { return "BallanceBug"; }
  virtual const char* GetDescription() override { return "A simple mod for editing your physics settings."; }
  DECLARE_BML_VERSION;

  virtual void OnLoad() override;
  virtual void OnUnload() override;
  virtual void OnPostLoadLevel() override;
  virtual void OnPostStartMenu() override;
  virtual void OnStartLevel() override;
  virtual void OnCamNavActive() override;
  virtual void OnBallNavActive() override;
  virtual void OnProcess() override;
  virtual void OnModifyConfig(const char* category, const char* key, IProperty* prop) override;

public:
  // Read-only probe for acceptance tests; not a host ABI extension.
  const char* WebStatus();
private:
  std::vector<CKBehavior*> physics_bb;
  std::unique_ptr<BGui::Label> status;
  IProperty *prop_gravity_x, *prop_gravity_y, *prop_gravity_z, *prop_time_factor, *prop_game_speed;
  const VxVector default_gravity = { 0, -20, 0 };
  VxVector gravity = default_gravity;
  static constexpr const float default_time_factor = 2;
  float time_factor = default_time_factor;
  float game_speed = 1.0f;
  unsigned nav_activations = 0;
  bool init = false;
  bool disabled = true;

  bool set_physics();
  void update_config();
};
