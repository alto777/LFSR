# include "LFSR.hpp"

/* recreation of Don Lancaster's Psychtone Music Composer-Synthesizer (note logic only) */

/*
a 6 bit LFSR with 4 pre-wired tap selections and a output bit weighting mechanism
takes a bit of doing but isn't always dreadful
*/

# define NUM_CHANNELS	6

struct Psychtone : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		STEP_PARAM,

		ENUMS(TUNE_SEL_PARAMS, 3),
		ENUMS(WEIGHT_PARAMS, 3),
		ENUMS(PAUSE_SEL_PARAMS, 6),
		ENUMS(LFSR_PARAM, NUM_CHANNELS),

		FWD_REV_PARAM,
		UP_DOWN_PARAM,

		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		OUTPUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		RUNNING_LIGHT,
		RESET_LIGHT,
		STEP_LIGHT,
		ENUMS(LFSR_LIGHTS, 6),
		NUM_LIGHTS
	};

	float phase = 0.0;

/* hello world global primitive info window 
	int hwCounter;
	int printValue = 0;
*/
/* running */
	dsp::SchmittTrigger runningTrigger;
	bool running = false;

/* clock */
	dsp::SchmittTrigger clockTrigger;
	bool nextStep = false;

/* step - reset internal clock phase and step the engine. GATE? */
	dsp::SchmittTrigger stepTrigger;
	dsp::SchmittTrigger resetTrigger;

/* shift register */
	bool bit;
	unsigned int lfsrBits;
	dsp::SchmittTrigger lfsrTrigger[6];

	Psychtone() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 3; i++) {
			configParam(Psychtone::TUNE_SEL_PARAMS + i, -36, 36, 6.0, "");
			configParam(Psychtone::WEIGHT_PARAMS + i, 0.0f, 12.0f, 0.0f, "");
		}
		for (int i = 0; i < 6; i++) {
			configParam(Psychtone::PAUSE_SEL_PARAMS + i, 0.0f, 2.0f, 1.0f, "");
			configParam(Psychtone::LFSR_PARAM + i, 0.0, 1.0, 0.0, "");
		}
		configParam(Psychtone::FWD_REV_PARAM, 0.0, 1.0, 1.0, "");
		configParam(Psychtone::UP_DOWN_PARAM, 0.0, 1.0, 1.0, "");
		configParam(Psychtone::CLOCK_PARAM, -2.0, 6.0, 2.0, "");
		configParam(Psychtone::RUN_PARAM, 0.0, 1.0, 0.0, "");
		configParam(Psychtone::STEP_PARAM, 0.0, 1.0, 0.0, "");
		onReset();
	}
	void process(const ProcessArgs& args) override;

	void onReset() override {
		lfsrBits = 0x0;
	}
};

/* OMG your brain is fried! */
static unsigned int tuneSelectBits[12] = {
	0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
};

void Psychtone::process(const ProcessArgs& args) {
/* Blink light at 1Hz
	float deltaTime = engineGetSampleTime();

	blinkPhase += deltaTime;
	if (blinkPhase >= 1.0f) blinkPhase -= 1.0f;
	lights[BLINK_LIGHT].value = (blinkPhase < 0.5f) ? 1.0f : 0.0f;
*/

/* running */
	if (runningTrigger.process(params[RUN_PARAM].getValue())) {
		running = !running;
	}
	lights[RUNNING_LIGHT].setBrightness(running ? 1.0f : 0.0f);

/* clock */
	bool nextStep = false;
	bool gateIn = false;

	if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
		lfsrBits = 0x0;
		phase = 0;		/* seems only fair? */
	}

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage())) {
				phase = 0.0;
				nextStep = true;
			}
			gateIn = clockTrigger.isHigh();
		}
		else {
			// Internal clock
			float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue() + inputs[CLOCK_INPUT].getVoltage());
			phase += clockTime * args.sampleTime;
			if (phase >= 1.0f) {
				phase -= 1.0f;
				nextStep = true;
			}
			gateIn = (phase < 0.5f);
		}
	}

/* for now, just feed our gateIn to the output */
//		outputs[GATE_OUTPUT].setVoltage(gateIn ? 10.0f : 0.0f);
		lights[CLOCK_LIGHT].setBrightness(gateIn ? 1.0f : 0.0f);
