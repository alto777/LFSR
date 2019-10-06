# include "LFSR.hpp"

struct a7Utility : Module {
	enum ParamIds {
		ENUMS(MBUTTON_PARAM, 2),
		ENUMS(MMODE_PARAM, 2),

		ENUMS(CCTRL_PARAM, 2),
		ENUMS(CAMP_PARAM, 2),
		ENUMS(CRANGE_PARAM, 2),

		ENUMS(WIDTH_PARAM, 2),

		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CCTRL_INPUT, 2),

		ENUMS(EDGE_INPUT, 2),

		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MGATE_OUTPUT, 2),
		ENUMS(MTRIG_OUTPUT, 2),

		ENUMS(COUT_OUTPUT, 2),

		ENUMS(RISE_OUTPUT, 2),
		ENUMS(FALL_OUTPUT, 2),

		ENUMS(CLOCK_INV_OUTPUT, 2),

		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(M_LIGHT, 2),

		ENUMS(RISE_LIGHT, 2),
		ENUMS(FALL_LIGHT, 2),

		NUM_LIGHTS
	};

/* buttons */
	dsp::SchmittTrigger manualButton[2];
	dsp::PulseGenerator manualTriggerPulse[2];

	bool mState[2] = {0, 0};

/* constants - no variables */

/* edge detector */
	bool signalState[2] = {0, 0};
	dsp::PulseGenerator risingEdgePulse[2];
	dsp::PulseGenerator fallingEdgePulse[2];

	dsp::PulseGenerator risingEdgeLampStab[2];
	dsp::PulseGenerator fallingEdgeLampStab[2];

	a7Utility() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 2; i++) {
			configParam(a7Utility::MBUTTON_PARAM + i, 0.0, 1.0, 0.0, "");
			configParam(a7Utility::MMODE_PARAM + i, 0.0, 1.0, 0.0, "");
			configParam(a7Utility::CCTRL_PARAM + i, 0.0, 1.0, 0.0, "");
			configParam(a7Utility::CAMP_PARAM + i, 0.0, 1.0, 0.0, "");
			configParam(a7Utility::CRANGE_PARAM + i, 0.0, 1.0, 0.0, "");
			configParam(a7Utility::WIDTH_PARAM + i, 0.0, 1.0, 0.0, "");
		}
	}
	void process(const ProcessArgs& args) override;

	void onReset() override {
		mState[0] = 0;
	};

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_t *mstate = json_array();

		for (int i = 0; i < 2; i++)
			json_array_insert_new(mstate, i, json_integer((int) mState[i]));

		json_object_set_new(rootJ, "mstate", mstate);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *mstate = json_object_get(rootJ, "mstate");

		if (mstate) {
			for (int i = 0; i < 2; i++) {
				json_t *wha = json_array_get(mstate, i);
				if (wha)
					mState[i] = json_integer_value(wha);
			}
		}
	}
};

