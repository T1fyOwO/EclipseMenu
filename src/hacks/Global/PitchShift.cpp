#include <algorithm>
#include <cmath>
#include <vector>

#include <Geode/modify/FMODAudioEngine.hpp>

#include <modules/config/config.hpp>
#include <modules/gui/components/float-toggle.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/gui/gui.hpp>
#include <modules/hack/hack.hpp>

namespace eclipse::hacks::Global {
	constexpr int kMaxPitchStages = 10;

	static std::vector<FMOD::DSP *> &pitchShiftChain() {
		static std::vector<FMOD::DSP *> chain;
		return chain;
	}

	static bool ensureStages(int count) {
		auto &chain = pitchShiftChain();
		if (static_cast<int>(chain.size()) >= count) {
			return true;
		}

		auto *engine = FMODAudioEngine::sharedEngine();
		if (!engine || !engine->m_system) {
			return false;
		}

		FMOD::ChannelGroup *masterGroup;
		if (engine->m_system->getMasterChannelGroup(&masterGroup) != FMOD_OK) {
			return false;
		}

		while (static_cast<int>(chain.size()) < count) {
			FMOD::DSP *dsp = nullptr;
			if (engine->m_system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &dsp) != FMOD_OK) {
				return false;
			}

			dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_FFTSIZE, 1024.f);
			dsp->setBypass(true);
			masterGroup->addDSP(0, dsp);
			chain.push_back(dsp);
		}

		return true;
	}

	static float currentAudioSpeed() {
		if (config::get<"global.audiospeed.toggle", bool>()) {
			return config::get<"global.audiospeed", double>(1.0);
		}

		if (config::get<"global.audiospeed.sync", bool>() && config::get<"global.speedhack.toggle", bool>()) {
			return config::get<"global.speedhack", double>(1.0);
		}

		return 1.f;
	}

	static void setPitch(float pitch) {
		static const float kMinPitch = std::pow(0.5f, static_cast<float>(kMaxPitchStages));
		static const float kMaxPitch = std::pow(2.0f, static_cast<float>(kMaxPitchStages));
		pitch = std::clamp(pitch, kMinPitch, kMaxPitch);

		const bool neutral = std::fabs(pitch - 1.f) < 0.001f;

		const int stages =
			neutral ? 0 : std::clamp(static_cast<int>(std::ceil(std::fabs(std::log2(pitch)))), 1, kMaxPitchStages);

		if (stages > 0 && !ensureStages(stages)) {
			return;
		}

		auto &chain = pitchShiftChain();

		const float perStage = stages > 0 ? std::clamp(std::pow(pitch, 1.f / stages), 0.5f, 2.0f) : 1.f;

		for (std::size_t i = 0; i < chain.size(); ++i) {
			if (static_cast<int>(i) < stages) {
				chain[i]->setBypass(false);
				chain[i]->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, perStage);
			} else {
				chain[i]->setBypass(true);
				chain[i]->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, 1.f);
			}
		}
	}

	class $hack(PitchShift) {
		void init() override {
			auto tab = gui::MenuTab::find("tab.global");

			config::setIfEmpty("global.pitchshift.toggle", false);
			config::setIfEmpty("global.autopitchshift.toggle", false);
			config::setIfEmpty("global.pitchshift", 1.f);

			tab->addFloatToggle("global.pitchshift", 0.001f, 15.f, "%.3f")
				->valueCallback(setPitch)
				->handleKeybinds()
				->setDescription()
				->toggleCallback([] {
					if (config::get<bool>("global.pitchshift.toggle", false)) {
						setPitch(config::get<double>("global.pitchshift", 1.f));
					} else if (!config::get<bool>("global.autopitchshift.toggle", false)) {
						setPitch(1.f);
					}
				});

			tab->addToggle("global.autopitchshift.toggle")->handleKeybinds()->setDescription()->callback([](bool) {
				if (!config::get<bool>("global.autopitchshift.toggle", false) &&
					!config::get<bool>("global.pitchshift.toggle", false)) {
					setPitch(1.f);
				}
			});
		}

		void lateInit() override {
			if (config::get<bool>("global.pitchshift.toggle", false)) {
				setPitch(config::get<double>("global.pitchshift", 1.f));
			}
		}

		[[nodiscard]]
		const char *getId() const override {
			return "Pitch Shift";
		}

		[[nodiscard]]
		int32_t getPriority() const override {
			return -9;
		}
	};

	REGISTER_HACK(PitchShift)

	class $modify(PitchShiftFMODHook, FMODAudioEngine) {
		static bool shouldUpdate() { return config::get<"global.autopitchshift.toggle", bool>(); }

		void update(float dt) override {
			FMODAudioEngine::update(dt);
			if (!shouldUpdate()) {
				return;
			}

			if (config::get<"global.pitchshift.toggle", bool>()) {
				return;
			}

			auto speed = currentAudioSpeed();
			if (speed <= 0.f) {
				speed = 1.f;
			}

			setPitch(1.f / speed);
		}
	};
} // namespace eclipse::hacks::Global