/*
returns -5.0 to 6 just fine, but still stops at max angle
setting value d0es not set physical angle

		weightedOutput += (tune[params[TUNE_SEL_PARAMS + i]] &
*/
	if (stepTrigger.process(params[STEP_PARAM].getValue())) {
		nextStep = true;
		phase = 0;
		gateIn = 1;
	}

	if (nextStep) {
		if (params[FWD_REV_PARAM].getValue() <= 0.0f)
			bit = (lfsrBits ^ (lfsrBits >> 5)) & 1;
		else
			bit = (lfsrBits ^ (lfsrBits >> 1)) & 1;

		lfsrBits >>= 1;

		if (params[UP_DOWN_PARAM].getValue() <= 0.0f) {
			if (bit) lfsrBits |= 0x20;
		}
		else {
			if (!bit) lfsrBits |= 0x20;
		}

		lfsrBits &= 0x3f;
	}

	int jj = 1;
	bool gateReason = false;
	for (int i = 0; i < 6; i++, jj <<= 1) {
		if (lfsrTrigger[i].process(params[LFSR_PARAM + i].getValue()))
			lfsrBits ^= jj;

		bit = !!(lfsrBits & jj);	/* mightn't need !! normalization */
		lights[LFSR_LIGHTS + i].setBrightness(bit ? 1.0 : 0.0);

		if (!gateReason)
			gateReason = ((params[PAUSE_SEL_PARAMS + i].getValue() < 1.0) && !bit) | ((params[PAUSE_SEL_PARAMS + i].getValue() > 1.0) && bit);
	}

	outputs[GATE_OUTPUT].setVoltage((gateIn & gateReason) ? 10.0f : 0.0f);
	lights[CLOCK_LIGHT].setBrightness((gateIn & gateReason) ? 1.0f : 0.0f);

/* calculate output */
	float theKnob;
	int myKnobIndex;

	float weightedOutput = 0;

/* just one knob first */
	for (int i = 0; i < 3; i++) {
		theKnob = params[TUNE_SEL_PARAMS + i].getValue();
		if (theKnob >= 7.0) theKnob -= 12.0;
		if (theKnob <= -6.0) theKnob += 12.0;
		params[TUNE_SEL_PARAMS + i].setValue(theKnob);

		theKnob += 6; if (theKnob >= 12) theKnob -= 12;

		myKnobIndex = theKnob;
		if (myKnobIndex < 6)
			weightedOutput += (tuneSelectBits[myKnobIndex] & lfsrBits) ? params[WEIGHT_PARAMS + i].getValue() : 0.0f;
		else
			weightedOutput -= (tuneSelectBits[myKnobIndex] & (lfsrBits ^ 0x3f)) ? params[WEIGHT_PARAMS + i].getValue() : 0.0f;
	}
	outputs[OUTPUT_OUTPUT].setVoltage(weightedOutput / 12.0f);

	lights[STEP_LIGHT].setSmoothBrightness(stepTrigger.isHigh(), args.sampleTime);
}

/* hello world... */
struct MyModuleDisplay : TransparentWidget {
	Psychtone *module;
	int frame = 0;
//	std::shared_ptr<Font> font;

	MyModuleDisplay() {
//		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Sudo.ttf"));
	}

/*	void draw(const DrawArgs &args) override {
		nvgFontSize(args.vg, 16);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);

		nvgFillColor(args.vg, nvgRGBA(0xe0, 0xe0, 0xff, 0x80));
		char text[128];
		if (module) {
		  snprintf(text, sizeof(text), "= %x", module->printValue);
		}
		nvgText(args.vg, 1, 1, text, NULL);
	}
*/
};

template <typename BASE>
struct bigLight : BASE {
	bigLight() {
		this->box.size = mm2px(Vec(6.0, 6.0));
	}
};

/* ...hello world */
struct myOwnKnob : SvgKnob {
	myOwnKnob() {
		box.size = Vec(40, 40);
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myOwnKnob.svg")));
//
// z'all it takes to tune this thing		snap = true;
		shadow->opacity = -1.0;
	}
};

struct myBigKnob : SvgKnob {
	myBigKnob() {
		minAngle = -6.0 * M_PI;
		maxAngle = 6.0 * M_PI;
//		snap - shows large mouse travel needed
		snap = true;
		smooth = false;

		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBigKnob.svg")));
	}
};

