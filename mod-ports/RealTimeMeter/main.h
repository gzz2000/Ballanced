#pragma once

#include <BML/BMLAll.h>
#include <chrono>
#include <memory>

extern "C" {
	MOD_EXPORT IMod* BMLEntry(IBML* bml);
    MOD_EXPORT void BMLExit(IMod* mod);
}

class RealTimeMeter : public IMod {
public:
	RealTimeMeter(IBML* bml) : IMod(bml) {};

	virtual const char * GetID() override { return "RealTimeMeter"; }
	virtual const char * GetVersion() override { return "0.1.2"; }
	virtual const char * GetName() override { return "Real Time Meter"; }
	virtual const char * GetAuthor() override { return "BallanceBug"; }
	virtual const char * GetDescription() override { return "Measure the actual time elapsed during your gameplays."; }
	DECLARE_BML_VERSION;

	void OnLoad() override;
    void OnUnload() override;
    const char *Inspect() const;
    const char *LabelText() const { return status ? status->GetText() : ""; }
	void OnModifyConfig(const char * category, const char * key, IProperty* prop) override;
	void OnLoadObject(const char * filename, CKBOOL isMap, const char * masterName,
										CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
										CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override;
	void OnPostResetLevel() override;
	void OnPreLoadLevel() override;
	void OnPreExitLevel() override;
	void OnStartLevel() override;
	void OnLevelFinish() override;
	void OnCounterInactive() override;
	void OnCounterActive() override;
	void OnProcess() override;

	void init_status_label();
	inline void format_time(float duration, char* buf);
	static int64_t get_system_timestamp();
	inline void update_status(float bml_time_ms, float sys_time_ms);

private:
	std::unique_ptr<BGui::Label> status;
	bool enabled = false, counting = false;
	bool level_finished = false, level_exited = true, streak_completed = false;
	float bml_time = 0;
	int64_t sys_time = 0, last_finish_sys_time = 0, last_exit_sys_time = 0;
	CKTimeManager* time_manager = nullptr;
	IProperty *prop_enable = nullptr, *prop_streak = nullptr;
	int streak_level = 0, current_level = 0;
};
