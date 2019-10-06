# include "LFSR.hpp"

struct Divada : Module {
	enum ParamIds {
		ENUMS(DIVIDE_BY_PARAM, 5),
		RESET_PARAM,

		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CLOCK_IN_INPUT, 5),
		RESET_INPUT,

		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CLOCK_OUT_OUTPUT, 5),

		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LAMP,

		NUM_LIGHTS
	};

	dsp::SchmittTrigger clockTrigger[5];
	const int factor[12] = {2, 3, 4, 5, 7, 8, 11, 13, 16, 17, 19, 23,};

	int divCount[5] = {0, 0, 0, 0, 0};		/* [] */

	dsp::SchmittTrigger resetTrigger;
	float resetLight = 0.0;

	Divada() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int ii = 0; ii < 5; ii++)
			configParam(Divada::DIVIDE_BY_PARAM + ii, 0.0, 11.0, 0.0, "");
		configParam(Divada::RESET_PARAM, 0.0, 1.0, 0.0, "");
	}
	void process(const ProcessArgs& args) override;
};

struct mySmallSnapKnob : RoundSmallBlackKnob {
	mySmallSnapKnob() {
		snap = true;
		smooth = false;
	}
};

void Divada::process(const ProcessArgs& args) {
	const float lightLambda = 0.075;

	if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
		for (int ii = 0; ii < 5; ii++)
			divCount[ii] = 0;
		resetLight = 1.0f;
	}

	lights[RESET_LAMP].setBrightness(resetLight);
	resetLight -= resetLight / lightLambda / args.sampleRate;

	for (int ii = 0; ii < 5; ii++) {
		int divBy = int(params[DIVIDE_BY_PARAM + ii].getValue() + 0.1);
		divBy = factor[divBy];

		if (clockTrigger[ii].process(inputs[CLOCK_IN_INPUT + ii].getVoltage()))
			if (++divCount[ii] >= divBy) divCount[ii] = 0;
		outputs[CLOCK_OUT_OUTPUT + ii].setVoltage((divCount[ii] < (divBy / 2)) ? 10.0f : 0.0f);
	}
}

struct DivadaWidget : ModuleWidget {
  DivadaWidget(Divada *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Divada.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float xmarginR = 1.0f; float xmarginL = 21.125;
		float ymargin = 16.0f;
		float inup = 5.0f;
		float unitSpace = 19.0f;

		for (int ii = 0; ii < 5; ii++) {
			addInput(createInput<PJ301MPort>(mm2px(Vec(xmarginR, ymargin + ii * unitSpace - inup)), module, Divada::CLOCK_IN_INPUT + ii));
			addParam(createParam<mySmallSnapKnob>(mm2px(Vec(11.24, ymargin + 0.1775f + ii * unitSpace)), module, Divada::DIVIDE_BY_PARAM + ii));
			addOutput(createOutput<PJ301MPort>(mm2px(Vec(xmarginL, ymargin + ii * unitSpace + inup)), module, Divada::CLOCK_OUT_OUTPUT + ii));
		}

		addInput(createInput<PJ301MPort>(mm2px(Vec(xmarginL - 5.0f, ymargin + 5.0f + 5 * unitSpace)), module, Divada::RESET_INPUT));
		addParam(createParam<LEDButton>(mm2px(Vec(7.0f - 3.0f + 0.135f, ymargin + 5.0f + 5 * unitSpace + 1.0f)), module, Divada::RESET_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(mm2px(Vec(7.0f - 3.0f + 1.49f + 0.135f, ymargin + 5.0f + 5 * unitSpace + 1.49f + 1.0f)), module, Divada::RESET_LAMP));
  }
};

Model *modelDivada = createModel<Divada, DivadaWidget>("Divada");