/* test variations for infinite but snapping knobs */
struct xBigKnob : SvgKnob {
	xBigKnob() {
		minAngle = -M_PI / 6.0;
		maxAngle = M_PI / 6.0;

		snap = true;
		smooth = false;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBigKnob.svg")));
	}
};

struct yBigKnob : SvgKnob {
	yBigKnob() {
		minAngle = -2.0 * M_PI;
		maxAngle = 2.0 * M_PI;

		snap = true;
		smooth = false;

		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBigKnob.svg")));
	}
};

struct my2Switch : SvgSwitch {
	my2Switch() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/togSwitch0ff.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/togSwitch0n.svg")));
	}
};

struct PsychtoneWidget : ModuleWidget {
	PsychtoneWidget(Psychtone *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Psychtone.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, box.size.y - 15)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, box.size.y - 15)));

/* hello world... */
	{
		MyModuleDisplay *display = new MyModuleDisplay();
		display->module = module;
		display->box.pos = Vec(100, 170);
		display->box.size = Vec(50, 180);
		addChild(display);
	}
/* ...hello world */
/* TUNE SELECT KNOBS w/ trimmer *//* not quite... */
/* snap rotary encoder */
/* my */
		addParam(createParam<myBigKnob>(Vec(37, 51), module, Psychtone::TUNE_SEL_PARAMS));
		addParam(createParam<myOwnKnob>(Vec(37 + 18, 51 + 18), module, Psychtone::WEIGHT_PARAMS));

		addParam(createParam<myBigKnob>(Vec(148, 51), module, Psychtone::TUNE_SEL_PARAMS + 1));
		addParam(createParam<myOwnKnob>(Vec(148 + 18, 51 + 18), module, Psychtone::WEIGHT_PARAMS + 1));

		addParam(createParam<myBigKnob>(Vec(262, 51), module, Psychtone::TUNE_SEL_PARAMS + 2));
		addParam(createParam<myOwnKnob>(Vec(262 + 18, 51 + 18), module, Psychtone::WEIGHT_PARAMS + 2));

/* the lights and pause select */
/* slide switch 3 position parameter */
		float xa = 313.0; float dx = -52.0;
		for (int i = 0; i < 6; i++, xa += dx) {
			addParam(createParam<CKSSThree>(Vec(xa, 309), module, Psychtone::PAUSE_SEL_PARAMS + i));

			addParam(createParam<LEDBezel>(Vec(xa - 1 - 3, 282 - 2.5), module, Psychtone::LFSR_PARAM + i));
			addChild(createLight<bigLight<BlueLight>>(Vec(xa + 1.5 - 3, 284 - 2.5), module, Psychtone::LFSR_LIGHTS + i));
		}

/* input/output */
		addInput(createInput<PJ301MPort>(Vec(42, 228), module, Psychtone::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(Vec(88, 228), module, Psychtone::EXT_CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(Vec(134, 228), module, Psychtone::RESET_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(303, 228), module, Psychtone::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(303, 228 - 44), module, Psychtone::OUTPUT_OUTPUT));

/* shift register control switches */

		addParam(createParam<my2Switch>(Vec(188, 198), module, Psychtone::FWD_REV_PARAM));
		addParam(createParam<my2Switch>(Vec(238, 198), module, Psychtone::UP_DOWN_PARAM));

		addParam(createParam<RoundLargeBlackKnob>(Vec(34, 179), module, Psychtone::CLOCK_PARAM));

		addParam(createParam<LEDBezel>(Vec(89, 187.5), module, Psychtone::RUN_PARAM));
		addChild(createLight<bigLight<GreenLight>>(Vec(91, 189.5), module, Psychtone::RUNNING_LIGHT));

		addParam(createParam<LEDBezel>(Vec(135, 187.5), module, Psychtone::STEP_PARAM));
		addChild(createLight<bigLight<GreenLight>>(Vec(137, 189.5), module, Psychtone::STEP_LIGHT));

		addChild(createLight<MediumLight<BlueLight>>(Vec(15, 15), module, Psychtone::CLOCK_LIGHT));
	}
};

Model *modelPsychtone = createModel<Psychtone, PsychtoneWidget>("Psychtone");