void a7Utility::process(const ProcessArgs& args) {
	float deltaTime = 1.0 / args.sampleRate;

/* manual button to gate / trigger section */
	for (int i = 0; i < 2; i++) {
		// mode 0 = momentary 1 = toggle
		bool mode = params[MMODE_PARAM + i].getValue() > 0.0f;
		if (!mode)
			mState[i] = 0;

		if (manualButton[i].process(params[MBUTTON_PARAM + i].getValue())) {
			manualTriggerPulse[i].trigger(0.001);
			if (mode) mState[i] ^= 1;
		}

		bool bState = manualButton[i].isHigh();
		if (mState[i] || (!mode && bState)) {
			outputs[MGATE_OUTPUT + i].setVoltage(10.0f);
			lights[M_LIGHT + i].setBrightness(1.0f);
		}
		else {
			outputs[MGATE_OUTPUT + i].setVoltage(0.0f);
			lights[M_LIGHT + i].setBrightness(0.0);
		}

		outputs[MTRIG_OUTPUT + i].setVoltage( manualTriggerPulse[i].process(deltaTime) ? 10.0f : 0.0);
	}

/* constant from knob switched by control */
	for (int i = 0; i < 2; i++) {
		bool isOn = params[CCTRL_PARAM + i].getValue() <= 0.0f;
		bool rangeX10 = params[CRANGE_PARAM + i].getValue() <= 0.0f;
		bool extControl = inputs[CCTRL_INPUT + i].isConnected();
		bool nonZero = 0;

		if (isOn) nonZero = 1;
		else {
			if (extControl) {
				if (inputs[CCTRL_INPUT + i].getVoltage() > 0.0f) nonZero = 1;
			}
			else nonZero = 1;
		}

		outputs[COUT_OUTPUT + i].setVoltage(nonZero ? (params[CAMP_PARAM + i].getValue() * (rangeX10 ? 10.0f : 1.0f)) : 0);
	}

/* generate pulse on clock edges */
	for (int i = 0; i < 2; i++) {
		if (signalState[i]) {
			if (inputs[EDGE_INPUT + i].getVoltage() < 0.3f) {
				signalState[i] = 0;
				fallingEdgePulse[i].trigger(0.001); // (params[WIDTH_PARAM + i].getValue() <= 0.0f ? 0.005f : 0.0005f);
				fallingEdgeLampStab[i].trigger(0.25); // (params[WIDTH_PARAM + i].getValue() <= 0.0f ? 0.005f : 0.0005f);
			}
		}
		else {
			if (inputs[EDGE_INPUT + i].getVoltage() > 0.7f) {
				signalState[i] = 1;
				risingEdgePulse[i].trigger(0.001); // (params[WIDTH_PARAM + i].getValue() <= 0.0f ? 0.005f : 0.0005f);
				risingEdgeLampStab[i].trigger(0.25); // params[WIDTH_PARAM + i].getValue() <= 0.0f ? 0.33f : 0.1f);
			}
		}

		outputs[RISE_OUTPUT + i].setVoltage(risingEdgePulse[i].process(deltaTime) ? 10.0f : 0.0);
		outputs[FALL_OUTPUT + i].setVoltage(fallingEdgePulse[i].process(deltaTime) ? 10.0f : 0.0);

		outputs[CLOCK_INV_OUTPUT + i].setVoltage(signalState[i] ? 0.0f : 10.0f);

		lights[RISE_LIGHT + i].setBrightness(risingEdgeLampStab[i].process(deltaTime) ? 1.0f : 0.0);
		lights[FALL_LIGHT + i].setBrightness(fallingEdgeLampStab[i].process(deltaTime) ? 1.0f : 0.0);
	}
}

/* why oh */
struct myCKSS : SvgSwitch {
	myCKSS() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myCKSS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myCKSS_1.svg")));
	}
};

struct myHCKSS : SvgSwitch {
	myHCKSS() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myHCKSS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myHCKSS_1.svg")));
	}
};

struct HexCapScrew0 : SvgScrew {
	HexCapScrew0() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HexCapScrewSilver.svg")));
		box.size = sw->box.size;
	}
};

struct HexCapScrew1 : SvgScrew {
	HexCapScrew1() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HexCapScrewSilver9.svg")));
		box.size = sw->box.size;
	}
};

struct HexCapScrew2 : SvgScrew {
	HexCapScrew2() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HexCapScrewSilver13.svg")));
		box.size = sw->box.size;
	}
};

struct HexCapScrew3 : SvgScrew {
	HexCapScrew3() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HexCapScrewSilver21.svg")));
		box.size = sw->box.size;
	}
};

struct a7UtilityWidget : ModuleWidget {
	a7UtilityWidget(a7Utility *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/a7Utility.svg")));

		addChild(createWidget<HexCapScrew3>(Vec(0, 0)));
		addChild(createWidget<HexCapScrew2>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<HexCapScrew0>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<HexCapScrew1>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<LEDBezel>(mm2px(Vec(0.798, 13.042)), module, a7Utility::MBUTTON_PARAM + 0));
		addParam(createParam<LEDBezel>(mm2px(Vec(25.851, 13.042)), module, a7Utility::MBUTTON_PARAM + 1));

		addParam(createParam<myCKSS>(mm2px(Vec(13.949, 14.059)), module, a7Utility::MMODE_PARAM + 0));
		addParam(createParam<myCKSS>(mm2px(Vec(39.003, 14.059)), module, a7Utility::MMODE_PARAM + 1));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(0.37, 23.495)), module, a7Utility::MTRIG_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(11.623, 23.495)), module, a7Utility::MGATE_OUTPUT + 0));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(25.424, 23.495)), module, a7Utility::MTRIG_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(36.677, 23.495)), module, a7Utility::MGATE_OUTPUT + 1));

		addChild(createLight<LargeLight<RedLight>>(mm2px(Vec(1.958, 14.2)), module, a7Utility::M_LIGHT + 0 ));
		addChild(createLight<LargeLight<RedLight>>(mm2px(Vec(27.012, 14.2)), module, a7Utility::M_LIGHT + 1));

		addParam(createParam<myHCKSS>(mm2px(Vec(23.806, 51.25)), module, a7Utility::CCTRL_PARAM + 0));
		addParam(createParam<RoundBlackKnob>(mm2px(Vec(9.682, 53.452)), module, a7Utility::CAMP_PARAM + 0));
		addParam(createParam<myCKSS>(mm2px(Vec(2.208, 55.72)), module, a7Utility::CRANGE_PARAM + 0));

		addParam(createParam<myHCKSS>(mm2px(Vec(23.806, 70.872)), module, a7Utility::CCTRL_PARAM + 1));
		addParam(createParam<RoundBlackKnob>(mm2px(Vec(9.682, 73.073)), module, a7Utility::CAMP_PARAM + 1));
		addParam(createParam<myCKSS>(mm2px(Vec(2.208, 75.341)), module, a7Utility::CRANGE_PARAM + 1));

		addInput(createInput<PJ301MPort>(mm2px(Vec(22.359, 57.296)), module, a7Utility::CCTRL_INPUT + 0));
		addInput(createInput<PJ301MPort>(mm2px(Vec(22.359, 76.917)), module, a7Utility::CCTRL_INPUT + 1));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(34.118, 54.272)), module, a7Utility::COUT_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(34.118, 73.894)), module, a7Utility::COUT_OUTPUT + 1));

		addInput(createInput<PJ301MPort>(mm2px(Vec(4.113, 101.77)), module, a7Utility::EDGE_INPUT + 0));
		addInput(createInput<PJ301MPort>(mm2px(Vec(4.113, 113.7)), module, a7Utility::EDGE_INPUT + 1));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(14.184, 101.77)), module, a7Utility::RISE_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(23.701, 101.77)), module, a7Utility::FALL_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(34.118, 101.77)), module, a7Utility::CLOCK_INV_OUTPUT + 0));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(14.184, 113.7)), module, a7Utility::RISE_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(23.701, 113.7)), module, a7Utility::FALL_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(34.118, 113.7)), module, a7Utility::CLOCK_INV_OUTPUT + 1));

		addChild(createLight<MediumLight<BlueLight>>(mm2px(Vec(16.773, 97.812)), module, a7Utility::RISE_LIGHT + 0));
		addChild(createLight<MediumLight<YellowLight>>(mm2px(Vec(26.291, 97.812)), module, a7Utility::FALL_LIGHT + 0));
		addChild(createLight<MediumLight<BlueLight>>(mm2px(Vec(16.773, 122.818)), module, a7Utility::RISE_LIGHT + 1));
		addChild(createLight<MediumLight<YellowLight>>(mm2px(Vec(26.291, 122.818)), module, a7Utility::FALL_LIGHT + 1));
	}
};

Model *modela7Utility = createModel<a7Utility, a7UtilityWidget>("a7Utility");
